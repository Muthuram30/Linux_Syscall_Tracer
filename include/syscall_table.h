/* syscall_table.h - syscall number -> name/category, and statistics */
#ifndef SYSCALL_TABLE_H
#define SYSCALL_TABLE_H
#include <stdio.h>
#include <stddef.h>

typedef enum { CAT_OTHER = 0, CAT_FILE, CAT_PROCESS, CAT_MEMORY, CAT_IO, CAT_COUNT } SyscallCategory;
#define MAX_SYSCALL_NR 1024

typedef struct {
    unsigned long counts[MAX_SYSCALL_NR];
    unsigned long total;
    unsigned long by_cat[CAT_COUNT];
} SyscallStats;

/* Name of `nr` on the architecture this binary was built for; "syscall_<nr>" if unknown. */
const char *syscall_name(long nr, char *fallback, size_t n);
SyscallCategory syscall_category(long nr);
const char *category_name(SyscallCategory c);
const char *arch_name(void);

void          stats_init(SyscallStats *s);
void          stats_add(SyscallStats *s, long nr);
unsigned long stats_unique(const SyscallStats *s);
long          stats_top(const SyscallStats *s);            /* nr of most frequent, -1 if none */
void          stats_print(const SyscallStats *s, FILE *out);
#endif
