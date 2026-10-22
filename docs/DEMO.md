# Demo Instructions and Troubleshooting Guide
## Linux System Call Tracer & Process Monitor

---

## 5-Minute Demonstration Script

This guide provides exact commands for demonstrating the project in a presentation or viva.

### Prerequisites Check (30 seconds)

```bash
# Verify you're in the project directory
cd ~/syscall-tracer
pwd  # Should show: /home/muthuram/syscall-tracer

# Verify binaries exist
ls build/syscall-tracer build/test_program

# Check Flask is installed
python3 -c "import flask; print('Flask OK')"
```

**If Flask is missing:**
```bash
pip3 install flask
```

---

## Demonstration Sequence

### Part 1: Build and Basic Trace (1 minute)

**Terminal 1:**
```bash
# Show the project structure
ls -la

# Build if needed (should be pre-built for demo)
make

# Run a basic trace
./build/syscall-tracer --run ./build/test_program
```

**What to highlight:**
- Real-time syscall output on stderr (unless --quiet)
- Different syscall categories appear
- Statistics printed at the end
- Log file created automatically

**Expected output:**
```
[tracer] tracing PID 12345 (./build/test_program) on x86_64, log: logs/trace_...
[12345] openat(path=/tmp/syscall_tracer_demo.txt flags=0x241) = 3
[12345] write(fd=3 count=28) = 28
[12345] close(fd=3) = 0
...
=========== System Call Statistics (x86_64) ===========
Total syscalls  : 159
Unique syscalls : 21
Most frequent   : openat() x62 (39.0%)
```

---

### Part 2: Dashboard Visualization (2 minutes)

**Terminal 1: Start Dashboard**
```bash
cd ~/syscall-tracer
python3 dashboard/app.py
```

**Expected output:**
```
============================================================
  Linux System Call Tracer - Dashboard
============================================================

  Dashboard URL: http://127.0.0.1:5000

  To trace a program, run in another terminal:
    cd ~/syscall-tracer
    ./build/syscall-tracer --run ./build/test_program

  Press Ctrl+C to stop the dashboard
============================================================

 * Running on http://127.0.0.1:5000
```

**Terminal 2: Run Tracer with Dashboard**
```bash
cd ~/syscall-tracer

# Run with default JSON output (logs/latest.json)
./build/syscall-tracer --run ./build/test_program

# Or run with quiet mode
./build/syscall-tracer --run ./build/test_program --quiet
```

**In Web Browser:**

Open: **http://127.0.0.1:5000**

**Walk through these features:**

1. **Dashboard Tab (Overview)**
   - Point out the 4 stat cards (CPU, Memory, Processes, Syscalls/sec)
   - Show "Current Process" section with PID, name, state
   - Scroll to "Recent System Calls" table
   - Show "System Call Frequency" bar chart

2. **Processes Tab**
   - Show process tree visualization
   - Explain parent-child relationships

3. **System Calls Tab**
   - Show category breakdown (5 colored cards)
   - Explain the syscall execution flow diagram
   - Point out user space → kernel transition

4. **Memory Tab**
   - Show CPU and memory usage graph over time

5. **Statistics Tab**
   - Show total/unique syscalls
   - Most frequent syscall
   - Architecture information
   - Table of top syscalls with percentages

6. **Logs Tab**
   - Log file path
   - Command that was traced
   - Start time and status
   - Milestone events

7. **About Tab**
   - Educational information
   - OS concepts explained

---

### Part 3: Different Test Modes (1 minute)

**Terminal 2: Demonstrate specific syscall categories**

```bash
# File I/O operations only
./build/syscall-tracer --run ./build/test_program io --quiet

# Memory operations (malloc, brk, mmap)
./build/syscall-tracer --run ./build/test_program mem --quiet

# Process creation (fork, wait)
./build/syscall-tracer --run ./build/test_program fork --quiet

# Process execution (fork, exec, wait)
./build/syscall-tracer --run ./build/test_program exec --quiet
```

**What to show:**
- Dashboard updates in real-time
- Different syscalls appear for each mode
- Category counts change based on test mode

---

### Part 4: Process Monitoring (30 seconds)

**Terminal 3: Monitor a running process (no ptrace)**

```bash
# Monitor your own shell ($$  = current PID)
./build/syscall-tracer --monitor $$ --interval 1

# Press Ctrl+C after a few refreshes
```

**What to highlight:**
- Uses /proc filesystem (no ptrace)
- Shows CPU%, memory, threads, context switches
- Refreshes every second
- No permission issues

---

### Part 5: Log Analysis (30 seconds)

**Terminal 2: Generate statistics from existing log**

```bash
# List available logs
ls -lh logs/trace_*.log

# Analyze a log file
./build/syscall-tracer --stats logs/trace_20261128_233023.log
```

**What to show:**
- Can analyze logs offline
- Statistics match original run
- Useful for comparing traces

---

## Quick Demo Commands (Copy-Paste Ready)

### One-Terminal Quick Demo

```bash
cd ~/syscall-tracer

# 1. Build
make

# 2. Basic trace
./build/syscall-tracer --run ./build/test_program --quiet

# 3. Show log
ls -lh logs/trace_*.log

# 4. Statistics
./build/syscall-tracer --stats $(ls logs/trace_*.log | tail -1)

# 5. Process monitor
timeout 5 ./build/syscall-tracer --monitor $$
```

### Two-Terminal Dashboard Demo

**Terminal 1:**
```bash
cd ~/syscall-tracer && python3 dashboard/app.py
```

**Terminal 2:**
```bash
cd ~/syscall-tracer

# Run multiple traces to see updates
for mode in all io mem fork exec; do
    echo "=== Testing $mode mode ==="
    ./build/syscall-tracer --run ./build/test_program $mode --quiet
    sleep 1
done
```

**Browser:** Open http://127.0.0.1:5000

---

## Troubleshooting Guide

### Problem 1: "Command not found" or "No such file or directory"

**Symptom:**
```bash
bash: ./build/syscall-tracer: No such file or directory
```

**Cause:** Project not built

**Solution:**
```bash
cd ~/syscall-tracer
make clean
make
ls build/  # Verify binaries exist
```

---

### Problem 2: Flask Import Error

**Symptom:**
```
ModuleNotFoundError: No module named 'flask'
```

**Cause:** Flask not installed

**Solution:**
```bash
# Install Flask
pip3 install flask

# Or use requirements file
pip3 install -r dashboard/requirements.txt

# Verify installation
python3 -c "import flask; print('Flask', flask.__version__)"
```

---

### Problem 3: Dashboard Shows "No Data" or "Demo Mode"

**Symptom:**
- Dashboard displays but shows no real data
- Status indicator shows "Demo Mode"

**Cause:** No trace is running or JSON file missing

**Solution:**
```bash
# Ensure tracer is running with JSON output (default)
./build/syscall-tracer --run ./build/test_program

# Check JSON file exists and is recent
ls -lh logs/latest.json
cat logs/latest.json | head -20

# Verify tracer uses correct JSON path
./build/syscall-tracer --run ./build/test_program --json logs/latest.json
```

---

### Problem 4: "Operation not permitted" Error

**Symptom:**
```
error: PTRACE_ATTACH to 12345 failed: Operation not permitted
```

**Cause:** Linux Yama LSM restricts ptrace

**Check Yama setting:**
```bash
cat /proc/sys/kernel/yama/ptrace_scope
```

**Values:**
- `0` = Any process can trace any other (least secure)
- `1` = Only parent/child processes (default)
- `2` = Admin only
- `3` = No tracing allowed

**Solutions:**

**Option 1: Use --run instead of --trace (Recommended)**
```bash
# This creates child process, always allowed
./build/syscall-tracer --run ./build/test_program
```

**Option 2: Temporarily allow tracing (requires sudo)**
```bash
# WARNING: Reduces security temporarily
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope

# Run your trace
./build/syscall-tracer --trace <PID>

# Restore security
echo 1 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```

**Option 3: Run as root (not recommended for demo)**
```bash
sudo ./build/syscall-tracer --trace <PID>
```

---

### Problem 5: "PTRACE_GET_SYSCALL_INFO failed"

**Symptom:**
```
warning: PTRACE_GET_SYSCALL_INFO failed for 12345: Invalid argument
```

**Cause:** Kernel older than Linux 5.3

**Check kernel version:**
```bash
uname -r
```

**Solution:**
- Upgrade to Linux 5.3 or later
- Use Ubuntu 20.04+ which has kernel 5.4+
- Or modify code to read registers manually (architecture-specific)

---

### Problem 6: Test Program Doesn't Generate Many Syscalls

**Symptom:**
Only seeing 50-100 syscalls instead of expected amount

**Cause:** This is normal! Modern Linux optimizes syscalls:
- vDSO for time functions (no real syscall)
- Buffered I/O in libc
- Dynamic linker caching

**Verification:**
```bash
# Compare with strace
strace -c ./build/test_program 2>&1 | head -20
```

**Not a problem:** Both tools show similar low counts

---

### Problem 7: Dashboard Not Updating

**Symptom:**
Dashboard loads but data doesn't refresh

**Causes and Solutions:**

**1. JavaScript Error**
```
# Open browser console (F12) and check for errors
# Common fix: Clear cache and reload (Ctrl+Shift+R)
```

**2. JSON File Not Updating**
```bash
# Check if tracer is still running
ps aux | grep syscall-tracer

# Check JSON modification time
ls -lh logs/latest.json
stat logs/latest.json

# Watch for updates
watch -n 1 'ls -lh logs/latest.json'
```

**3. Port Already in Use**
```bash
# Check if port 5000 is in use
lsof -i :5000

# Kill existing process
kill <PID>

# Or use different port
python3 dashboard/app.py
# Edit app.py: app.run(port=5001)
```

---

### Problem 8: Permission Denied for Log Directory

**Symptom:**
```
error: cannot create log directory 'logs': Permission denied
```

**Solution:**
```bash
# Create with correct permissions
mkdir -p logs
chmod 755 logs

# Or use different directory
./build/syscall-tracer --run ./build/test_program --log-dir /tmp/my_logs
```

---

### Problem 9: Compilation Errors

**Symptom:**
```
error: 'PTRACE_GET_SYSCALL_INFO' undeclared
```

**Cause:** Missing headers or old system

**Solution:**
```bash
# Install build essentials
sudo apt-get update
sudo apt-get install build-essential

# Check GCC version (need 9.0+)
gcc --version

# Install specific version if needed
sudo apt-get install gcc-11
```

---

### Problem 10: Browser Shows "Connection Refused"

**Symptom:**
Cannot access http://127.0.0.1:5000

**Causes and Solutions:**

**1. Dashboard Not Running**
```bash
# Start dashboard
python3 dashboard/app.py
```

**2. Wrong URL**
```
# Use correct URL
http://127.0.0.1:5000  ✓
http://localhost:5000  ✓
http://127.0.0.1/      ✗ (missing port)
```

**3. Firewall Blocking**
```bash
# Check if process is listening
netstat -tlnp | grep 5000

# Or use ss
ss -tlnp | grep 5000
```

---

## Pre-Demo Checklist

### Before Your Presentation

**1 Day Before:**
- [ ] Test entire demo sequence
- [ ] Verify all commands work
- [ ] Check dashboard loads correctly
- [ ] Prepare backup VM/container if needed
- [ ] Screenshot key dashboard views (backup if demo fails)

**1 Hour Before:**
- [ ] Reboot system (fresh start)
- [ ] Rebuild project: `make clean && make`
- [ ] Clear old logs: `rm logs/trace_*.log` (optional)
- [ ] Test dashboard: `python3 dashboard/app.py`
- [ ] Run test trace: `./build/syscall-tracer --run ./build/test_program`
- [ ] Close unnecessary applications

**5 Minutes Before:**
- [ ] Open terminals (2-3 pre-positioned)
- [ ] Navigate to project directory in all terminals
- [ ] Have browser tab ready at http://127.0.0.1:5000
- [ ] Close unrelated browser tabs
- [ ] Silence notifications
- [ ] Check volume if showing videos

---

## Common Questions During Demo

### Q: "Can you trace any program?"

**A:** Almost any program you own:
```bash
# Yes - trace any CLI program
./build/syscall-tracer --run ls -la
./build/syscall-tracer --run python3 -c "print('hello')"

# Yes - trace long-running programs
./build/syscall-tracer --run sleep 10

# Limited - need permissions for existing processes
./build/syscall-tracer --trace <PID>  # May fail due to Yama
```

### Q: "How is this different from strace?"

**A:** Show side-by-side:
```bash
# strace output (text only)
strace -c ./build/test_program

# Our tracer (text + JSON + dashboard)
./build/syscall-tracer --run ./build/test_program
# Then show dashboard in browser
```

**Explain:**
- strace: Production debugging tool
- Our tracer: Educational tool with visualization

### Q: "Can you filter syscalls?"

**A:** Not yet, but demonstrate workaround:
```bash
# Filter log with grep
./build/syscall-tracer --run ./build/test_program --quiet > /dev/null
grep "openat\|read\|write" logs/trace_*.log | tail -20
```

### Q: "What overhead does tracing add?"

**A:** Demonstrate:
```bash
# Without tracing
time ./build/test_program

# With tracing
time ./build/syscall-tracer --run ./build/test_program --quiet > /dev/null

# Typically 5-10x slower (acceptable for debugging/learning)
```

### Q: "Can you trace multi-threaded programs?"

**A:** Yes! Demonstrate:
```bash
# The tracer automatically follows threads (clone syscalls)
# Test program creates child process which is traced
./build/syscall-tracer --run ./build/test_program exec

# Check thread list in dashboard
# Or in logs:
grep "clone\|fork" logs/trace_*.log
```

---

## Emergency Fallback Plan

### If Live Demo Fails

**Have These Ready:**

1. **Screenshots:**
   - Pre-captured dashboard views
   - Terminal output with statistics
   - Log file samples

2. **Pre-recorded Video:**
   - 2-minute screencast showing full demo
   - Narrated walkthrough

3. **Backup Commands:**
```bash
# Show existing log
cat logs/trace_20261128_233023.log | head -30

# Show statistics from old log
./build/syscall-tracer --stats logs/trace_*.log

# Show JSON snapshot
cat logs/latest.json | python3 -m json.tool | head -50
```

4. **Explanation Mode:**
   - Walk through code instead
   - Explain architecture with diagrams
   - Show test results from earlier
   - Demo on examiner's machine

---

## Post-Demo Cleanup

```bash
# Stop dashboard (Ctrl+C in Terminal 1)

# Optional: Clean up logs
rm logs/trace_*.log
rm logs/latest.json

# Or keep for archival
mkdir -p ~/demo_backup_$(date +%Y%m%d)
cp -r logs ~/demo_backup_$(date +%Y%m%d)/
```

---

## Success Indicators

### Your Demo Was Successful If:

✓ Tracer compiled without errors  
✓ Test program ran and generated syscalls  
✓ Dashboard loaded and displayed data  
✓ Statistics were accurate  
✓ Charts updated in real-time  
✓ You explained key OS concepts  
✓ Audience understood the project  
✓ Questions were answered confidently  

---

**Good luck with your demonstration!**

For additional help, refer to:
- README.md - Complete documentation
- docs/viva.md - Q&A preparation
- docs/presentation.md - Presentation slides
