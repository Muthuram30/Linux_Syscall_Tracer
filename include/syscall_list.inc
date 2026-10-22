#ifdef SYS_read
SC(read, CAT_IO)
#endif
#ifdef SYS_write
SC(write, CAT_IO)
#endif
#ifdef SYS_pread64
SC(pread64, CAT_IO)
#endif
#ifdef SYS_pwrite64
SC(pwrite64, CAT_IO)
#endif
#ifdef SYS_readv
SC(readv, CAT_IO)
#endif
#ifdef SYS_writev
SC(writev, CAT_IO)
#endif
#ifdef SYS_sendfile
SC(sendfile, CAT_IO)
#endif
#ifdef SYS_ioctl
SC(ioctl, CAT_IO)
#endif
#ifdef SYS_poll
SC(poll, CAT_IO)
#endif
#ifdef SYS_select
SC(select, CAT_IO)
#endif
#ifdef SYS_pselect6
SC(pselect6, CAT_IO)
#endif
#ifdef SYS_ppoll
SC(ppoll, CAT_IO)
#endif
#ifdef SYS_epoll_wait
SC(epoll_wait, CAT_IO)
#endif
#ifdef SYS_epoll_pwait
SC(epoll_pwait, CAT_IO)
#endif
#ifdef SYS_epoll_ctl
SC(epoll_ctl, CAT_IO)
#endif
#ifdef SYS_epoll_create1
SC(epoll_create1, CAT_IO)
#endif
#ifdef SYS_pipe
SC(pipe, CAT_IO)
#endif
#ifdef SYS_pipe2
SC(pipe2, CAT_IO)
#endif
#ifdef SYS_open
SC(open, CAT_FILE)
#endif
#ifdef SYS_openat
SC(openat, CAT_FILE)
#endif
#ifdef SYS_close
SC(close, CAT_FILE)
#endif
#ifdef SYS_stat
SC(stat, CAT_FILE)
#endif
#ifdef SYS_fstat
SC(fstat, CAT_FILE)
#endif
#ifdef SYS_lstat
SC(lstat, CAT_FILE)
#endif
#ifdef SYS_newfstatat
SC(newfstatat, CAT_FILE)
#endif
#ifdef SYS_statx
SC(statx, CAT_FILE)
#endif
#ifdef SYS_lseek
SC(lseek, CAT_FILE)
#endif
#ifdef SYS_access
SC(access, CAT_FILE)
#endif
#ifdef SYS_faccessat
SC(faccessat, CAT_FILE)
#endif
#ifdef SYS_faccessat2
SC(faccessat2, CAT_FILE)
#endif
#ifdef SYS_dup
SC(dup, CAT_FILE)
#endif
#ifdef SYS_dup2
SC(dup2, CAT_FILE)
#endif
#ifdef SYS_dup3
SC(dup3, CAT_FILE)
#endif
#ifdef SYS_fcntl
SC(fcntl, CAT_FILE)
#endif
#ifdef SYS_getdents
SC(getdents, CAT_FILE)
#endif
#ifdef SYS_getdents64
SC(getdents64, CAT_FILE)
#endif
#ifdef SYS_getcwd
SC(getcwd, CAT_FILE)
#endif
#ifdef SYS_chdir
SC(chdir, CAT_FILE)
#endif
#ifdef SYS_unlink
SC(unlink, CAT_FILE)
#endif
#ifdef SYS_unlinkat
SC(unlinkat, CAT_FILE)
#endif
#ifdef SYS_mkdir
SC(mkdir, CAT_FILE)
#endif
#ifdef SYS_mkdirat
SC(mkdirat, CAT_FILE)
#endif
#ifdef SYS_rmdir
SC(rmdir, CAT_FILE)
#endif
#ifdef SYS_rename
SC(rename, CAT_FILE)
#endif
#ifdef SYS_renameat
SC(renameat, CAT_FILE)
#endif
#ifdef SYS_readlink
SC(readlink, CAT_FILE)
#endif
#ifdef SYS_readlinkat
SC(readlinkat, CAT_FILE)
#endif
#ifdef SYS_ftruncate
SC(ftruncate, CAT_FILE)
#endif
#ifdef SYS_fsync
SC(fsync, CAT_FILE)
#endif
#ifdef SYS_statfs
SC(statfs, CAT_FILE)
#endif
#ifdef SYS_fstatfs
SC(fstatfs, CAT_FILE)
#endif
#ifdef SYS_mmap
SC(mmap, CAT_MEMORY)
#endif
#ifdef SYS_munmap
SC(munmap, CAT_MEMORY)
#endif
#ifdef SYS_mprotect
SC(mprotect, CAT_MEMORY)
#endif
#ifdef SYS_mremap
SC(mremap, CAT_MEMORY)
#endif
#ifdef SYS_brk
SC(brk, CAT_MEMORY)
#endif
#ifdef SYS_madvise
SC(madvise, CAT_MEMORY)
#endif
#ifdef SYS_fork
SC(fork, CAT_PROCESS)
#endif
#ifdef SYS_vfork
SC(vfork, CAT_PROCESS)
#endif
#ifdef SYS_clone
SC(clone, CAT_PROCESS)
#endif
#ifdef SYS_clone3
SC(clone3, CAT_PROCESS)
#endif
#ifdef SYS_execve
SC(execve, CAT_PROCESS)
#endif
#ifdef SYS_execveat
SC(execveat, CAT_PROCESS)
#endif
#ifdef SYS_wait4
SC(wait4, CAT_PROCESS)
#endif
#ifdef SYS_waitid
SC(waitid, CAT_PROCESS)
#endif
#ifdef SYS_exit
SC(exit, CAT_PROCESS)
#endif
#ifdef SYS_exit_group
SC(exit_group, CAT_PROCESS)
#endif
#ifdef SYS_getpid
SC(getpid, CAT_PROCESS)
#endif
#ifdef SYS_getppid
SC(getppid, CAT_PROCESS)
#endif
#ifdef SYS_gettid
SC(gettid, CAT_PROCESS)
#endif
#ifdef SYS_kill
SC(kill, CAT_PROCESS)
#endif
#ifdef SYS_tgkill
SC(tgkill, CAT_PROCESS)
#endif
#ifdef SYS_set_tid_address
SC(set_tid_address, CAT_PROCESS)
#endif
#ifdef SYS_set_robust_list
SC(set_robust_list, CAT_PROCESS)
#endif
#ifdef SYS_rseq
SC(rseq, CAT_PROCESS)
#endif
#ifdef SYS_prlimit64
SC(prlimit64, CAT_PROCESS)
#endif
#ifdef SYS_arch_prctl
SC(arch_prctl, CAT_PROCESS)
#endif
#ifdef SYS_prctl
SC(prctl, CAT_PROCESS)
#endif
#ifdef SYS_futex
SC(futex, CAT_PROCESS)
#endif
#ifdef SYS_sched_yield
SC(sched_yield, CAT_PROCESS)
#endif
#ifdef SYS_sched_getaffinity
SC(sched_getaffinity, CAT_PROCESS)
#endif
#ifdef SYS_nanosleep
SC(nanosleep, CAT_OTHER)
#endif
#ifdef SYS_clock_nanosleep
SC(clock_nanosleep, CAT_OTHER)
#endif
#ifdef SYS_clock_gettime
SC(clock_gettime, CAT_OTHER)
#endif
#ifdef SYS_gettimeofday
SC(gettimeofday, CAT_OTHER)
#endif
#ifdef SYS_getrandom
SC(getrandom, CAT_OTHER)
#endif
#ifdef SYS_uname
SC(uname, CAT_OTHER)
#endif
#ifdef SYS_sysinfo
SC(sysinfo, CAT_OTHER)
#endif
#ifdef SYS_getuid
SC(getuid, CAT_OTHER)
#endif
#ifdef SYS_geteuid
SC(geteuid, CAT_OTHER)
#endif
#ifdef SYS_getgid
SC(getgid, CAT_OTHER)
#endif
#ifdef SYS_getegid
SC(getegid, CAT_OTHER)
#endif
#ifdef SYS_rt_sigaction
SC(rt_sigaction, CAT_OTHER)
#endif
#ifdef SYS_rt_sigprocmask
SC(rt_sigprocmask, CAT_OTHER)
#endif
#ifdef SYS_rt_sigreturn
SC(rt_sigreturn, CAT_OTHER)
#endif
#ifdef SYS_sigaltstack
SC(sigaltstack, CAT_OTHER)
#endif
