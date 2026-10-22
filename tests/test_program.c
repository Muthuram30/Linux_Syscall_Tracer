/* test_program.c - small program that exercises the system calls we trace.
 *
 *   parent
 *   ├── open/write/close, open/read/close   (file I/O)
 *   ├── malloc of a big block               (brk / mmap)
 *   ├── fork() ── child: exec("/bin/echo")  (clone, execve)
 *   ├── wait()                              (wait4)
 *   ├── sleep()                             (nanosleep/clock_nanosleep)
 *   └── exit()                              (exit_group)
 *
 * Usage: test_program [all|io|fork|exec|wait|mem|sleep|exit]   (default: all) */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void say(const char *s) { write(STDOUT_FILENO, s, strlen(s)); }

static void do_io(void)
{
    const char *path = "/tmp/syscall_tracer_demo.txt";
    char buf[128];
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }
    write(fd, "hello from the test program\n", 28);
    close(fd);
    fd = open(path, O_RDONLY);
    ssize_t n = read(fd, buf, sizeof buf - 1);
    if (n > 0) { buf[n] = 0; say("read back: "); say(buf); }
    close(fd);
    unlink(path);
    open("/nonexistent/file", O_RDONLY);  /* fails: ENOENT */
}

static void do_mem(void)
{
    size_t sz = 8 * 1024 * 1024;
    char *p = malloc(sz);
    if (!p) return;
    memset(p, 0x5a, sz);
    free(p);
    void *q = malloc(64);
    free(q);
}

static void do_fork_exec_wait(int do_exec, int do_wait)
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }
    if (pid == 0) {
        if (do_exec) {
            execl("/bin/echo", "echo", "hello from exec()'d child", (char *)NULL);
            _exit(127);
        }
        say("child running\n");
        _exit(7);
    }
    if (do_wait) {
        int st;
        waitpid(pid, &st, 0);
        char m[64];
        snprintf(m, sizeof m, "child %d finished, status %d\n", pid, WIFEXITED(st) ? WEXITSTATUS(st) : -1);
        say(m);
    }
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "all";
    int all = !strcmp(mode, "all");
    struct timespec ts;

    say("== test_program started ==\n");
    syscall(SYS_getpid);
    syscall(SYS_getppid);
    syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts);

    if (all || !strcmp(mode, "io"))    do_io();
    if (all || !strcmp(mode, "mem"))   do_mem();
    if (all || !strcmp(mode, "fork"))  do_fork_exec_wait(0, 1);
    if (all || !strcmp(mode, "exec"))  do_fork_exec_wait(1, 1);
    if (!strcmp(mode, "wait"))         do_fork_exec_wait(0, 1);
    if (all || !strcmp(mode, "sleep")) { say("sleeping 2s...\n"); sleep(2); }
    if (!strcmp(mode, "exit"))         { say("exiting with code 3\n"); exit(3); }

    say("== test_program done ==\n");
    return 0;
}
