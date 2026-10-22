/* logger.h - timestamped trace-log files (logs/trace_YYYYMMDD_HHMMSS.log) */
#ifndef LOGGER_H
#define LOGGER_H
#include <stdio.h>
#include <stddef.h>

typedef struct {
    FILE *fp;
    char  path[512];
} Logger;

/* Creates `dir` if missing and opens a new timestamped log. 0 = ok, -1 = error (message printed). */
int  logger_open(Logger *lg, const char *dir);
void logger_close(Logger *lg);
void logger_printf(Logger *lg, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
/* "YYYY-MM-DD HH:MM:SS.mmm" wall-clock time */
void logger_timestamp(char *buf, size_t n, long epoch_ms);
long logger_now_ms(void);
#endif
