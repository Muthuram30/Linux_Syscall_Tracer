/* syscall_tracer.c - system call tracer built on ptrace().
 *
 * How it works (see README for the long version):
 *   1. The tracee is either fork()ed by us (child calls PTRACE_TRACEME, then exec()s)
 *      or an existing process is attached with PTRACE_ATTACH.
 *   2. We resume it with PTRACE_SYSCALL: the kernel stops the tracee at every system
 *      call ENTRY and again at every EXIT and notifies us through waitpid().
 *   3. At each stop, PTRACE_GET_SYSCALL_INFO (Linux >= 5.3) tells us whether it is an
 *      entry or exit, the syscall number, its arguments and the return value.  This is
 *      architecture independent, so no register layout is hard-coded.
 *   4. PTRACE_O_TRACEFORK/VFORK/CLONE make the kernel auto-attach new children, so the
 *      whole process tree is traced.
 */
#define _GNU_SOURCE
#include "syscall_tracer.h"
#include "logger.h"
#include "process_monitor.h"
#include "syscall_table.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Defined by hand to avoid the <linux/ptrace.h> vs <sys/ptrace.h> header clash. */
#ifndef PTRACE_GET_SYSCALL_INFO
#define PTRACE_GET_SYSCALL_INFO 0x420e
#endif
struct sc_info {
    uint8_t op; uint8_t pad[3]; uint32_t arch;
    uint64_t instruction_pointer, stack_pointer;
    union {
        struct { uint64_t nr; uint64_t args[6]; } entry;
        struct { int64_t rval; uint8_t is_error; } exit;
        struct { uint64_t nr; uint64_t args[6]; uint32_t ret_data; } seccomp;
    };
};
#define SC_OP_ENTRY 1
#define SC_OP_EXIT  2
#define AUDIT_ARCH_X86_64_  0xc000003eu
#define AUDIT_ARCH_AARCH64_ 0xc00000b7u

#define STREAM_CAP 120
#define MILE_CAP   60
#define HIST_CAP   180

typedef struct {
    long ms; pid_t pid; long nr; long ret; int fail; int has_ret;
    unsigned long seq; char detail[96];
} Event;

typedef struct { long ms; double cpu; unsigned long rss_kb, vsize_kb; } Sample;

typedef struct {
    pid_t pid, parent;
    int alive, in_call;
    long nr; uint64_t args[6];
    char detail[96]; char comm[32];
} Thread;

typedef struct {
    TracerOptions opt;
    Logger log;
    SyscallStats stats;
    Thread *th; size_t nth, capth;
    int live;
    pid_t root;
    char command[256];
    Event stream[STREAM_CAP]; int s_head, s_n;
    Event miles[MILE_CAP];    int m_head, m_n;
    Sample hist[HIST_CAP];    int h_head, h_n;
    unsigned long seq;
    long start_ms, last_snap_ms, last_total;
    double rate;
    ProcInfo prev, cur; int have_cur, have_prev;
    struct timespec prev_ts;
    int exit_code, exit_signal;
    const char *status;
    int arch_warned;
} Tracer;

static volatile sig_atomic_t g_stop, g_tick;
static void on_int(int s)  { (void)s; g_stop = 1; }
static void on_tick(int s) { (void)s; g_tick = 1; }

/* ---------- helpers ---------- */

static Thread *thread_get(Tracer *t, pid_t pid, int create)
{
    for (size_t i = 0; i < t->nth; i++)
        if (t->th[i].pid == pid) {
            if (create && !t->th[i].alive) { t->th[i].alive = 1; t->live++; }
            return &t->th[i];
        }
    if (!create) return NULL;
    if (t->nth == t->capth) {
        size_t nc = t->capth ? t->capth * 2 : 16;
        Thread *p = realloc(t->th, nc * sizeof *p);
        if (!p) { perror("realloc"); exit(1); }
        t->th = p; t->capth = nc;
    }
    Thread *n = &t->th[t->nth++];
    memset(n, 0, sizeof *n);
    n->pid = pid; n->alive = 1; t->live++;
    return n;
}

static void read_comm(pid_t pid, char *buf, size_t n)
{
    char path[64];
    snprintf(path, sizeof path, "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (f) { if (fgets(buf, (int)n, f)) buf[strcspn(buf, "\n")] = 0; fclose(f); }
}

static char proc_state(pid_t pid)
{
    ProcInfo pi;
    return pm_read(pid, &pi) == 0 ? pi.state : '?';
}

/* Read a NUL-terminated string from the tracee's memory word by word. */
static void peek_string(pid_t pid, uint64_t addr, char *out, size_t max)
{
    size_t i = 0;
    out[0] = 0;
    while (i + 1 < max) {
        errno = 0;
        long w = ptrace(PTRACE_PEEKDATA, pid, (void *)(uintptr_t)(addr + i), NULL);
        if (errno) break;
        for (size_t b = 0; b < sizeof w && i + 1 < max; b++) {
            char c = (char)((w >> (8 * b)) & 0xff);
            if (!c) { out[i] = 0; return; }
            out[i++] = (c == '|' || c == '"' || c == '\\' || (unsigned char)c < 32) ? '?' : c;
        }
    }
    out[i] = 0;
}

static void describe(pid_t pid, long nr, const uint64_t *a, char *out, size_t n)
{
    char path[80];
    out[0] = 0;
#ifdef SYS_openat
    if (nr == SYS_openat) { peek_string(pid, a[1], path, sizeof path); snprintf(out, n, "path=%s flags=0x%lx", path, (unsigned long)a[2]); return; }
#endif
#ifdef SYS_open
    if (nr == SYS_open)   { peek_string(pid, a[0], path, sizeof path); snprintf(out, n, "path=%s flags=0x%lx", path, (unsigned long)a[1]); return; }
#endif
#ifdef SYS_execve
    if (nr == SYS_execve) { peek_string(pid, a[0], path, sizeof path); snprintf(out, n, "path=%s", path); return; }
#endif
#ifdef SYS_read
    if (nr == SYS_read || nr == SYS_write) { snprintf(out, n, "fd=%ld count=%lu", (long)a[0], (unsigned long)a[2]); return; }
#endif
#ifdef SYS_close
    if (nr == SYS_close)  { snprintf(out, n, "fd=%ld", (long)a[0]); return; }
#endif
#ifdef SYS_mmap
    if (nr == SYS_mmap)   { snprintf(out, n, "len=%lu prot=0x%lx", (unsigned long)a[1], (unsigned long)a[2]); return; }
#endif
#ifdef SYS_munmap
    if (nr == SYS_munmap) { snprintf(out, n, "len=%lu", (unsigned long)a[1]); return; }
#endif
#ifdef SYS_wait4
    if (nr == SYS_wait4)  { snprintf(out, n, "pid=%ld", (long)a[0]); return; }
#endif
#ifdef SYS_exit
    if (nr == SYS_exit) { snprintf(out, n, "status=%ld", (long)a[0]); return; }
#endif
#ifdef SYS_exit_group
    if (nr == SYS_exit_group) { snprintf(out, n, "status=%ld", (long)a[0]); return; }
#endif
}

static int is_milestone(long nr)
{
#ifdef SYS_read
    if (nr == SYS_read || nr == SYS_write || nr == SYS_close) return 1;
#endif
#ifdef SYS_execve
    if (nr == SYS_execve) return 1;
#endif
#ifdef SYS_clone
    if (nr == SYS_clone) return 1;
#endif
#ifdef SYS_wait4
    if (nr == SYS_wait4) return 1;
#endif
#ifdef SYS_exit_group
    if (nr == SYS_exit_group) return 1;
#endif
#ifdef SYS_nanosleep
    if (nr == SYS_nanosleep) return 1;
#endif
#ifdef SYS_clock_nanosleep
    if (nr == SYS_clock_nanosleep) return 1;
#endif
#ifdef SYS_openat
    if (nr == SYS_openat) return 1;
#endif
#ifdef SYS_fork
    if (nr == SYS_fork || nr == SYS_vfork) return 1;
#endif
    return 0;
}

static void ring_push(Event *buf, int cap, int *head, int *n, const Event *e)
{
    buf[*head] = *e;
    *head = (*head + 1) % cap;
    if (*n < cap) (*n)++;
}

/* ---------- JSON snapshot for the dashboard ---------- */

static void jstr(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') fprintf(f, "\\%c", *s);
        else if ((unsigned char)*s < 32) fputc('?', f);
        else fputc(*s, f);
    }
    fputc('"', f);
}

static void jevents(FILE *f, const Event *buf, int cap, int head, int n)
{
    char fb[32], ts[40];
    fputc('[', f);
    for (int i = 0; i < n; i++) {
        const Event *e = &buf[(head - n + i + cap * 2) % cap];
        logger_timestamp(ts, sizeof ts, e->ms);
        fprintf(f, "%s{\"seq\":%lu,\"time\":\"%s\",\"pid\":%d,\"name\":", i ? "," : "", e->seq, ts + 11, e->pid);
        jstr(f, syscall_name(e->nr, fb, sizeof fb));
        fprintf(f, ",\"nr\":%ld,\"ret\":%ld,\"has_ret\":%d,\"fail\":%d,\"category\":\"%s\",\"detail\":", e->nr, e->ret,
                e->has_ret, e->fail, category_name(syscall_category(e->nr)));
        jstr(f, e->detail);
        fputc('}', f);
    }
    fputc(']', f);
}

static void write_snapshot(Tracer *t)
{
    if (!t->opt.json_path) return;
    char tmp[600], fb[32];
    snprintf(tmp, sizeof tmp, "%s.tmp", t->opt.json_path);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    long now = logger_now_ms();
    long top = stats_top(&t->stats);

    fprintf(f, "{\"schema\":1,\"status\":\"%s\",\"updated_ms\":%ld,\"started_ms\":%ld,\"arch\":\"%s\",\"tracer_pid\":%d,\"root_pid\":%d,",
            t->status, now, t->start_ms, arch_name(), getpid(), t->root);
    fprintf(f, "\"command\":"); jstr(f, t->command);
    fprintf(f, ",\"log_file\":"); jstr(f, t->log.path);
    fprintf(f, ",\"exit_code\":%d,\"exit_signal\":%d,\"total\":%lu,\"unique\":%lu,\"rate\":%.1f,", t->exit_code, t->exit_signal,
            t->stats.total, stats_unique(&t->stats), t->rate);
    fprintf(f, "\"top\":{\"name\":\"%s\",\"count\":%lu},", top >= 0 ? syscall_name(top, fb, sizeof fb) : "-", top >= 0 ? t->stats.counts[top] : 0);
    fprintf(f, "\"categories\":{\"file\":%lu,\"process\":%lu,\"memory\":%lu,\"io\":%lu,\"other\":%lu},",
            t->stats.by_cat[CAT_FILE], t->stats.by_cat[CAT_PROCESS], t->stats.by_cat[CAT_MEMORY], t->stats.by_cat[CAT_IO], t->stats.by_cat[CAT_OTHER]);
    fprintf(f, "\"counts\":[");
    int first = 1;
    for (int i = 0; i < MAX_SYSCALL_NR; i++) if (t->stats.counts[i]) {
        fprintf(f, "%s{\"name\":\"%s\",\"nr\":%d,\"count\":%lu,\"category\":\"%s\"}", first ? "" : ",",
                syscall_name(i, fb, sizeof fb), i, t->stats.counts[i], category_name(syscall_category(i)));
        first = 0;
    }
    fprintf(f, "],\"events\":"); jevents(f, t->stream, STREAM_CAP, t->s_head, t->s_n);
    fprintf(f, ",\"milestones\":"); jevents(f, t->miles, MILE_CAP, t->m_head, t->m_n);
    if (t->have_cur) {
        const ProcInfo *p = &t->cur;
        fprintf(f, ",\"process\":{\"pid\":%d,\"ppid\":%d,\"name\":", p->pid, p->ppid); jstr(f, p->name);
        fprintf(f, ",\"parent_name\":"); jstr(f, p->parent_name);
        fprintf(f, ",\"cmdline\":"); jstr(f, p->cmdline);
        fprintf(f, ",\"state\":\"%c\",\"threads\":%d,\"vsize_kb\":%lu,\"rss_kb\":%lu,\"mem_percent\":%.2f,\"vol_ctxt\":%lu,\"nonvol_ctxt\":%lu,\"start\":%ld}",
                p->state, p->threads, p->vsize_kb, p->rss_kb, p->mem_percent, p->vol_ctxt, p->nonvol_ctxt, (long)p->start_time);
    } else fprintf(f, ",\"process\":null");
    fprintf(f, ",\"history\":[");
    for (int i = 0; i < t->h_n; i++) {
        const Sample *s = &t->hist[(t->h_head - t->h_n + i + HIST_CAP * 2) % HIST_CAP];
        fprintf(f, "%s{\"t\":%ld,\"cpu\":%.1f,\"rss_kb\":%lu,\"vsize_kb\":%lu}", i ? "," : "", s->ms, s->cpu, s->rss_kb, s->vsize_kb);
    }
    fprintf(f, "],\"threads\":[");
    for (size_t i = 0; i < t->nth; i++) {
        char name[32] = "";
        if (t->th[i].alive) read_comm(t->th[i].pid, t->th[i].comm, sizeof t->th[i].comm);
        snprintf(name, sizeof name, "%s", t->th[i].comm);
        fprintf(f, "%s{\"pid\":%d,\"parent\":%d,\"alive\":%d,\"name\":", i ? "," : "", t->th[i].pid, t->th[i].parent, t->th[i].alive);
        jstr(f, name);
        fputc('}', f);
    }
    fprintf(f, "]}\n");
    fclose(f);
    rename(tmp, t->opt.json_path);
}

static void periodic(Tracer *t)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (t->root > 0 && t->status[0] == 'r') {
        ProcInfo p;
        if (pm_read(t->root, &p) == 0) {
            double dt = t->have_prev ? (now.tv_sec - t->prev_ts.tv_sec) + (now.tv_nsec - t->prev_ts.tv_nsec) / 1e9 : 0;
            double cpu = t->have_prev ? pm_cpu_percent(&t->prev, &p, dt) : 0;
            t->cur = p; t->have_cur = 1; t->prev = p; t->prev_ts = now; t->have_prev = 1;
            t->hist[t->h_head] = (Sample){ logger_now_ms(), cpu, p.rss_kb, p.vsize_kb };
            t->h_head = (t->h_head + 1) % HIST_CAP;
            if (t->h_n < HIST_CAP) t->h_n++;
        }
    }
    long ms = logger_now_ms();
    if (t->last_snap_ms) t->rate = (double)(t->stats.total - t->last_total) * 1000.0 / (double)(ms - t->last_snap_ms + 1);
    t->last_snap_ms = ms; t->last_total = t->stats.total;
    write_snapshot(t);
}

static void handle_syscall_stop(Tracer *t, Thread *th)
{
    struct sc_info si;
    char fb[32], ts[40];
    memset(&si, 0, sizeof si);
    if (ptrace(PTRACE_GET_SYSCALL_INFO, th->pid, (void *)sizeof si, &si) < 0) {
        if (errno != ESRCH) fprintf(stderr, "warning: PTRACE_GET_SYSCALL_INFO failed for %d: %s (kernel >= 5.3 required)\n", th->pid, strerror(errno));
        return;
    }
    if (!t->arch_warned) {
        uint32_t want = 
#if defined(__x86_64__)
            AUDIT_ARCH_X86_64_;
#else
            AUDIT_ARCH_AARCH64_;
#endif
        if (si.arch != want) {
            fprintf(stderr, "warning: tracee ABI (0x%x) differs from tracer ABI (%s, 0x%x): names may be wrong\n", si.arch, arch_name(), want);
        }
        t->arch_warned = 1;
    }
    long ms = logger_now_ms();
    logger_timestamp(ts, sizeof ts, ms);

    if (si.op == SC_OP_ENTRY) {
        th->in_call = 1;
        th->nr = (long)si.entry.nr;
        memcpy(th->args, si.entry.args, sizeof th->args);
        describe(th->pid, th->nr, th->args, th->detail, sizeof th->detail);
        stats_add(&t->stats, th->nr);
        logger_printf(&t->log, "%s|%d|%s|%ld|ENTER|-|-|%s\n", ts, th->pid, syscall_name(th->nr, fb, sizeof fb), th->nr, th->detail);
#ifdef SYS_exit_group
        if (th->nr == SYS_exit_group) {
            Event e = { ms, th->pid, th->nr, 0, 0, 0, ++t->seq, "" };
            snprintf(e.detail, sizeof e.detail, "%s", th->detail);
            ring_push(t->stream, STREAM_CAP, &t->s_head, &t->s_n, &e);
            ring_push(t->miles, MILE_CAP, &t->m_head, &t->m_n, &e);
            if (!t->opt.quiet) fprintf(stderr, "[%d] %s(%s) = ?\n", th->pid, syscall_name(th->nr, fb, sizeof fb), th->detail);
        }
#endif
#ifdef SYS_exit
        if (th->nr == SYS_exit) {
            Event e = { ms, th->pid, th->nr, 0, 0, 0, ++t->seq, "" };
            snprintf(e.detail, sizeof e.detail, "%s", th->detail);
            ring_push(t->stream, STREAM_CAP, &t->s_head, &t->s_n, &e);
            ring_push(t->miles, MILE_CAP, &t->m_head, &t->m_n, &e);
            if (!t->opt.quiet) fprintf(stderr, "[%d] %s(%s) = ?\n", th->pid, syscall_name(th->nr, fb, sizeof fb), th->detail);
        }
#endif
    } else if (si.op == SC_OP_EXIT) {
        long ret = (long)si.exit.rval;
        int fail = si.exit.is_error != 0;
        th->in_call = 0;
        char state = proc_state(th->pid);
        logger_printf(&t->log, "%s|%d|%s|%ld|EXIT|%ld|%c|%s\n", ts, th->pid, syscall_name(th->nr, fb, sizeof fb), th->nr, ret, state, th->detail);
        Event e = { ms, th->pid, th->nr, ret, fail, 1, ++t->seq, "" };
        snprintf(e.detail, sizeof e.detail, "%s", th->detail);
        ring_push(t->stream, STREAM_CAP, &t->s_head, &t->s_n, &e);
        if (is_milestone(th->nr)) ring_push(t->miles, MILE_CAP, &t->m_head, &t->m_n, &e);
        if (!t->opt.quiet) fprintf(stderr, "[%d] %s(%s) = %ld%s\n", th->pid, syscall_name(th->nr, fb, sizeof fb), th->detail, ret, fail ? " (error)" : "");
    }
}

#define PTRACE_OPTS (PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL)

static void detach_all(Tracer *t)
{
    for (size_t i = 0; i < t->nth; i++)
        if (t->th[i].alive) ptrace(PTRACE_DETACH, t->th[i].pid, NULL, NULL);
}

static void trace_loop(Tracer *t, int attached)
{
    int status, stopping = 0;
    while (t->live > 0) {
        if (g_tick) { g_tick = 0; periodic(t); }
        if (g_stop && !stopping) {
            stopping = 1;
            fprintf(stderr, "\n[tracer] interrupted - %s\n", attached ? "detaching from tracee" : "terminating tracee");
            if (attached) { detach_all(t); t->status = "detached"; return; }
            kill(t->root, SIGTERM);
        }
        pid_t pid = waitpid(-1, &status, __WALL);
        if (pid < 0) {
            if (errno == EINTR) continue;
            if (errno != ECHILD) perror("waitpid");
            break;
        }
        Thread *th = thread_get(t, pid, 1);
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            th->alive = 0; t->live--;
            if (pid == t->root) {
                t->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                t->exit_signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
            }
            continue;
        }
        if (!WIFSTOPPED(status)) continue;

        int sig = WSTOPSIG(status), event = status >> 16, inject = 0;
        if (sig == (SIGTRAP | 0x80)) {
            handle_syscall_stop(t, th);
        } else if (sig == SIGTRAP && event) {
            unsigned long msg = 0;
            if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK || event == PTRACE_EVENT_CLONE) {
                ptrace(PTRACE_GETEVENTMSG, pid, NULL, &msg);
                Thread *c = thread_get(t, (pid_t)msg, 1);
                c->parent = pid;
            } else if (event == PTRACE_EVENT_EXEC) {
                read_comm(pid, th->comm, sizeof th->comm);
            }
        } else if (sig == SIGSTOP && !th->comm[0] && pid != t->root) {
            read_comm(pid, th->comm, sizeof th->comm);
        } else if (sig != SIGTRAP) {
            inject = sig;
        }
        if (ptrace(PTRACE_SYSCALL, pid, NULL, (void *)(long)inject) < 0 && errno != ESRCH)
            fprintf(stderr, "warning: PTRACE_SYSCALL(%d): %s\n", pid, strerror(errno));
    }
}

static void install_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_int;  sigaction(SIGINT, &sa, NULL); sigaction(SIGTERM, &sa, NULL);
    sa.sa_handler = on_tick; sigaction(SIGALRM, &sa, NULL);
    struct itimerval it = { {0, 250000}, {0, 250000} };
    setitimer(ITIMER_REAL, &it, NULL);
}

static int start_session(Tracer *t, const TracerOptions *opt)
{
    memset(t, 0, sizeof *t);
    t->opt = *opt;
    stats_init(&t->stats);
    t->status = "running";
    t->start_ms = logger_now_ms();
    g_stop = g_tick = 0;
    if (logger_open(&t->log, opt->log_dir ? opt->log_dir : "logs") < 0) return -1;
    return 0;
}

static int finish_session(Tracer *t, int attached)
{
    struct itimerval off = {{0, 0}, {0, 0}};
    setitimer(ITIMER_REAL, &off, NULL);
    if (strcmp(t->status, "detached") != 0) t->status = "finished";
    write_snapshot(t);
    logger_printf(&t->log, "# END total=%lu unique=%lu exit_code=%d signal=%d\n", t->stats.total, stats_unique(&t->stats), t->exit_code, t->exit_signal);
    stats_print(&t->stats, stdout);
    printf("Log file: %s\n", t->log.path);
    if (t->exit_signal) printf("Tracee killed by signal %d (%s)\n", t->exit_signal, strsignal(t->exit_signal));
    else if (!attached) printf("Tracee exited with code %d\n", t->exit_code);
    int rc = attached ? 0 : t->exit_code;
    logger_close(&t->log);
    free(t->th);
    return rc;
}

static void header(Tracer *t, const char *how)
{
    char ts[40];
    logger_timestamp(ts, sizeof ts, t->start_ms);
    logger_printf(&t->log, "# syscall-tracer log | %s | arch=%s | started %s | root_pid=%d\n", how, arch_name(), ts, t->root);
    logger_printf(&t->log, "# TIMESTAMP|PID|SYSCALL|NR|PHASE|RETURN|STATE|DETAIL\n");
}

int tracer_run(char *const argv[], const TracerOptions *opt)
{
    static Tracer T;
    Tracer *t = &T;
    if (start_session(t, opt) < 0) return -1;

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) { perror("PTRACE_TRACEME"); _exit(126); }
        raise(SIGSTOP);
        execvp(argv[0], argv);
        fprintf(stderr, "error: cannot execute '%s': %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    int status;
    if (waitpid(pid, &status, 0) < 0 || !WIFSTOPPED(status)) {
        fprintf(stderr, "error: tracee did not stop as expected\n");
        return -1;
    }
    if (ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)(long)PTRACE_OPTS) < 0) {
        fprintf(stderr, "error: PTRACE_SETOPTIONS: %s\n", strerror(errno));
        kill(pid, SIGKILL);
        return -1;
    }
    t->root = pid; t->live = 0;
    thread_get(t, pid, 1)->parent = getpid();
    snprintf(t->command, sizeof t->command, "%s", argv[0]);
    for (int i = 1; argv[i] && strlen(t->command) < 200; i++) { strcat(t->command, " "); strncat(t->command, argv[i], 40); }
    header(t, "run");
    fprintf(stderr, "[tracer] tracing PID %d (%s) on %s, log: %s\n", pid, t->command, arch_name(), t->log.path);
    install_handlers();
    periodic(t);
    ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    trace_loop(t, 0);
    return finish_session(t, 0);
}

int tracer_attach(pid_t pid, const TracerOptions *opt)
{
    static Tracer T;
    Tracer *t = &T;
    ProcInfo pi;

    if (kill(pid, 0) < 0 && errno == ESRCH) { fprintf(stderr, "error: process %d does not exist\n", pid); return -1; }
    if (pm_read(pid, &pi) < 0) { fprintf(stderr, "error: cannot read /proc/%d: %s\n", pid, strerror(errno)); return -1; }
    if (start_session(t, opt) < 0) return -1;

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        fprintf(stderr, "error: PTRACE_ATTACH to %d failed: %s\n", pid, strerror(errno));
        if (errno == EPERM)
            fprintf(stderr, "  Linux only lets you trace processes you own, and the Yama LSM may restrict tracing.\n");
        logger_close(&t->log);
        return -1;
    }
    int status;
    if (waitpid(pid, &status, __WALL) < 0 || !WIFSTOPPED(status)) { fprintf(stderr, "error: process %d vanished during attach\n", pid); return -1; }
    if (ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)(long)(PTRACE_OPTS & ~PTRACE_O_EXITKILL)) < 0) {
        fprintf(stderr, "error: PTRACE_SETOPTIONS: %s\n", strerror(errno));
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    t->root = pid;
    thread_get(t, pid, 1)->parent = pi.ppid;
    snprintf(t->command, sizeof t->command, "%s", pi.cmdline);
    header(t, "attach");
    fprintf(stderr, "[tracer] attached to PID %d (%s) on %s, log: %s (Ctrl-C to detach)\n", pid, pi.name, arch_name(), t->log.path);
    install_handlers();
    periodic(t);
    ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    trace_loop(t, 1);
    return finish_session(t, 1);
}

int tracer_stats_from_log(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "error: cannot open log '%s': %s\n", path, strerror(errno)); return -1; }
    static SyscallStats st;
    char line[1024];
    unsigned long bad = 0, lineno = 0;
    stats_init(&st);
    while (fgets(line, sizeof line, f)) {
        lineno++;
        if (line[0] == '#' || line[0] == '\n') continue;
        char *save = NULL, *fld[6] = {0};
        char *tok = strtok_r(line, "|", &save);
        for (int i = 0; i < 6 && tok; i++, tok = strtok_r(NULL, "|", &save)) fld[i] = tok;
        char *end = NULL;
        long nr = fld[3] ? strtol(fld[3], &end, 10) : -1;
        if (!fld[4] || !fld[3] || *end != '\0' || nr < 0 || nr >= MAX_SYSCALL_NR) { bad++; continue; }
        if (strcmp(fld[4], "ENTER") == 0) stats_add(&st, nr);
    }
    fclose(f);
    printf("Statistics for %s\n", path);
    stats_print(&st, stdout);
    if (bad) fprintf(stderr, "warning: skipped %lu malformed line(s)\n", bad);
    if (st.total == 0) fprintf(stderr, "warning: no ENTER records found\n");
    return 0;
}
