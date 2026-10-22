/* process_monitor.c - process information from /proc/<pid>/{stat,status,cmdline}.
 *
 * /proc is a virtual filesystem: the kernel generates these "files" on demand from
 * its in-memory task_struct data, so reading them costs no disk I/O. */
#define _GNU_SOURCE
#include "process_monitor.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t stop_flag;
static void on_sigint(int s) { (void)s; stop_flag = 1; }

const char *pm_state_desc(char s)
{
    switch (s) {
    case 'R': return "Running / runnable";
    case 'S': return "Sleeping (interruptible)";
    case 'D': return "Disk sleep (uninterruptible)";
    case 'Z': return "Zombie";
    case 'T': return "Stopped (signal)";
    case 't': return "Tracing stop (ptrace)";
    case 'X': return "Dead";
    case 'I': return "Idle kernel thread";
    default:  return "Unknown";
    }
}

/* Parse /proc/<pid>/stat.  Field 2 (comm) is in parentheses and may contain spaces
 * or ')' so we locate the LAST ')' before parsing the rest. */
static int read_stat(int pid, ProcInfo *pi)
{
    char path[64], buf[2048];
    snprintf(path, sizeof path, "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = '\0';
    char *lp = strchr(buf, '('), *rp = strrchr(buf, ')');
    if (!lp || !rp || rp < lp) { errno = EINVAL; return -1; }   /* malformed data */
    size_t len = (size_t)(rp - lp - 1);
    if (len >= sizeof pi->name) len = sizeof pi->name - 1;
    memcpy(pi->name, lp + 1, len);
    pi->name[len] = '\0';
    long threads = 0; unsigned long long start = 0;
    int got = sscanf(rp + 2, "%c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu %*d %*d %*d %*d %ld %*d %llu",
                     &pi->state, &pi->ppid, &pi->utime, &pi->stime, &threads, &start);
    if (got != 6) { errno = EINVAL; return -1; }
    pi->threads = (int)threads;
    long hz = sysconf(_SC_CLK_TCK);
    long btime = 0;
    FILE *fs = fopen("/proc/stat", "r");
    if (fs) {
        char line[256];
        while (fgets(line, sizeof line, fs)) if (sscanf(line, "btime %ld", &btime) == 1) break;
        fclose(fs);
    }
    pi->start_time = (time_t)(btime + (hz > 0 ? (long)(start / (unsigned long long)hz) : 0));
    return 0;
}

static void read_status(int pid, ProcInfo *pi)
{
    char path[64], line[256];
    snprintf(path, sizeof path, "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    pi->vsize_kb = pi->rss_kb = pi->vol_ctxt = pi->nonvol_ctxt = 0;
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        sscanf(line, "VmSize: %lu", &pi->vsize_kb);
        sscanf(line, "VmRSS: %lu", &pi->rss_kb);
        sscanf(line, "voluntary_ctxt_switches: %lu", &pi->vol_ctxt);
        sscanf(line, "nonvoluntary_ctxt_switches: %lu", &pi->nonvol_ctxt);
    }
    fclose(f);
}

int pm_read(int pid, ProcInfo *pi)
{
    memset(pi, 0, sizeof *pi);
    pi->pid = pid;
    if (pid <= 0) { errno = EINVAL; return -1; }
    if (read_stat(pid, pi) < 0) return -1;
    read_status(pid, pi);

    char path[64];
    snprintf(path, sizeof path, "/proc/%d/cmdline", pid);
    FILE *f = fopen(path, "r");
    if (f) {   /* arguments are separated by NUL bytes */
        size_t n = fread(pi->cmdline, 1, sizeof pi->cmdline - 1, f);
        fclose(f);
        for (size_t i = 0; i + 1 < n; i++) if (pi->cmdline[i] == '\0') pi->cmdline[i] = ' ';
        pi->cmdline[n] = '\0';
    }
    if (pi->cmdline[0] == '\0') snprintf(pi->cmdline, sizeof pi->cmdline, "[%s]", pi->name);

    /* parent's name */
    ProcInfo par = {0};
    if (pi->ppid > 0 && read_stat(pi->ppid, &par) == 0) snprintf(pi->parent_name, sizeof pi->parent_name, "%s", par.name);
    else snprintf(pi->parent_name, sizeof pi->parent_name, "-");

    unsigned long mem_total = 0;
    FILE *fm = fopen("/proc/meminfo", "r");
    if (fm) { char l[128]; while (fgets(l, sizeof l, fm)) if (sscanf(l, "MemTotal: %lu", &mem_total) == 1) break; fclose(fm); }
    pi->mem_percent = mem_total ? 100.0 * (double)pi->rss_kb / (double)mem_total : 0.0;
    return 0;
}

double pm_cpu_percent(const ProcInfo *prev, const ProcInfo *cur, double dt)
{
    long hz = sysconf(_SC_CLK_TCK);
    if (!prev || dt <= 0 || hz <= 0) return 0.0;
    double ticks = (double)((cur->utime + cur->stime) - (prev->utime + prev->stime));
    return 100.0 * (ticks / (double)hz) / dt;
}

int pm_children(int pid, int *out, int max)
{
    DIR *d = opendir("/proc");
    if (!d) return -1;
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) && n < max) {
        if (!isdigit((unsigned char)e->d_name[0])) continue;
        ProcInfo p;
        if (read_stat(atoi(e->d_name), &p) == 0 && p.ppid == pid) out[n++] = atoi(e->d_name);
    }
    closedir(d);
    return n;
}

void pm_print(const ProcInfo *pi, double cpu)
{
    char when[32] = "?";
    struct tm tm;
    localtime_r(&pi->start_time, &tm);
    strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", &tm);
    int kids[32];
    int nk = pm_children(pi->pid, kids, 32);

    printf("PID              : %d\n", pi->pid);
    printf("PPID             : %d (%s)\n", pi->ppid, pi->parent_name);
    printf("Name             : %s\n", pi->name);
    printf("Command line     : %s\n", pi->cmdline);
    printf("State            : %c - %s\n", pi->state, pm_state_desc(pi->state));
    printf("CPU usage        : %.1f %%  (user %llu + kernel %llu ticks)\n", cpu, pi->utime, pi->stime);
    printf("Memory usage     : %.2f %% of RAM\n", pi->mem_percent);
    printf("Virtual memory   : %lu kB\n", pi->vsize_kb);
    printf("Resident memory  : %lu kB\n", pi->rss_kb);
    printf("Threads          : %d\n", pi->threads);
    printf("Context switches : %lu voluntary, %lu involuntary\n", pi->vol_ctxt, pi->nonvol_ctxt);
    printf("Started at       : %s\n", when);
    printf("Children         :");
    if (nk <= 0) printf(" none");
    for (int i = 0; i < nk; i++) printf(" %d", kids[i]);
    printf("\n");
}

int pm_monitor(int pid, int interval)
{
    ProcInfo prev, cur;
    int have_prev = 0;
    struct sigaction sa = {0};
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);      /* no SA_RESTART: sleep() is interrupted */
    stop_flag = 0;
    if (interval < 1) interval = 1;

    if (pm_read(pid, &cur) < 0) {
        if (errno == ENOENT) fprintf(stderr, "error: process %d does not exist\n", pid);
        else fprintf(stderr, "error: cannot read /proc/%d: %s\n", pid, strerror(errno));
        return -1;
    }
    double cpu = 0;
    while (!stop_flag) {
        if (isatty(STDOUT_FILENO)) printf("\033[H\033[J");
        printf("=== Process Monitor (Ctrl-C to quit, refresh %ds) ===\n", interval);
        pm_print(&cur, cpu);
        fflush(stdout);
        prev = cur; have_prev = 1;
        sleep((unsigned)interval);
        if (stop_flag) break;
        if (pm_read(pid, &cur) < 0) { printf("\nProcess %d has exited.\n", pid); return 0; }
        cpu = have_prev ? pm_cpu_percent(&prev, &cur, interval) : 0;
        if (cur.state == 'Z') { pm_print(&cur, cpu); printf("\nProcess %d is a zombie (exited, waiting to be reaped).\n", pid); return 0; }
    }
    return 0;
}

int pm_monitor_run(char *const argv[], int interval)
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "error: cannot execute '%s': %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    printf("Launched '%s' as PID %d\n", argv[0], pid);
    usleep(100000);
    int rc = pm_monitor(pid, interval);
    int st;
    if (waitpid(pid, &st, WNOHANG) == 0) { kill(pid, SIGTERM); waitpid(pid, &st, 0); }
    return rc;
}
