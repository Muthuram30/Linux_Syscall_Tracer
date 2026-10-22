# Viva Questions and Answers
## Linux System Call Tracer & Process Monitor

---

## Basic Operating System Concepts

### 1. What is an Operating System?

**Answer:** An operating system is system software that manages computer hardware and software resources and provides common services for computer programs. It acts as an intermediary between users/applications and the computer hardware. Key functions include:
- Process management
- Memory management
- File system management
- Device management
- Security and access control

### 2. What are the main functions of an Operating System?

**Answer:**
1. **Process Management:** Creating, scheduling, and terminating processes
2. **Memory Management:** Allocating and deallocating memory, virtual memory
3. **File Management:** Creating, deleting, reading, writing files
4. **Device Management:** Managing I/O devices through device drivers
5. **Security:** User authentication, access control, protection
6. **Resource Allocation:** CPU scheduling, disk scheduling

### 3. What is a System Call?

**Answer:** A system call is a programmatic way for a program to request a service from the operating system kernel. It's the interface between user applications and the kernel. When a program makes a system call:
1. The CPU switches from user mode to kernel mode
2. Control transfers to the kernel
3. The kernel performs the requested operation
4. Results are returned to the user program
5. CPU switches back to user mode

Examples: `read()`, `write()`, `fork()`, `open()`, `close()`

### 4. What is the difference between User Mode and Kernel Mode?

**Answer:**

**User Mode:**
- Normal mode for running user applications
- Limited access to hardware and memory
- Cannot execute privileged instructions
- Safer - errors don't crash the system
- Programs run in isolated virtual address spaces

**Kernel Mode:**
- Privileged mode for OS kernel
- Full access to all hardware and memory
- Can execute any CPU instruction
- Direct access to device controllers
- Errors can crash the entire system

The CPU uses a mode bit to distinguish between these modes.

### 5. What happens during a System Call?

**Answer:**
1. User program calls a library function (e.g., `write()` in libc)
2. Library function prepares syscall arguments and number
3. Library executes special instruction (`syscall` on x86_64, `svc` on ARM)
4. **Mode switch:** CPU transitions from user mode to kernel mode
5. **Context switch:** Saves user context, loads kernel context
6. Kernel identifies syscall number and dispatches to appropriate handler
7. Kernel performs the operation (e.g., writes to file)
8. Kernel prepares return value
9. **Mode switch:** CPU returns to user mode
10. Control returns to user program with result

This entire process is called a "trap" into the kernel.

---

## Process Management

### 6. What is a Process?

**Answer:** A process is a program in execution. It includes:
- **Code (text section):** The executable instructions
- **Data section:** Global and static variables
- **Heap:** Dynamically allocated memory
- **Stack:** Temporary data (function parameters, return addresses, local variables)
- **Program Counter:** Next instruction to execute
- **CPU registers:** Current values of registers
- **Process Control Block (PCB):** Metadata maintained by OS

### 7. What is a PID?

**Answer:** PID stands for **Process IDentifier**. It's a unique numerical identifier assigned to each process by the operating system. PIDs are:
- Positive integers (typically 1 to 32768)
- PID 0: Scheduler/swapper (kernel)
- PID 1: init/systemd (first user process)
- Used by the kernel to track and manage processes
- Referenced by system calls like `kill()`, `wait()`, `getpid()`

### 8. What is PPID?

**Answer:** PPID stands for **Parent Process IDentifier**. It's the PID of the process that created the current process. Every process (except init) has a parent. When a parent process terminates, orphaned children are re-parented to init/systemd (PID 1).

### 9. Explain the fork() system call.

**Answer:** `fork()` creates a new process by duplicating the calling process:

**Behavior:**
- Creates a child process (exact copy of parent)
- Child gets a new PID
- Child's PPID is the parent's PID
- Both processes continue execution after `fork()`
- Returns twice:
  - Returns 0 in the child process
  - Returns child's PID in the parent
  - Returns -1 on error

**Memory:**
- Child gets a copy of parent's address space
- Uses Copy-on-Write (COW) for efficiency
- Initially shares pages, copies only when modified

**Example:**
```c
pid_t pid = fork();
if (pid == 0) {
    // Child process
    printf("I am child\\n");
} else if (pid > 0) {
    // Parent process
    printf("I am parent, child is %d\\n", pid);
}
```

### 10. Explain the exec() system call family.

**Answer:** The `exec()` family replaces the current process image with a new program:

**Key Functions:**
- `execve()` - most fundamental (takes path, argv, envp)
- `execl()`, `execv()` - variants with different argument passing
- `execle()`, `execvp()` - variants with path search or environment

**Behavior:**
- Loads new program into memory
- Replaces code, data, heap, stack
- Preserves PID, PPID, open file descriptors
- On success, never returns (old program is gone)
- On error, returns -1

**Common Pattern (fork + exec):**
```c
pid_t pid = fork();
if (pid == 0) {
    execl("/bin/ls", "ls", "-l", NULL);
    perror("exec failed");  // Only reached if exec fails
    exit(1);
}
```

### 11. What is the difference between fork() and exec()?

**Answer:**

**fork():**
- Creates a new process
- Duplicates the parent process
- Both parent and child continue with the same code
- Returns twice

**exec():**
- Does NOT create a new process
- Replaces current process with a new program
- Same PID, different program
- Never returns on success

**Together:** They enable running new programs:
```c
fork();  // Create child
exec();  // Child becomes new program
```

### 12. Explain the wait() system call.

**Answer:** `wait()` and `wait4()` make a parent process wait for a child to terminate:

**Purpose:**
- Prevents zombie processes
- Retrieves child's exit status
- Synchronizes parent with child termination

**Behavior:**
- Blocks parent until any child terminates
- Returns PID of terminated child
- Fills status parameter with exit information
- If no children exist, returns -1 (ECHILD)

**Status Information:**
- `WIFEXITED(status)` - true if exited normally
- `WEXITSTATUS(status)` - exit code (0-255)
- `WIFSIGNALED(status)` - true if killed by signal
- `WTERMSIG(status)` - signal number

**Example:**
```c
int status;
pid_t pid = wait(&status);
if (WIFEXITED(status)) {
    printf("Child %d exited with code %d\\n", 
           pid, WEXITSTATUS(status));
}
```

### 13. What is a Zombie Process?

**Answer:** A zombie (or defunct) process is a process that has completed execution but still has an entry in the process table:

**Characteristics:**
- Process has terminated
- Exit status not yet collected by parent
- Appears as `<defunct>` in process listings
- State: **Z** (zombie)
- Doesn't consume CPU or memory (except PCB)

**Cause:**
Parent hasn't called `wait()` to read child's exit status

**Problem:**
Too many zombies can exhaust PID space

**Solution:**
Parent must call `wait()` or `waitpid()` to "reap" zombies

**Example:**
```c
// Creates zombie if parent doesn't wait
if (fork() == 0) {
    exit(0);  // Child exits immediately
}
// Parent continues without waiting - child becomes zombie
sleep(100);  // Zombie exists for 100 seconds
```

### 14. What is an Orphan Process?

**Answer:** An orphan process is a process whose parent has terminated before it:

**What Happens:**
- When a parent terminates, its children become orphans
- The OS automatically re-parents orphans to init/systemd (PID 1)
- Init adopts orphans and eventually reaps them
- Orphans continue executing normally

**Not a Problem:**
Unlike zombies, orphans are properly managed by init

**Example:**
```c
if (fork() == 0) {
    sleep(10);  // Child sleeps
    // Parent will be gone, child is orphaned
    printf("PPID: %d\\n", getppid());  // Shows 1 (init)
}
// Parent exits immediately
```

### 15. What are the different process states?

**Answer:**

**Main States:**
1. **Running (R):** Executing on CPU or ready to run
2. **Sleeping (S):** Waiting for an event (interruptible)
3. **Disk Sleep (D):** Waiting for I/O (uninterruptible)
4. **Stopped (T):** Stopped by signal (SIGSTOP, SIGTSTP)
5. **Tracing Stop (t):** Stopped by ptrace
6. **Zombie (Z):** Terminated but not reaped
7. **Dead (X):** Being terminated

**State Transitions:**
```
  New → Ready → Running → Terminated
          ↑       ↓
          └─ Waiting
```

**In Linux /proc:**
Read from `/proc/[pid]/stat`, field 3

---

## Memory Management

### 16. What is Virtual Memory?

**Answer:** Virtual memory is a memory management technique that provides:

**Benefits:**
1. Each process has its own address space
2. Processes isolated from each other
3. Can use more memory than physically available
4. Simplifies memory allocation

**Implementation:**
- Uses paging or segmentation
- Virtual addresses translated to physical addresses
- Page table maintained by kernel
- Hardware support from MMU (Memory Management Unit)

**Address Translation:**
Virtual Address → MMU → Physical Address

### 17. Explain mmap() system call.

**Answer:** `mmap()` creates a new mapping in the virtual address space:

**Purpose:**
- Map files or devices into memory
- Allocate anonymous memory (heap alternative)
- Share memory between processes

**Advantages:**
- Fast file I/O (no read/write syscalls)
- Shared memory for IPC
- Large memory allocations

**Example:**
```c
void *addr = mmap(NULL, length, 
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS,
                  -1, 0);
if (addr == MAP_FAILED) {
    perror("mmap");
}
```

**Our Project:**
Large `malloc()` calls use `mmap()` internally

### 18. What is the difference between brk() and mmap()?

**Answer:**

**brk() / sbrk():**
- Adjusts program break (end of data segment)
- Used for small allocations
- Contiguous memory
- Part of the heap
- `malloc()` uses this for small allocations

**mmap():**
- Maps memory anywhere in address space
- Used for large allocations (>128 KB typically)
- Non-contiguous
- Can map files
- `malloc()` uses this for large allocations

**In Our Trace:**
```c
malloc(100)      → brk()    // Small allocation
malloc(10000000) → mmap()   // Large allocation
```

---

## File I/O

### 19. Explain the open() system call.

**Answer:** `open()` opens a file and returns a file descriptor:

**Syntax:**
```c
int fd = open(pathname, flags, mode);
```

**Flags:**
- `O_RDONLY` - Read only
- `O_WRONLY` - Write only
- `O_RDWR` - Read and write
- `O_CREAT` - Create if doesn't exist
- `O_TRUNC` - Truncate to zero length
- `O_APPEND` - Append mode

**Returns:**
- File descriptor (non-negative integer) on success
- -1 on error (sets errno)

**Modern Variant:**
`openat()` - opens relative to directory FD (safer)

### 20. What is a File Descriptor?

**Answer:** A file descriptor is a small non-negative integer that represents an open file:

**Key Points:**
- Per-process table maintained by kernel
- Index into file descriptor table
- Standard descriptors:
  - 0 = stdin
  - 1 = stdout
  - 2 = stderr

**What it References:**
- Regular files
- Directories
- Sockets
- Pipes
- Devices

**Operations:**
- read(fd, ...)
- write(fd, ...)
- close(fd)

### 21. Explain read() and write() system calls.

**Answer:**

**read():**
```c
ssize_t read(int fd, void *buf, size_t count);
```
- Reads up to count bytes from fd into buf
- Returns number of bytes read
- Returns 0 at EOF
- Returns -1 on error

**write():**
```c
ssize_t write(int fd, const void *buf, size_t count);
```
- Writes up to count bytes from buf to fd
- Returns number of bytes written
- May write less than count (short write)
- Returns -1 on error

**Important:**
Neither guarantees reading/writing all requested bytes. Must check return value and retry if needed.

---

## Process Tracing (ptrace)

### 22. What is ptrace()?

**Answer:** `ptrace()` is a system call that allows one process to observe and control another:

**Uses:**
- Debuggers (gdb, lldb)
- Tracers (strace, our project)
- System call filtering
- Security monitoring

**Key Operations:**
- `PTRACE_TRACEME` - Child requests tracing
- `PTRACE_ATTACH` - Attach to existing process
- `PTRACE_SYSCALL` - Stop at syscall entry/exit
- `PTRACE_GET_SYSCALL_INFO` - Read syscall details
- `PTRACE_DETACH` - Stop tracing

**Security:**
- Can only trace processes you own
- Restricted by Yama LSM
- Requires appropriate permissions

### 23. How does our tracer work?

**Answer:**

**Step 1: Start Tracing**
```c
pid = fork();
if (pid == 0) {
    ptrace(PTRACE_TRACEME, ...);  // Child requests tracing
    exec(program);                 // Become target program
}
// Parent becomes the tracer
```

**Step 2: Set Options**
```c
ptrace(PTRACE_SETOPTIONS, pid, NULL, 
       PTRACE_O_TRACESYSGOOD | 
       PTRACE_O_TRACEFORK |
       PTRACE_O_TRACECLONE);
```

**Step 3: Resume and Wait**
```c
ptrace(PTRACE_SYSCALL, pid, NULL, NULL);  // Resume until next syscall
waitpid(pid, &status, 0);                  // Wait for stop
```

**Step 4: Inspect Syscall**
```c
struct ptrace_syscall_info info;
ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info);
// info contains syscall number, arguments, return value
```

**Step 5: Repeat**

### 24. What is PTRACE_GET_SYSCALL_INFO?

**Answer:** `PTRACE_GET_SYSCALL_INFO` is a modern ptrace request (Linux 5.3+) that provides **architecture-independent** system call information:

**Why It's Better:**
- Old method: Manually read CPU registers (architecture-specific)
- New method: Kernel provides structured information

**Information Provided:**
- Operation type (entry, exit, seccomp)
- Syscall number
- Six arguments (at entry)
- Return value and error flag (at exit)
- Architecture identifier

**Structure:**
```c
struct ptrace_syscall_info {
    uint8_t op;          // PTRACE_SYSCALL_INFO_ENTRY or _EXIT
    uint32_t arch;       // Architecture (AUDIT_ARCH_X86_64, etc.)
    uint64_t instruction_pointer;
    union {
        struct {         // At entry
            uint64_t nr;
            uint64_t args[6];
        } entry;
        struct {         // At exit
            int64_t rval;
            uint8_t is_error;
        } exit;
    };
};
```

---

## /proc Filesystem

### 25. What is the /proc filesystem?

**Answer:** `/proc` is a **virtual filesystem** that provides an interface to kernel data structures:

**Characteristics:**
- Not stored on disk
- Files generated on-the-fly by kernel
- Updated in real-time
- Read-only for most files
- Mounted at `/proc`

**Contents:**
- Per-process directories: `/proc/[pid]/`
- System information: `/proc/meminfo`, `/proc/cpuinfo`
- Kernel parameters: `/proc/sys/`

**Purpose:**
- Query process information
- Inspect system state
- Tune kernel parameters (via /proc/sys)

### 26. What information can we get from /proc/[pid]/stat?

**Answer:** `/proc/[pid]/stat` provides process status in one line:

**Key Fields:**
1. PID
2. Process name (in parentheses)
3. State (R, S, D, Z, T, etc.)
4. PPID
5-13. Various counters
14. utime - CPU time in user mode
15. stime - CPU time in kernel mode
18. Priority
19. Nice value
20. Number of threads
22. Start time (in clock ticks since boot)

**Usage:**
```c
FILE *f = fopen("/proc/12345/stat", "r");
fscanf(f, "%d %s %c %d ... %llu %llu ...", 
       &pid, name, &state, &ppid, &utime, &stime);
```

### 27. How do we calculate CPU usage from /proc?

**Answer:**

**Formula:**
```
CPU% = (Δutime + Δstime) / (Δtime × Hz) × 100
```

**Steps:**
1. Read `/proc/[pid]/stat` at time T1 (get utime1, stime1)
2. Wait Δt seconds
3. Read `/proc/[pid]/stat` at time T2 (get utime2, stime2)
4. Get clock ticks per second: `Hz = sysconf(_SC_CLK_TCK)`
5. Calculate: `CPU% = ((utime2+stime2)-(utime1+stime1)) / (Δt × Hz) × 100`

**In Our Code:**
```c
double pm_cpu_percent(const ProcInfo *prev, 
                      const ProcInfo *cur, 
                      double dt_seconds)
{
    long hz = sysconf(_SC_CLK_TCK);
    double ticks = (double)((cur->utime + cur->stime) - 
                            (prev->utime + prev->stime));
    return 100.0 * (ticks / (double)hz) / dt_seconds;
}
```

---

## Our Project

### 28. Explain the architecture of your project.

**Answer:**

**Components:**
1. **Tracer (C):** Core tracing engine using ptrace()
2. **Process Monitor (C):** Reads /proc for process info
3. **Logger (C):** Writes timestamped logs
4. **Syscall Table (C):** Maps syscall numbers to names/categories
5. **Dashboard (Python/Flask):** Web UI for visualization

**Data Flow:**
```
Target Program
    ↓ (traced by)
Tracer (ptrace)
    ↓ (logs to)
Log File + JSON Snapshot
    ↓ (read by)
Dashboard
    ↓ (displays to)
User's Browser
```

**Files:**
- C source: `src/*.c`
- Headers: `include/*.h`
- Dashboard: `dashboard/app.py`, `templates/`, `static/`
- Build: `Makefile`
- Tests: `tests/`

### 29. Why did you use C for the tracer?

**Answer:**

**Reasons:**
1. **System Programming:** Direct access to ptrace(), low-level APIs
2. **Performance:** Minimal overhead for tracing
3. **Portability:** Works on any Linux system with GCC
4. **Educational:** Demonstrates C programming skills
5. **Standard Practice:** System tools (strace, gdb) are written in C

**Alternatives Considered:**
- Python: Too slow, no direct ptrace support
- Go: Good but less common for system tools
- Rust: Modern but steeper learning curve

### 30. Why did you use Flask for the dashboard?

**Answer:**

**Reasons:**
1. **Simplicity:** Easy to set up and run
2. **Python:** Familiar language, good for rapid development
3. **Lightweight:** No heavy framework overhead
4. **JSON API:** Easy to serve JSON data to JavaScript
5. **Educational:** Demonstrates full-stack development

**Requirements:**
- Only needs Python 3 and Flask
- No database required (reads JSON files)
- Single-command startup: `python3 app.py`

---

## Advanced Questions

### 31. What is the difference between your tracer and strace?

**Answer:**

**Similarities:**
- Both use ptrace()
- Both intercept system calls
- Both show syscall arguments and return values

**Differences:**

**strace:**
- More mature and feature-rich
- Decodes all argument types (structures, pointers)
- Handles signals and errors comprehensively
- Command-line only
- Production-ready

**Our Tracer:**
- Educational focus
- Basic argument decoding
- Web dashboard visualization
- Real-time JSON output
- Categorizes syscalls
- Designed for learning

### 32. How would you extend this project?

**Answer:**

**Technical Enhancements:**
1. Decode all syscall arguments (structures, arrays)
2. Measure syscall duration (performance profiling)
3. Filter by syscall name or category
4. Support older kernels (manual register reading)
5. Handle signals comprehensively
6. Multi-threaded process support

**Dashboard Features:**
1. Historical trace comparison
2. Export data to CSV/JSON
3. Advanced search and filtering
4. Timeline visualization
5. Alert system for errors

**Educational:**
1. Interactive tutorials
2. Syscall documentation browser
3. Quiz mode
4. Code annotations

### 33. What challenges did you face?

**Answer:**

**Technical Challenges:**
1. **ptrace Complexity:** Understanding ptrace states and options
2. **Parsing /proc:** Handling variable formats in stat/status files
3. **Architecture Differences:** Syscall numbers differ between x86_64 and ARM
4. **Permissions:** Yama LSM restrictions on tracing
5. **Concurrency:** Handling multiple threads and child processes

**Solutions:**
1. Used PTRACE_GET_SYSCALL_INFO for architecture independence
2. Robust parsing with error checking
3. Extensive testing on Ubuntu x86_64
4. Clear documentation about permission requirements
5. Careful tracking of thread lifecycle

### 34. What did you learn from this project?

**Answer:**

**OS Concepts:**
- Deep understanding of system calls
- Process lifecycle (fork, exec, wait, exit)
- User/kernel mode transitions
- Memory management (mmap, brk)
- /proc virtual filesystem

**Programming Skills:**
- C systems programming
- ptrace API
- Makefile and build systems
- Python Flask web development
- JavaScript and Chart.js

**Software Engineering:**
- Modular design
- Error handling
- Documentation
- Testing
- Full-stack development

### 35. How does context switching work?

**Answer:**

**Context Switch:** The process of saving the state of one process and loading another:

**Steps:**
1. **Save Current Context:**
   - CPU registers
   - Program counter
   - Stack pointer
   - Process state

2. **Update Process State:**
   - Current process: Running → Ready/Waiting
   - Next process: Ready → Running

3. **Load New Context:**
   - Restore registers
   - Restore program counter
   - Switch page tables (virtual memory)
   - Restore stack pointer

4. **Resume Execution:**
   - CPU continues with new process

**Cost:**
- Time: ~1-10 microseconds
- Cache pollution (TLB flush)

**When It Happens:**
- Time slice expires (preemption)
- Process blocks (I/O wait)
- System call completion
- Signal delivery

**In Our Tracer:**
Every ptrace stop causes a context switch between tracee and tracer!

---

These 35 questions cover fundamental OS concepts, system programming, and project-specific details suitable for a college OS project viva examination.
