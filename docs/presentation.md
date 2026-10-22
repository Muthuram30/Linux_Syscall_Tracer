# Linux System Call Tracer & Process Monitor
## Operating Systems Project Presentation

---

## Slide 1: Title Slide

### Linux System Call Tracer & Process Monitor
**An Educational Operating Systems Project**

**Technologies:**
- C Programming (ptrace, /proc)
- Python Flask (Web Dashboard)
- Linux System Programming

**Presented by:** [Your Name]  
**Course:** Operating Systems  
**Date:** [Current Date]

---

## Slide 2: Problem Statement

### The Challenge

**Understanding OS Internals is Abstract**
- System calls happen invisibly
- Process lifecycle is hidden from users
- Resource usage is difficult to observe in real-time
- Learning tools are often complex or inaccessible

**Our Solution:**
Build an educational tool that makes operating system concepts visible and understandable through real-time tracing and visualization.

---

## Slide 3: Project Objectives

### What We Aimed to Achieve

1. **Implement System Call Tracing**
   - Use Linux `ptrace()` to intercept syscalls
   - Capture entry and exit points
   - Record arguments and return values

2. **Monitor Process Information**
   - Read from `/proc` virtual filesystem
   - Track CPU and memory usage
   - Monitor process lifecycle

3. **Visualize OS Concepts**
   - Real-time web dashboard
   - Educational flow diagrams
   - Statistical analysis

4. **Demonstrate Core OS Principles**
   - User/kernel mode transitions
   - Process management
   - Memory management
   - File I/O operations

---

## Slide 4: OS Concepts Covered

### Fundamental Operating System Topics

**System Calls**
- Interface between user programs and kernel
- Mode switching (user → kernel → user)
- System call numbers and categories

**Process Management**
- `fork()` - Process creation
- `exec()` - Program execution
- `wait()` - Process synchronization
- `exit()` - Process termination

**Memory Management**
- `mmap()` - Virtual memory mapping
- `brk()` - Heap management
- Virtual address spaces

**File I/O**
- `open()`, `read()`, `write()`, `close()`
- File descriptors
- Directory operations

---

## Slide 5: System Architecture

### High-Level Design

```
┌─────────────────────────────────────────┐
│           User Space                     │
│  ┌──────────────┐    ┌──────────────┐  │
│  │ Test Program │◄───│   Tracer     │  │
│  │              │    │   (ptrace)   │  │
│  └──────┬───────┘    └──────┬───────┘  │
│         │ syscalls          │           │
├─────────┼───────────────────┼───────────┤
│         ▼                   ▼           │
│    System Call Interface                │
├─────────────────────────────────────────┤
│           Kernel Space                   │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐│
│  │ Process │  │ Memory  │  │  File   ││
│  │ Manager │  │ Manager │  │ System  ││
│  └─────────┘  └─────────┘  └─────────┘│
└─────────────────────────────────────────┘
         │                    │
         ▼                    ▼
  ┌─────────────┐      ┌──────────────┐
  │    /proc    │      │  Dashboard   │
  │ Filesystem  │      │   (Flask)    │
  └─────────────┘      └──────────────┘
```

**Three-Tier Architecture:**
1. **Tracer (C)** - Core tracing engine
2. **Data Layer** - Logs and JSON snapshots
3. **Dashboard (Python/Flask)** - Visualization

---

## Slide 6: Technology Stack

### Tools and Technologies Used

**Backend (C)**
- GCC compiler
- ptrace() system call API
- /proc filesystem
- GNU Make build system

**Dashboard (Python)**
- Flask web framework
- JSON API endpoints
- Real-time data updates

**Frontend (Web)**
- HTML5 / CSS3
- JavaScript (ES6+)
- Chart.js for visualization
- Responsive dark theme design

**Development Tools**
- Linux Ubuntu 24.04
- VS Code / Vim
- Git version control
- strace (for comparison testing)

---

## Slide 7: How ptrace() Works

### Process Tracing Mechanism

**Setup:**
```c
pid_t pid = fork();
if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execvp(program, args);
}
```

**Tracing Loop:**
```
1. Child executes → Kernel stops child
2. Tracer calls waitpid() → Gets notification
3. Tracer inspects syscall → PTRACE_GET_SYSCALL_INFO
4. Tracer continues child → PTRACE_SYSCALL
5. Repeat for every syscall entry and exit
```

**Key Features:**
- Two stops per syscall (entry + exit)
- Architecture-independent (Linux 5.3+)
- Auto-trace child processes (fork/clone)
- Read tracee memory for arguments

---

## Slide 8: System Call Categories

### Classification of System Calls

**We categorize syscalls into 5 groups:**

| Category | Examples | Purpose |
|----------|----------|---------|
| **File** | open, read, write, close, stat | File operations |
| **Process** | fork, exec, wait, exit, getpid | Process control |
| **Memory** | mmap, munmap, brk, mprotect | Memory management |
| **I/O** | poll, select, epoll, ioctl | I/O multiplexing |
| **Other** | clock_gettime, getuid, uname | Misc operations |

**Benefits:**
- Easier to understand syscall patterns
- Identify bottlenecks by category
- Educational classification

---

## Slide 9: /proc Filesystem

### Virtual Filesystem for Process Information

**What is /proc?**
- Virtual filesystem (not on disk)
- Kernel generates files on-the-fly
- Real-time process and system information

**Key Files We Use:**
```
/proc/[pid]/stat       → Process statistics
/proc/[pid]/status     → Detailed status
/proc/[pid]/cmdline    → Command line
/proc/[pid]/comm       → Process name
/proc/meminfo          → System memory
/proc/stat             → System boot time
```

**Advantages:**
- No special APIs needed
- Standard file operations (open, read)
- Human-readable format
- Updated in real-time

---

## Slide 10: Process Lifecycle Demo

### fork() → exec() → wait() → exit()

**The Process Family Tree:**

```
Parent Process (PID: 1234)
    │
    │ fork() creates child
    ├─────────────────────┐
    │                     │
    │                Child Process (PID: 5678)
    │                     │
    │                     │ execve("/bin/echo")
    │                     │ [Becomes echo program]
    │                     │
    │ wait4(5678)         │ [echo executes]
    │ [blocked]           │
    │                     │ exit(0)
    │◄────────────────────┘
    │ [wait4 returns]
    │ [child reaped]
    │
    │ exit_group(0)
    ▼
[Process terminates]
```

**Our tracer captures all these transitions!**

---

## Slide 11: Dashboard Features

### Interactive Web Visualization

**Real-Time Monitoring:**
- Live system call stream (updates every second)
- CPU and memory usage graphs
- Process information display
- Active thread tracking

**Statistical Analysis:**
- Total syscalls executed
- Most frequent syscalls
- Category breakdown (pie/bar charts)
- Top 10 syscalls by count

**Educational Visualizations:**
- System call execution flow diagram
- Process tree hierarchy
- User space ↔ Kernel space transitions
- Milestone events timeline

**User Experience:**
- Dark theme professional design
- Responsive layout
- Sidebar navigation
- No-data demo mode

---

## Slide 12: Project Structure

### Modular and Well-Organized

```
syscall-tracer/
├── src/               C source files
│   ├── main.c         CLI interface
│   ├── syscall_tracer.c   Core tracer
│   ├── process_monitor.c  /proc reader
│   ├── logger.c       Log writer
│   └── syscall_table.c    Syscall lookup
├── include/           Header files
├── tests/             Test programs
├── dashboard/         Flask web app
│   ├── app.py        Backend
│   ├── templates/    HTML
│   └── static/       CSS/JS
├── logs/             Generated logs
├── build/            Compiled binaries
└── docs/             Documentation
```

**Key Design Principles:**
- Separation of concerns
- Modular architecture
- Easy to extend
- Well-documented

---

## Slide 13: Implementation Highlights

### Key Technical Achievements

**1. Architecture-Independent Tracing**
- Uses `PTRACE_GET_SYSCALL_INFO` (modern API)
- Works on x86_64 and aarch64
- No manual register reading

**2. Automatic Child Process Tracking**
- `PTRACE_O_TRACEFORK` option
- Follows fork(), vfork(), clone()
- Maintains process tree

**3. Syscall Argument Decoding**
- Reads strings from tracee memory
- Decodes file paths, file descriptors
- Shows meaningful context

**4. Real-Time JSON Snapshots**
- Live data for dashboard
- Atomic file updates
- 250ms refresh rate

**5. Robust Error Handling**
- Permission checks
- Process termination handling
- Signal pass-through

---

## Slide 14: Testing Strategy

### Comprehensive Test Suite

**Automated Tests (13 tests):**
```bash
make test
```

✓ Binary compilation  
✓ Help command  
✓ Standalone execution  
✓ Tracing with different modes  
✓ Statistics generation  
✓ Monitor mode  
✓ strace comparison  
✓ JSON output  
✓ Error handling  

**Test Program:**
- Exercises file I/O
- Tests memory allocation
- Demonstrates fork/exec/wait
- Includes sleep and exit

**Validation:**
- Compare with strace output
- Verify log file format
- Check JSON schema
- Test permission handling

---

## Slide 15: Experimental Results

### Performance and Statistics

**Sample Trace Output:**

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

Top syscalls:
  openat        62   39.0%
  newfstatat    62   39.0%
  mmap           8    5.0%
  mprotect       4    2.5%
===================================================
```

**Observations:**
- File operations dominate (80.5%)
- Dynamic linking causes many openat calls
- Memory operations are infrequent but important

---

## Slide 16: Comparison with strace

### Our Tracer vs. Industry Standard

| Feature | strace | Our Tracer |
|---------|--------|------------|
| **Purpose** | Production debugging | Educational tool |
| **Argument Decoding** | Complete | Basic |
| **Output Format** | Text only | Text + JSON |
| **Visualization** | None | Web dashboard |
| **Statistics** | Summary mode | Categorized + charts |
| **Real-time UI** | No | Yes |
| **Learning Curve** | Steep | Gentle |

**strace Example:**
```bash
$ strace -c ./test_program
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 54.23    0.000045          45         1           execve
 ...
```

**Our Advantage:** Educational focus with visualization!

---

## Slide 17: Challenges Faced

### Technical Obstacles and Solutions

**Challenge 1: ptrace Complexity**
- **Problem:** ptrace has many states and edge cases
- **Solution:** Used modern PTRACE_GET_SYSCALL_INFO API
- **Lesson:** Prefer high-level APIs when available

**Challenge 2: Permission Restrictions**
- **Problem:** Yama LSM blocks attaching to processes
- **Solution:** Use --run mode (parent-child allowed)
- **Lesson:** Document security limitations clearly

**Challenge 3: Parsing /proc Files**
- **Problem:** Variable formats, error handling
- **Solution:** Robust parsing with error checks
- **Lesson:** Never trust external data format

**Challenge 4: Thread Tracking**
- **Problem:** Multiple threads, parent-child relationships
- **Solution:** Maintain thread table with lifecycle tracking
- **Lesson:** Data structures are crucial for stateful tracing

---

## Slide 18: What We Learned

### Educational Outcomes

**Operating Systems:**
- Deep understanding of system calls
- User/kernel mode transitions
- Process lifecycle management
- Virtual memory concepts
- /proc filesystem internals

**Systems Programming:**
- C language expertise
- ptrace API mastery
- Error handling in systems code
- Build systems (Makefile)
- Debugging techniques

**Full-Stack Development:**
- Python Flask web framework
- REST API design
- JavaScript and Chart.js
- Responsive UI design
- Real-time data updates

**Software Engineering:**
- Modular design principles
- Documentation best practices
- Test-driven development
- Version control (Git)

---

## Slide 19: Live Demonstration

### Demo Sequence (5 minutes)

**Terminal 1: Start Dashboard**
```bash
cd ~/syscall-tracer
python3 dashboard/app.py
# Dashboard opens at http://127.0.0.1:5000
```

**Terminal 2: Run Tracer**
```bash
./build/syscall-tracer --run ./build/test_program
```

**Show in Browser:**
1. System overview cards updating
2. Process information display
3. System call stream (live table)
4. Frequency bar chart
5. Category breakdown
6. Memory usage graph
7. Process tree
8. Statistics page

**Terminal 3: Monitor a Process**
```bash
./build/syscall-tracer --monitor $$
```

---

## Slide 20: Demo Commands Summary

### Quick Reference for Live Demo

**Build the Project:**
```bash
cd ~/syscall-tracer
make
```

**Run Tests:**
```bash
make test
```

**Trace the Test Program:**
```bash
./build/syscall-tracer --run ./build/test_program
```

**Start Dashboard:**
```bash
python3 dashboard/app.py
# Open: http://127.0.0.1:5000
```

**Different Test Modes:**
```bash
./build/test_program io      # File I/O
./build/test_program mem     # Memory ops
./build/test_program fork    # Process creation
./build/test_program exec    # Program execution
```

**Generate Statistics:**
```bash
./build/syscall-tracer --stats logs/trace_*.log
```

---

## Slide 21: Limitations

### Current Constraints

**Technical Limitations:**
1. **Kernel Version:** Requires Linux 5.3+ for modern API
2. **Performance:** ptrace adds ~5-10x overhead (not for production)
3. **Permissions:** Limited by Linux security policies (Yama)
4. **Architecture:** Tested primarily on x86_64
5. **vDSO:** Some syscalls bypass tracing (gettimeofday)

**Feature Limitations:**
1. **Argument Decoding:** Only basic types (strings, integers)
2. **Structures:** Complex structures not decoded
3. **Signals:** Basic signal handling only
4. **Filtering:** No syscall filtering yet
5. **Performance Metrics:** No duration measurement

**Not Issues, Just Scope:**
- Project is educational, not production-ready
- Focus on understanding concepts
- Extensible design for future enhancements

---

## Slide 22: Future Enhancements

### Roadmap for Improvement

**Technical Features:**
- [ ] Support older kernels (< 5.3) with register reading
- [ ] Complete argument decoding (all types)
- [ ] Syscall duration profiling
- [ ] Filter by syscall name/category
- [ ] Network syscall support (socket operations)
- [ ] Better thread visualization

**Dashboard Improvements:**
- [ ] Multiple trace comparison
- [ ] Export to CSV/JSON
- [ ] Advanced search and filtering
- [ ] Timeline view with zoom
- [ ] Dark/light theme toggle
- [ ] Mobile-responsive design

**Educational Content:**
- [ ] Interactive syscall explorer
- [ ] Built-in tutorials
- [ ] Quiz mode for learning
- [ ] Video explanations
- [ ] Code annotations

---

## Slide 23: Real-World Applications

### Practical Uses of System Call Tracing

**Debugging:**
- Identify failing system calls
- Debug permission issues
- Trace library dependencies

**Performance Analysis:**
- Find I/O bottlenecks
- Identify excessive syscalls
- Optimize system call patterns

**Security:**
- Detect malicious behavior
- Monitor system call sequences
- Sandbox applications (seccomp)

**Reverse Engineering:**
- Understand program behavior
- Analyze closed-source software
- Document undocumented protocols

**Education:**
- Teaching OS concepts
- Understanding program execution
- Learning systems programming

---

## Slide 24: Key Takeaways

### Summary of the Project

**What We Built:**
✓ Functional system call tracer using ptrace()  
✓ Process monitor using /proc filesystem  
✓ Real-time web dashboard with visualizations  
✓ Comprehensive test suite  
✓ Extensive documentation  

**What We Demonstrated:**
✓ Understanding of OS fundamentals  
✓ Systems programming skills in C  
✓ Full-stack development capabilities  
✓ Software engineering best practices  
✓ Problem-solving and debugging skills  

**Educational Value:**
- Makes abstract OS concepts concrete
- Provides hands-on learning experience
- Bridges theory and practice
- Suitable for teaching and presentations

**Code Quality:**
- Modular and maintainable
- Well-documented and tested
- Follows industry standards
- Ready for extension

---

## Slide 25: Conclusion

### Final Thoughts

**Project Success:**
- ✅ All objectives achieved
- ✅ Comprehensive implementation
- ✅ Educational and practical value
- ✅ Extensible architecture

**Learning Experience:**
- Deep dive into Linux internals
- Mastery of system programming
- Full-stack development skills
- Software engineering discipline

**Future Direction:**
- Continue adding features
- Share as open-source educational tool
- Potential research applications
- Teaching material for OS courses

**Thank You!**

Questions?

---

## Slide 26: References

### Documentation and Resources

**Project Documentation:**
- README.md - Complete user guide
- docs/viva.md - 35 Q&A for examination
- Code comments - Inline explanations

**Man Pages:**
- `man 2 ptrace` - Process tracing
- `man 5 proc` - /proc filesystem
- `man 2 syscalls` - System call list
- `man 2 fork` - Process creation

**Books:**
- "The Linux Programming Interface" - Michael Kerrisk
- "Advanced Programming in the UNIX Environment" - Stevens
- "Operating System Concepts" - Silberschatz

**Online Resources:**
- Linux Kernel Documentation
- PTRACE_GET_SYSCALL_INFO documentation
- Flask framework documentation

**Source Code:**
- Available at: `~/syscall-tracer`
- Well-commented and modular
- MIT License (educational use)

---

## Backup Slides

### Additional Technical Details

*[Include here if questions arise about specific implementation details]*

---

**End of Presentation**

*Total Slides: 26*  
*Estimated Duration: 20-25 minutes*  
*Format: Technical presentation suitable for college project evaluation*
