/* main.c - command-line interface */
#include "syscall_tracer.h"
#include "process_monitor.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *p)
{
    printf("Linux System Call Tracer & Process Monitor (educational)\n\n"
           "Usage: %s [options] <command>\n\n"
           "Commands:\n"
           "  --help                     show this help\n"
           "  --monitor <PID>            show /proc information for a process (refreshing)\n"
           "  --monitor-run <prog> [args] launch a program and monitor it\n"
           "  --trace <PID>              attach with ptrace and trace system calls\n"
           "  --run <prog> [args...]     launch a program under the tracer (recommended)\n"
           "  --stats <logfile>          statistics from an existing trace log\n\n"
           "Options:\n"
           "  --interval <sec>           refresh interval for --monitor (default 1)\n"
           "  --log-dir <dir>            log directory (default: logs)\n"
           "  --json <file>              live snapshot for the dashboard (default: logs/latest.json)\n"
           "  --quiet                    do not print every syscall to stderr\n\n"
           "Examples:\n"
           "  %s --run ./build/test_program\n"
           "  %s --monitor $$\n"
           "  %s --stats logs/trace_20250101_120000.log\n", p, p, p, p);
}

static int parse_pid(const char *s)
{
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || *end || v <= 0 || v > 4194304) { fprintf(stderr, "error: invalid PID '%s'\n", s); return -1; }
    return (int)v;
}

int main(int argc, char **argv)
{
    TracerOptions opt = { "logs", "logs/latest.json", 0 };
    int interval = 1;
    if (argc < 2) { usage(argv[0]); return 1; }

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int has_arg = i + 1 < argc;
        if (!strcmp(a, "--help") || !strcmp(a, "-h")) { usage(argv[0]); return 0; }
        else if (!strcmp(a, "--quiet")) opt.quiet = 1;
        else if (!strcmp(a, "--log-dir") || !strcmp(a, "--json") || !strcmp(a, "--interval") ||
                 !strcmp(a, "--monitor") || !strcmp(a, "--trace") || !strcmp(a, "--stats")) {
            if (!has_arg) { fprintf(stderr, "error: %s needs an argument\n", a); return 1; }
            const char *v = argv[++i];
            if (!strcmp(a, "--log-dir")) opt.log_dir = v;
            else if (!strcmp(a, "--json")) opt.json_path = v;
            else if (!strcmp(a, "--interval")) { interval = atoi(v); if (interval < 1) { fprintf(stderr, "error: invalid interval\n"); return 1; } }
            else if (!strcmp(a, "--stats")) return tracer_stats_from_log(v) < 0 ? 1 : 0;
            else {
                int pid = parse_pid(v);
                if (pid < 0) return 1;
                if (!strcmp(a, "--monitor")) return pm_monitor(pid, interval) < 0 ? 1 : 0;
                return tracer_attach(pid, &opt) < 0 ? 1 : 0;
            }
        } else if (!strcmp(a, "--run") || !strcmp(a, "--monitor-run")) {
            if (!has_arg) { fprintf(stderr, "error: %s needs a program\n", a); return 1; }
            char **prog = &argv[i + 1];            /* everything after belongs to the program */
            if (!strcmp(a, "--run")) { int rc = tracer_run(prog, &opt); return rc < 0 ? 1 : rc; }
            return pm_monitor_run(prog, interval) < 0 ? 1 : 0;
        } else { fprintf(stderr, "error: unknown argument '%s' (try --help)\n", a); return 1; }
    }
    fprintf(stderr, "error: no command given (try --help)\n");
    return 1;
}
