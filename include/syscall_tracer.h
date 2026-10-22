/* syscall_tracer.h - ptrace()-based system call tracer */
#ifndef SYSCALL_TRACER_H
#define SYSCALL_TRACER_H
#include <sys/types.h>

typedef struct {
    const char *log_dir;    /* where trace_*.log is written (created if missing) */
    const char *json_path;  /* live snapshot for the dashboard, or NULL */
    int         quiet;      /* 1 = do not print each syscall to stderr */
} TracerOptions;

/* Launch argv[] under ptrace and trace it (and all children). Returns tracee exit code, or -1. */
int tracer_run(char *const argv[], const TracerOptions *opt);
/* Attach to a running process (needs ptrace permission on it). */
int tracer_attach(pid_t pid, const TracerOptions *opt);
/* Rebuild and print statistics from an existing log file. */
int tracer_stats_from_log(const char *path);
#endif
