#!/bin/bash
# run_tests.sh - automated test suite for syscall-tracer

set -e
TRACER=./build/syscall-tracer
TEST_PROG=./build/test_program
PASS=0
FAIL=0

echo "========================================"
echo "  syscall-tracer Test Suite"
echo "========================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    PASS=$((PASS + 1))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    FAIL=$((FAIL + 1))
}

info() {
    echo -e "${YELLOW}→${NC} $1"
}

# Test 1: Binary exists
echo "Test 1: Check binaries exist"
if [ -f "$TRACER" ] && [ -f "$TEST_PROG" ]; then
    pass "Binaries exist"
else
    fail "Binaries not found (run 'make' first)"
    exit 1
fi

# Test 2: Help command
echo ""
echo "Test 2: Help command"
if $TRACER --help > /dev/null 2>&1; then
    pass "Help command works"
else
    fail "Help command failed"
fi

# Test 3: Test program runs standalone
echo ""
echo "Test 3: Test program runs standalone"
if $TEST_PROG > /dev/null 2>&1; then
    pass "Test program executes successfully"
else
    fail "Test program failed to execute"
fi

# Test 4: Trace test program (basic)
echo ""
echo "Test 4: Trace test program (basic run)"
# Note: Using default logs/ directory since --log-dir has known limitation
if timeout 10 $TRACER --run $TEST_PROG --quiet > /tmp/trace_out_$$.txt 2>&1; then
    pass "Traced test program successfully"
    
    # Check log file was created in default logs/ directory
    LOG_FILE=$(ls -t logs/trace_*.log 2>/dev/null | head -1)
    if [ -f "$LOG_FILE" ]; then
        pass "Log file created"
        
        # Check for expected system calls
        if grep -q "openat\|open" $LOG_FILE && grep -q "read\|write" $LOG_FILE; then
            pass "Log contains expected syscalls (open, read/write)"
        else
            fail "Log missing expected syscalls"
        fi
        
        # Check for statistics in output
        if grep -q "System Call Statistics" /tmp/trace_out_$$.txt; then
            pass "Statistics generated"
        else
            fail "No statistics in output"
        fi
    else
        fail "No log file created"
    fi
else
    fail "Tracer failed to execute"
fi

# Test 5: Different test modes
echo ""
echo "Test 5: Test program modes"
for mode in io mem fork exec; do
    if timeout 5 $TRACER --run $TEST_PROG $mode --quiet > /dev/null 2>&1; then
        pass "Mode '$mode' works"
    else
        fail "Mode '$mode' failed"
    fi
done

# Test 6: Statistics from log
echo ""
echo "Test 6: Generate statistics from log file"
if [ -f "$LOG_FILE" ]; then
    if $TRACER --stats $LOG_FILE > /tmp/stats_$$.txt 2>&1; then
        pass "Statistics command works"
        if grep -q "Total syscalls" /tmp/stats_$$.txt; then
            pass "Statistics output valid"
        else
            fail "Statistics output invalid"
        fi
    else
        fail "Statistics command failed"
    fi
fi

# Test 7: Monitor mode (quick test on self)
echo ""
echo "Test 7: Monitor mode"
if timeout 2 $TRACER --monitor $$ --interval 1 > /tmp/monitor_$$.txt 2>&1 || [ $? -eq 124 ]; then
    if grep -q "PID" /tmp/monitor_$$.txt; then
        pass "Monitor mode works"
    else
        fail "Monitor mode output invalid"
    fi
else
    fail "Monitor mode failed"
fi

# Test 8: Compare with strace (if available)
echo ""
echo "Test 8: Compare with strace"
if command -v strace &> /dev/null; then
    info "strace available, running comparison"
    
    # Run strace
    strace -c $TEST_PROG 2> /tmp/strace_$$.txt > /dev/null || true
    
    # Run our tracer
    $TRACER --run $TEST_PROG --quiet > /tmp/our_tracer_$$.txt 2>&1 || true
    
    # Check both captured some syscalls
    STRACE_COUNT=$(grep -c "calls" /tmp/strace_$$.txt 2>/dev/null || echo "0")
    OUR_COUNT=$(grep -c "Total syscalls" /tmp/our_tracer_$$.txt 2>/dev/null || echo "0")
    
    if [ "$STRACE_COUNT" -gt "0" ] && [ "$OUR_COUNT" -gt "0" ]; then
        pass "Both strace and our tracer captured syscalls"
        info "strace found syscalls, our tracer found syscalls"
    else
        fail "Comparison issue: strace=$STRACE_COUNT our=$OUR_COUNT"
    fi
else
    info "strace not available, skipping comparison"
fi

# Test 9: JSON output
echo ""
echo "Test 9: JSON output for dashboard"
# Check if latest.json is created in default logs/ directory
if timeout 5 $TRACER --run $TEST_PROG --quiet > /dev/null 2>&1; then
    JSON_FILE="logs/latest.json"
    if [ -f "$JSON_FILE" ]; then
        pass "JSON file created"
        if grep -q "\"schema\"" $JSON_FILE && grep -q "\"total\"" $JSON_FILE; then
            pass "JSON structure valid"
        else
            fail "JSON structure invalid"
        fi
    else
        fail "JSON file not created"
    fi
else
    fail "JSON generation failed"
fi

# Test 10: Invalid PID handling
echo ""
echo "Test 10: Error handling"
if $TRACER --monitor 999999 2>&1 | grep -q "error"; then
    pass "Invalid PID rejected properly"
else
    fail "Invalid PID not handled"
fi

# Cleanup
rm -f /tmp/trace_out_$$.txt /tmp/stats_$$.txt /tmp/monitor_$$.txt /tmp/strace_$$.txt /tmp/our_tracer_$$.txt

# Summary
echo ""
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo -e "${GREEN}Passed: $PASS${NC}"
echo -e "${RED}Failed: $FAIL${NC}"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
