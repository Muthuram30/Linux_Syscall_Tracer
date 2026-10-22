/* logger.c - writes timestamped trace logs into logs/ */
#include "logger.h"
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

long logger_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

void logger_timestamp(char *buf, size_t n, long epoch_ms)
{
    time_t s = (time_t)(epoch_ms / 1000);
    struct tm tm;
    char base[32];
    localtime_r(&s, &tm);
    strftime(base, sizeof base, "%Y-%m-%d %H:%M:%S", &tm);
    snprintf(buf, n, "%s.%03ld", base, epoch_ms % 1000);
}

int logger_open(Logger *lg, const char *dir)
{
    char stamp[32];
    time_t now = time(NULL);
    struct tm tm;

    /* A missing log directory is created instead of failing. */
    if (mkdir(dir, 0755) < 0 && errno != EEXIST) {
        fprintf(stderr, "error: cannot create log directory '%s': %s\n", dir, strerror(errno));
        return -1;
    }
    localtime_r(&now, &tm);
    strftime(stamp, sizeof stamp, "%Y%m%d_%H%M%S", &tm);
    snprintf(lg->path, sizeof lg->path, "%s/trace_%s.log", dir, stamp);
    lg->fp = fopen(lg->path, "w");
    if (!lg->fp) {
        fprintf(stderr, "error: cannot open log file '%s': %s\n", lg->path, strerror(errno));
        return -1;
    }
    return 0;
}

void logger_close(Logger *lg)
{
    if (lg->fp) { fclose(lg->fp); lg->fp = NULL; }
}

void logger_printf(Logger *lg, const char *fmt, ...)
{
    va_list ap;
    if (!lg->fp) return;
    va_start(ap, fmt);
    vfprintf(lg->fp, fmt, ap);
    va_end(ap);
}
