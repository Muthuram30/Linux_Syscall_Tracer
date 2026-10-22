/* process_monitor.h - reads per-process information from the /proc pseudo filesystem */
#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H
#include <sys/types.h>
#include <time.h>

typedef struct {
    int  pid, ppid, threads;
    char name[64], parent_name[64], cmdline[256];
    char state;                      /* R S D Z T t X ... */
    unsigned long long utime, stime; /* clock ticks (user / kernel mode) */
    unsigned long vsize_kb, rss_kb;  /* VmSize / VmRSS */
    unsigned long vol_ctxt, nonvol_ctxt;
    time_t start_time;
    double mem_percent;              /* RSS / MemTotal */
} ProcInfo;

/* 0 on success; -1 with errno set (ENOENT = no such process). */
int    pm_read(int pid, ProcInfo *pi);
double pm_cpu_percent(const ProcInfo *prev, const ProcInfo *cur, double dt_seconds);
int    pm_children(int pid, int *out, int max);
const char *pm_state_desc(char s);
void   pm_print(const ProcInfo *pi, double cpu_percent);
/* Refresh every `interval` seconds until the process exits or Ctrl-C. */
int    pm_monitor(int pid, int interval);
/* fork()+exec() a program (no tracing) and monitor it. */
int    pm_monitor_run(char *const argv[], int interval);
#endif
