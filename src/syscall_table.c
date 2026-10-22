/* syscall_table.c - syscall names, categories and statistics.
 *
 * Numbers come from <sys/syscall.h> (SYS_xxx), so the table always matches the
 * architecture the tool was compiled for (x86_64 or aarch64).  Syscalls that do
 * not exist on the architecture (e.g. fork/open on aarch64) are skipped by #ifdef. */
#define _GNU_SOURCE
#include "syscall_table.h"
#include <stdlib.h>
#include <sys/syscall.h>

typedef struct { long nr; const char *name; SyscallCategory cat; } Entry;

#define SC(n, c) { SYS_##n, #n, c },
static const Entry table[] = {
#include "syscall_list.inc"
    { -1, NULL, CAT_OTHER }
};
#undef SC

static const Entry *lut[MAX_SYSCALL_NR];
static int lut_ready;

static void lut_init(void)
{
    for (const Entry *e = table; e->name; e++)
        if (e->nr >= 0 && e->nr < MAX_SYSCALL_NR) lut[e->nr] = e;
    lut_ready = 1;
}

const char *arch_name(void)
{
#if defined(__x86_64__)
    return "x86_64";
#elif defined(__aarch64__)
    return "aarch64";
#else
    return "unsupported";
#endif
}

const char *syscall_name(long nr, char *fallback, size_t n)
{
    if (!lut_ready) lut_init();
    if (nr >= 0 && nr < MAX_SYSCALL_NR && lut[nr]) return lut[nr]->name;
    snprintf(fallback, n, "syscall_%ld", nr);
    return fallback;
}

SyscallCategory syscall_category(long nr)
{
    if (!lut_ready) lut_init();
    return (nr >= 0 && nr < MAX_SYSCALL_NR && lut[nr]) ? lut[nr]->cat : CAT_OTHER;
}

const char *category_name(SyscallCategory c)
{
    static const char *n[] = { "other", "file", "process", "memory", "io" };
    return (c >= 0 && c < CAT_COUNT) ? n[c] : "other";
}

void stats_init(SyscallStats *s) { *s = (SyscallStats){0}; }

void stats_add(SyscallStats *s, long nr)
{
    s->total++;
    if (nr >= 0 && nr < MAX_SYSCALL_NR) s->counts[nr]++;
    s->by_cat[syscall_category(nr)]++;
}

unsigned long stats_unique(const SyscallStats *s)
{
    unsigned long u = 0;
    for (int i = 0; i < MAX_SYSCALL_NR; i++) if (s->counts[i]) u++;
    return u;
}

long stats_top(const SyscallStats *s)
{
    long best = -1;
    for (int i = 0; i < MAX_SYSCALL_NR; i++)
        if (s->counts[i] && (best < 0 || s->counts[i] > s->counts[best])) best = i;
    return best;
}

typedef struct { int nr; unsigned long count; } Pair;
static int cmp_pair(const void *a, const void *b)
{
    const Pair *x = a, *y = b;
    return (y->count > x->count) - (y->count < x->count);
}

void stats_print(const SyscallStats *s, FILE *out)
{
    Pair p[MAX_SYSCALL_NR];
    int np = 0;
    char fb[32];
    long top = stats_top(s);
    double t = s->total ? (double)s->total : 1.0;

    for (int i = 0; i < MAX_SYSCALL_NR; i++)
        if (s->counts[i]) p[np++] = (Pair){ i, s->counts[i] };
    qsort(p, np, sizeof p[0], cmp_pair);

    fprintf(out, "\n=========== System Call Statistics (%s) ===========\n", arch_name());
    fprintf(out, "Total syscalls  : %lu\n", s->total);
    fprintf(out, "Unique syscalls : %lu\n", stats_unique(s));
    if (top >= 0)
        fprintf(out, "Most frequent   : %s() x%lu (%.1f%%)\n", syscall_name(top, fb, sizeof fb),
                s->counts[top], 100.0 * s->counts[top] / t);
    fprintf(out, "\nBy category:\n");
    for (int c = CAT_FILE; c < CAT_COUNT + 1; c++) {
        int cc = c % CAT_COUNT;   /* file, process, memory, io, other */
        fprintf(out, "  %-8s %6lu  (%5.1f%%)\n", category_name(cc), s->by_cat[cc], 100.0 * s->by_cat[cc] / t);
    }
    fprintf(out, "\nPer syscall:\n");
    for (int i = 0; i < np; i++)
        fprintf(out, "  %-18s %6lu  %5.1f%%\n", syscall_name(p[i].nr, fb, sizeof fb), p[i].count,
                100.0 * p[i].count / t);
    fprintf(out, "===================================================\n");
}
