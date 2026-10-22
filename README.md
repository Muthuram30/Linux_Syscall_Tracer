# Linux System Call Tracer & Process Monitor

An educational operating systems project demonstrating system call tracing using `ptrace()`, process monitoring via `/proc`, and real-time visualization through an interactive web dashboard.

## Abstract

This project implements a comprehensive system call tracer and process monitor for Linux, designed to help students understand fundamental operating system concepts including system calls, process management, memory management, and the user/kernel boundary. The tracer uses `ptrace()` to intercept system calls, reads process information from the `/proc` virtual filesystem, and presents the data through an educational web dashboard.

## Problem Statement

Understanding how user programs interact with the operating system kernel is crucial for systems programming. However, this interaction happens at a low level that is difficult to observe. This project addresses the need for an educational tool that:

1. Makes system calls visible and understandable
2. Demonstrates the process lifecycle (fork, exec, wait, exit)
3. Shows real-time resource usage (CPU, memory)
4. Explains the user space / kernel space boundary
5. Provides hands-on experience with Linux system programming

## Objectives

- **Primary:** Build a working system call tracer using Linux `ptrace()` API
- **Secondary:** Monitor process information using `/proc` filesystem
- **Tertiary:** Visualize system call patterns and process behavior
- **Educational:** Demonstrate core OS concepts in an accessible way

## OS Concepts Demonstrated

### 1. System Calls
- Interface between user programs and the kernel
- Transition from user mode to kernel mode
- System call numbers and arguments
- Return values and error handling

### 2. Process Management
- Process creation: `fork()`, `vfork()`, `clone()`
- Program execution: `execve()`, `execveat()`
- Process synchronization: `wait4()`, `waitid()`
- Process termination: `exit()`, `exit_group()`

### 3. Memory Management
- Virtual memory: `mmap()`, `munmap()`
- Heap management: `brk()`, `sbrk()`
- Memory protection: `mprotect()`
- Memory advice: `madvise()`

### 4. File I/O
- File operations: `open()`, `openat()`, `read()`, `write()`, `close()`
- File metadata: `stat()`, `fstat()`, `lstat()`
- Directory operations: `getdents()`, `mkdir()`, `rmdir()`

### 5. Process Tracing
- `ptrace()` system call and its capabilities
- PTRACE_TRACEME, PTRACE_ATTACH, PTRACE_SYSCALL
- Stopping at system call entry and exit
- Reading tracee memory and registers

### 6. /proc Filesystem
- Virtual filesystem exposing kernel data
- Per-process information: `/proc/[pid]/stat`, `/proc/[pid]/status`
- System information: `/proc/meminfo`, `/proc/stat`

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│                  User Space                          │
├─────────────────────────────────────────────────────┤
│                                                       │
│  ┌─────────────┐         ┌──────────────┐          │
│  │ Test Program│         │   Tracer     │          │
│  │             │◄────────│  (ptrace)    │          │
│  └─────────────┘         └──────────────┘          │
│        │                        │                    │
│        │ system calls           │ traces syscalls   │
│        ▼                        ▼                    │
├─────────────────────────────────────────────────────┤
│              System Call Interface                   │
├─────────────────────────────────────────────────────┤
│                  Kernel Space                        │
│                                                       │
│  ┌────────────┐  ┌────────────┐  ┌──────────────┐ │
│  │  Process   │  │   Memory   │  │   File       │ │
│  │  Manager   │  │   Manager  │  │   System     │ │
│  └────────────┘  └────────────┘  └──────────────┘ │
│                                                       │
└─────────────────────────────────────────────────────┘
         │                                  │
         ▼                                  ▼
┌─────────────────┐              ┌─────────────────┐
│  /proc Virtual  │              │  Web Dashboard  │
│   Filesystem    │              │   (Flask)       │
└─────────────────┘              └─────────────────┘
```

## Software Requirements

### Build Requirements
- **OS:** Ubuntu Linux 20.04 or later (or any modern Linux distribution)
- **Compiler:** GCC 9.0+ or Clang 10.0+
- **Kernel:** Linux 5.3+ (for PTRACE_GET_SYSCALL_INFO support)
- **Build Tools:** GNU Make

### Runtime Requirements
- **Python:** 3.8+ (for dashboard)
- **Flask:** 3.0+ (install via pip)

### Optional Tools
- `strace` - for comparison testing
- `gdb` - for debugging
- Modern web browser (Chrome, Firefox, Edge)

## Project Structure

```
syscall-tracer/
├── README.md                 # This file
├── Makefile                  # Build configuration
├── src/                      # C source files
│   ├── main.c               # Command-line interface
│   ├── syscall_tracer.c     # ptrace-based tracer
│   ├── process_monitor.c    # /proc reader
│   ├── logger.c             # Log file writer
│   └── syscall_table.c      # Syscall name/category lookup
├── include/                  # C header files
│   ├── syscall_tracer.h
│   ├── process_monitor.h
│   ├── logger.h
│   ├── syscall_table.h
│   └── syscall_list.inc     # Generated syscall list
├── tests/                    # Test programs and scripts
│   ├── test_program.c       # Demo program that exercises syscalls
│   └── run_tests.sh         # Automated test suite
├── logs/                     # Generated log files (created at runtime)
├── build/                    # Compiled binaries (created by make)
├── dashboard/                # Web dashboard
│   ├── app.py               # Flask application
│   ├── requirements.txt     # Python dependencies
│   ├── templates/           # HTML templates
│   │   └── index.html
│   └── static/              # CSS and JavaScript
│       ├── css/style.css
│       └── js/app.js
└── docs/                     # Documentation
    ├── viva.md              # Viva questions and answers
    └── presentation.md      # Presentation slides
```

## Installation

### 1. Clone or Download

```bash
cd ~
# Project should be in ~/syscall-tracer
cd syscall-tracer
```

### 2. Install System Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential gcc make python3 python3-pip

# Fedora/RHEL
sudo dnf install gcc make python3 python3-pip

# Arch Linux
sudo pacman -S base-devel python python-pip
```

### 3. Install Python Dependencies (for dashboard)

```bash
pip3 install -r dashboard/requirements.txt
# Or install Flask directly:
pip3 install flask
```

### 4. Verify Installation

```bash
gcc --version    # Should show GCC version
python3 --version # Should show Python 3.8+
make --version   # Should show GNU Make
```

## Compilation

### Build Everything

```bash
make
```

This compiles:
- `build/syscall-tracer` - Main tracer executable
- `build/test_program` - Test program

### Build Options

```bash
make clean          # Remove build artifacts
make test           # Run automated tests
make run            # Trace the test program
make help           # Show available targets
```

### Manual Compilation

If you need to compile manually:

```bash
mkdir -p build logs

# Compile individual modules
gcc -Wall -Wextra -g -O0 -Iinclude -c src/logger.c -o build/logger.o
gcc -Wall -Wextra -g -O0 -Iinclude -c src/syscall_table.c -o build/syscall_table.o
gcc -Wall -Wextra -g -O0 -Iinclude -c src/process_monitor.c -o build/process_monitor.o
gcc -Wall -Wextra -g -O0 -Iinclude -c src/syscall_tracer.c -o build/syscall_tracer.o
gcc -Wall -Wextra -g -O0 -Iinclude -c src/main.c -o build/main.o

# Link
gcc -o build/syscall-tracer build/*.o

# Compile test program
gcc -Wall -Wextra -g -O0 -o build/test_program tests/test_program.c
```

## Running the Project

### 1. Basic System Call Tracing

Trace a program from start to finish:

```bash
./build/syscall-tracer --run ./build/test_program
```

This will:
- Fork and trace the test program
- Capture all system calls
- Write a log file to `logs/trace_YYYYMMDD_HHMMSS.log`
- Display statistics when the program exits

### 2. Quiet Mode

Suppress per-syscall output to stderr:

```bash
./build/syscall-tracer --run ./build/test_program --quiet
```

### 3. Attach to Running Process

Attach to an already-running process by PID:

```bash
# In terminal 1, start a long-running program:
sleep 300 &
echo $!   # Note the PID

# In terminal 2, attach the tracer:
./build/syscall-tracer --trace <PID>
```

**Note:** Linux restricts ptrace to processes you own. The Yama LSM may further restrict tracing to your own child processes. You may need to temporarily adjust `/proc/sys/kernel/yama/ptrace_scope` or use `sudo`.

### 4. Process Monitoring (No Tracing)

Monitor a process using only `/proc` (no ptrace):

```bash
# Monitor your shell
./build/syscall-tracer --monitor $$

# Monitor and launch a program
./build/syscall-tracer --monitor-run ./build/test_program
```

### 5. Generate Statistics from Log

Analyze an existing log file:

```bash
./build/syscall-tracer --stats logs/trace_20261128_120000.log
```

### 6. Custom Log Directory and JSON Output

```bash
./build/syscall-tracer --run ./build/test_program \
    --log-dir ./my_logs \
    --json ./my_logs/snapshot.json
```

## Dashboard Usage

### Start the Dashboard

```bash
# Terminal 1: Start the dashboard
cd ~/syscall-tracer
python3 dashboard/app.py
```

The dashboard will be available at: **http://127.0.0.1:5000**

### Run a Trace with Live Updates

```bash
# Terminal 2: Run the tracer with JSON output
./build/syscall-tracer --run ./build/test_program
```

The tracer automatically writes to `logs/latest.json` which the dashboard reads every second.

### Dashboard Features

- **System Overview:** Real-time stats cards showing CPU, memory, processes, syscall rate
- **Process Info:** Detailed information about the traced process
- **System Call Stream:** Live table of recent system calls
- **Frequency Chart:** Bar chart of most frequent syscalls
- **Categories:** Breakdown by file, process, memory, I/O, other
- **Memory Chart:** CPU and memory usage over time
- **Process Tree:** Visual representation of process hierarchy
- **Statistics:** Detailed syscall counts and percentages
- **Logs:** Milestone events and log file information
- **About:** Educational information about OS concepts

## CLI Commands Reference

```bash
# Show help
./build/syscall-tracer --help

# Trace a command
./build/syscall-tracer --run <command> [args...]

# Attach to existing process
./build/syscall-tracer --trace <PID>

# Monitor process (no ptrace)
./build/syscall-tracer --monitor <PID> [--interval <sec>]

# Monitor and launch
./build/syscall-tracer --monitor-run <command> [args...]

# Analyze log file
./build/syscall-tracer --stats <logfile>

# Options
--quiet              # Don't print each syscall to stderr
--log-dir <dir>      # Log directory (default: logs)
--json <file>        # JSON snapshot file (default: logs/latest.json)
--interval <sec>     # Monitor refresh interval (default: 1)
```

## System Call Tracing Explanation

### How ptrace() Works

The `ptrace()` system call allows one process (the tracer) to observe and control another process (the tracee):

1. **PTRACE_TRACEME:** Child process requests to be traced by its parent
2. **PTRACE_ATTACH:** Tracer attaches to an existing process
3. **PTRACE_SYSCALL:** Resume tracee; kernel stops it at next syscall
4. **PTRACE_GET_SYSCALL_INFO:** Read syscall number, arguments, return value

### System Call Entry and Exit

Every system call has two stops:

```
User Program                    Kernel

    │
    │  syscall instruction
    ├───────────────────────►  [ENTRY STOP]
    │                           - syscall number
    │                           - arguments
    │                           
    │                          [Kernel executes syscall]
    │                           
    │◄──────────────────────   [EXIT STOP]
    │                           - return value
    │                           - error indicator
    │
```

The tracer sees both stops and can extract:
- At entry: syscall number, 6 arguments
- At exit: return value, success/failure

### Architecture Independence

We use `PTRACE_GET_SYSCALL_INFO` (Linux 5.3+) which provides architecture-independent syscall information. Older tracers had to manually decode CPU registers, which differed between x86_64, ARM, etc.

## /proc Filesystem Explanation

The `/proc` filesystem is a **virtual filesystem** - files don't exist on disk but are generated on-the-fly by the kernel.

### Key Files We Read

- **/proc/[pid]/stat** - Process status (PID, state, CPU time, memory, threads)
- **/proc/[pid]/status** - Detailed status (VmSize, VmRSS, context switches)
- **/proc/[pid]/cmdline** - Command-line arguments
- **/proc/[pid]/comm** - Process name
- **/proc/meminfo** - System memory information
- **/proc/stat** - System boot time (for calculating process start time)

### Reading Process State

```c
// /proc/[pid]/stat fields (space-separated)
// Field 3 is the process state character:
// R = Running, S = Sleeping, D = Disk sleep, Z = Zombie, T = Stopped
FILE *f = fopen("/proc/12345/stat", "r");
```

## Process Lifecycle

Our test program demonstrates the complete process lifecycle:

```
┌──────────────┐
│    fork()    │  Creates a child process
└──────┬───────┘
       │
       ├─► Parent                 Child
       │   │                      │
       │   │ wait4()              │ exec()
       │   │  (blocked)           │  Replace program image
       │   │                      │
       │   │                      │ [Program runs]
       │   │                      │
       │   │                      │ exit(status)
       │   │◄─────────────────────┘
       │   │
       │   │  [wait4 returns]
       │   │  [child is reaped]
       │   ▼
       └─► exit_group()
```

### System Calls Involved

1. **fork() / clone():** Duplicate process
2. **execve():** Replace program image
3. **wait4() / waitid():** Wait for child state change
4. **exit() / exit_group():** Terminate process/thread

## Testing

### Run Automated Tests

```bash
make test
# Or directly:
bash tests/run_tests.sh
```

The test suite verifies:
- Binary compilation
- Help command
- Standalone test program execution
- Tracing with different modes (io, mem, fork, exec)
- Statistics generation
- Monitor mode
- Comparison with `strace` (if available)
- JSON output
- Error handling

### Manual Testing

Test individual syscall categories:

```bash
./build/syscall-tracer --run ./build/test_program io      # File I/O
./build/syscall-tracer --run ./build/test_program mem     # Memory
./build/syscall-tracer --run ./build/test_program fork    # fork/wait
./build/syscall-tracer --run ./build/test_program exec    # exec
./build/syscall-tracer --run ./build/test_program sleep   # nanosleep
```

### Compare with strace

```bash
# Our tracer
./build/syscall-tracer --run ./build/test_program --quiet

# strace
strace -c ./build/test_program

# Both should show similar syscall counts
```

## Results

### Sample Output

```
=========== System Call Statistics (x86_64) ===========
Total syscalls  : 159
Unique syscalls : 21
Most frequent   : openat() x62 (39.0%)

By category:
  file      128  ( 80.5%)
  process    10  (  6.3%)
  memory     14  (  8.8%)
  io          7  (  4.4%)
  other       0  (  0.0%)

Per syscall:
  openat                 62   39.0%
  newfstatat             62   39.0%
  mmap                    8    5.0%
  mprotect                4    2.5%
  read                    1    0.6%
  write                   2    1.3%
  execve                  1    0.6%
  exit_group              1    0.6%
===================================================
Log file: logs/trace_20261128_233023.log
Tracee exited with code 0
```

### Log File Format

```
# syscall-tracer log | run | arch=x86_64 | started 2026-11-28 23:30:23.123 | root_pid=12345
# TIMESTAMP|PID|SYSCALL|NR|PHASE|RETURN|STATE|DETAIL
2026-11-28 23:30:23.145|12345|openat|257|ENTER|-|-|path=/tmp/test.txt flags=0x241
2026-11-28 23:30:23.146|12345|openat|257|EXIT|3|S|path=/tmp/test.txt flags=0x241
2026-11-28 23:30:23.147|12345|write|1|ENTER|-|-|fd=3 count=28
2026-11-28 23:30:23.148|12345|write|1|EXIT|28|S|fd=3 count=28
```

## Limitations

1. **Kernel Version:** Requires Linux 5.3+ for PTRACE_GET_SYSCALL_INFO
2. **Performance:** ptrace adds significant overhead (not for production)
3. **Permissions:** Can only trace processes you own (unless running as root)
4. **Yama LSM:** `/proc/sys/kernel/yama/ptrace_scope` may restrict tracing
5. **vDSO Calls:** Some syscalls (gettimeofday, clock_gettime) may use vDSO and bypass tracing
6. **Architecture:** Tested on x86_64; should work on aarch64 but syscall names may differ
7. **Threads:** Tracks threads but visualization is simplified

## Future Improvements

### Technical Enhancements
- [ ] Support for older kernels (manual register decoding)
- [ ] Argument decoding (strings, structures, pointers)
- [ ] Return value interpretation (errno messages)
- [ ] Filtering by syscall category or name
- [ ] Performance profiling (syscall duration)
- [ ] Network syscall support (socket, bind, connect)
- [ ] Signal handling visualization

### Dashboard Improvements
- [ ] Real-time log streaming
- [ ] Multiple trace comparison
- [ ] Export to CSV/JSON
- [ ] Timeline view of process lifecycle
- [ ] Advanced filtering and search
- [ ] Mobile-responsive design
- [ ] Dark/light theme toggle

### Educational Features
- [ ] Interactive syscall explorer
- [ ] Step-by-step syscall explanation
- [ ] Quiz mode for learning
- [ ] Annotated examples
- [ ] Video tutorials

## Troubleshooting

### "Operation not permitted" when tracing

**Problem:** `PTRACE_ATTACH` fails with EPERM

**Solutions:**
1. Check if you own the process: `ps -p <PID> -o user=`
2. Check Yama ptrace scope: `cat /proc/sys/kernel/yama/ptrace_scope`
   - 0 = any process can trace any other (least secure)
   - 1 = only parent/child (default on Ubuntu)
   - 2 = admin only
   - 3 = no tracing allowed
3. Temporarily allow tracing: `echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope`
4. Use `--run` instead of `--trace` (creates child process, allowed by default)

### "PTRACE_GET_SYSCALL_INFO failed"

**Problem:** Kernel doesn't support PTRACE_GET_SYSCALL_INFO

**Solution:** Upgrade to Linux 5.3 or later, or modify the code to manually read registers (architecture-specific)

### Dashboard shows "No Data"

**Problem:** Dashboard can't read JSON snapshot

**Solutions:**
1. Ensure tracer is running with JSON output enabled (default)
2. Check `logs/latest.json` exists and is readable
3. Verify dashboard and tracer are in the same directory structure
4. Refresh the browser (Ctrl+F5)

### Flask import error

**Problem:** `ModuleNotFoundError: No module named 'flask'`

**Solution:**
```bash
pip3 install flask
# Or with requirements file:
pip3 install -r dashboard/requirements.txt
```

### Test program doesn't generate many syscalls

This is expected! Modern Linux uses techniques to reduce syscalls:
- vDSO for time-related calls
- Buffered I/O in libc
- Dynamic linker caching

You can verify with: `strace -c ./build/test_program`

## References

### Man Pages
- `man 2 ptrace` - Process tracing
- `man 2 syscall` - System call interface
- `man 5 proc` - /proc filesystem
- `man 2 fork` - Create child process
- `man 2 execve` - Execute program
- `man 2 wait` - Wait for process

### Linux Kernel Documentation
- https://www.kernel.org/doc/html/latest/admin-guide/LSM/Yama.html
- https://www.kernel.org/doc/html/latest/filesystems/proc.html

### Books
- "The Linux Programming Interface" by Michael Kerrisk
- "Advanced Programming in the UNIX Environment" by Stevens & Rago
- "Operating System Concepts" by Silberschatz, Galvin, Gagne

## Conclusion

This project demonstrates fundamental operating system concepts through hands-on implementation. By building a system call tracer and process monitor, we gain deep insights into:

- How user programs interact with the kernel
- The mechanics of system calls
- Process lifecycle and management
- Memory management at the OS level
- The design of the Linux `/proc` filesystem
- Real-world use of `ptrace()` for debugging and analysis

The project is educational, modular, and extensively documented, making it suitable for college OS courses, technical presentations, and self-study.

## Authors

Developed as an educational OS project for college coursework.

## License

This is an educational project. Free to use for learning purposes.

---

**Last Updated:** September 2026  
**Tested On:** Ubuntu 24.04 LTS, Linux Kernel 6.8+, GCC 13.3
