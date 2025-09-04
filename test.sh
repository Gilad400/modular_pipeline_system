#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[TEST]${NC} $1"
}
print_error() {
    echo -e "${RED}[TEST ERROR]${NC} $1"
}
print_success() {
    echo -e "${GREEN}[TEST PASS]${NC} $1"
}
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}
print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

display_test_category() {
    echo ""
    echo "============================================"
    echo "$1"
    echo "============================================"
    echo ""
}

# Test counter
TESTS_PASSED=0
TESTS_TOTAL=0

# Function to check test results
check_test_result() {
    local test_name="$1"
    local expected="$2"
    local actual="$3"
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    if [ "$actual" == "$expected" ]; then
        print_success "$test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        print_error "$test_name"
        print_warning "Wanted: '$expected'"
        print_warning "Received: '$actual'"
        return 1
    fi
}

# Check for timeout utility
TIMEOUT_UTIL=""
if command -v timeout >/dev/null 2>&1; then
    TIMEOUT_UTIL="timeout"
elif command -v gtimeout >/dev/null 2>&1; then
    TIMEOUT_UTIL="gtimeout"
fi

print_status "Starting comprehensive test suite..."

# First, build the project
print_status "Building project..."
./build.sh || {
    print_error "Build failed"
    exit 1
}

display_test_category "Input Validation and Error Handling"

# No arguments
TESTS_TOTAL=$((TESTS_TOTAL + 1))
./output/analyzer >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    print_success "Missing Arguments Detection"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Missing Arguments Detection: wanted exit code 1, received $EXIT_CODE"
fi

# Duplicate plugin detection
TESTS_TOTAL=$((TESTS_TOTAL + 1))
./output/analyzer 10 logger logger >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    print_success "Duplicate Plugin Detection"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Duplicate Plugin Detection: wanted exit code 1, received $EXIT_CODE"
fi

# Invalid queue size (non-numeric)
TESTS_TOTAL=$((TESTS_TOTAL + 1))
./output/analyzer abc logger >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    print_success "Invalid Queue Size Format Detection"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Invalid Queue Size Format: wanted exit code 1, received $EXIT_CODE"
fi

# Zero queue size
TESTS_TOTAL=$((TESTS_TOTAL + 1))
./output/analyzer 0 logger >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    print_success "Zero Queue Size Detection"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Zero Queue Size: wanted exit code 1, received $EXIT_CODE"
fi

# Negative queue size
TESTS_TOTAL=$((TESTS_TOTAL + 1))
./output/analyzer -8 logger >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    print_success "Negative Queue Size Detection"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Negative Queue Size: wanted exit code 1, received $EXIT_CODE"
fi

# Non-existent plugin
TESTS_TOTAL=$((TESTS_TOTAL + 1))
echo -e "test\n<END>" | ./output/analyzer 10 invalid_plugin >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    print_success "Unknown Plugin Detection"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Unknown Plugin: wanted exit code 1, received $EXIT_CODE"
fi

# Missing plugin specification
TESTS_TOTAL=$((TESTS_TOTAL + 1))
./output/analyzer 10 >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 1 ]; then
    print_success "Missing Plugin Specification Detection"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Missing Plugin Specification: wanted exit code 1, received $EXIT_CODE"
fi

display_test_category "Basic Plugin Functionality"

# Test graceful shutdown
EXPECTED="Pipeline shutdown complete"
ACTUAL=$(echo -e "<END>" | ./output/analyzer 10 logger 2>&1 | grep "Pipeline shutdown complete")
check_test_result "System Graceful Shutdown" "$EXPECTED" "$ACTUAL"

# Basic uppercaser + logger
EXPECTED="[logger] GILAD"
ACTUAL=$(echo -e "gilad\n<END>" | timeout 10s ./output/analyzer 10 uppercaser logger 2>&1 | grep -E "\[logger\]")
check_test_result "uppercaser + logger" "$EXPECTED" "$ACTUAL"

# flipper + logger
EXPECTED="[logger] dalig"
ACTUAL=$(echo -e "gilad\n<END>" | ./output/analyzer 10 flipper logger 2>&1 | grep -E "\[logger\]")
check_test_result "flipper + logger" "$EXPECTED" "$ACTUAL"

# rotator + logger
EXPECTED="[logger] ohell"
ACTUAL=$(echo -e "hello\n<END>" | timeout 20s ./output/analyzer 10 rotator logger 2>&1 | grep -E "\[logger\]")
check_test_result "rotator + logger" "$EXPECTED" "$ACTUAL"

# expander + logger
EXPECTED="[logger] h i"
ACTUAL=$(echo -e "hi\n<END>" | timeout 20s ./output/analyzer 10 expander logger 2>&1 | grep -E "\[logger\]")
check_test_result "Cexpander + logger" "$EXPECTED" "$ACTUAL"

# expander + logger (whitespace)
EXPECTED="[logger]  "
ACTUAL=$(echo -e " \n<END>" | timeout 20s ./output/analyzer 10 expander logger 2>&1 | grep -E "\[logger\]")
check_test_result "expander + logger (whitespace)" "$EXPECTED" "$ACTUAL"

# Typewriter simulation
EXPECTED="[typewriter] hi"
ACTUAL=$(echo -e "hi\n<END>" | timeout 15s ./output/analyzer 10 typewriter 2>&1 | grep -E "\[typewriter\]")
check_test_result "Typewriter simulation" "$EXPECTED" "$ACTUAL"

display_test_category "Multi-Stage Pipeline Processing"

# Two-stage transformation
EXPECTED="[logger] OHELL"
ACTUAL=$(echo -e "hello\n<END>" | timeout 20s ./output/analyzer 10 uppercaser rotator logger 2>&1 | grep -E "\[logger\]")
check_test_result "Dual Transformation Pipeline" "$EXPECTED" "$ACTUAL"

# Three-stage transformation
EXPECTED="[logger] LLEHO"
ACTUAL=$(echo -e "hello\n<END>" | timeout 25s ./output/analyzer 10 uppercaser rotator flipper logger 2>&1 | grep -E "\[logger\]")
check_test_result "Triple Transformation Chain" "$EXPECTED" "$ACTUAL"

# Complex transformation with mixed characters
EXPECTED="[logger] 1A % ! 6 5 3 T 3 2 "
ACTUAL=$(echo -e "123t356!%a\n<END>" | timeout 25s ./output/analyzer 15 flipper expander rotator uppercaser logger 2>&1 | grep -E "\[logger\]")
check_test_result "Mixed Character Processing" "$EXPECTED" "$ACTUAL"

# Extended transformation chain
EXPECTED="[logger] G"
ACTUAL=$(echo -e "g\n<END>" | timeout 25s ./output/analyzer 12 uppercaser rotator flipper logger 2>&1 | grep -E "\[logger\]")
check_test_result "Extended Processing Chain" "$EXPECTED" "$ACTUAL"

# Complex memory-intensive chain
EXPECTED="[logger] S E T T"
ACTUAL=$(echo -e "test\n<END>" | timeout 25s ./output/analyzer 8 uppercaser rotator flipper expander logger 2>&1 | grep -E "\[logger\]")
check_test_result "Memory Intensive Transformation" "$EXPECTED" "$ACTUAL"

display_test_category "Edge Cases and Special Inputs"

# Empty string processing
EXPECTED="[logger] "
ACTUAL=$(echo -e "\n<END>" | timeout 20s ./output/analyzer 10 uppercaser logger 2>&1 | grep -E "\[logger\]")
check_test_result "Empty Input Processing" "$EXPECTED" "$ACTUAL"

# Single character handling
EXPECTED="[logger] x"
ACTUAL=$(echo -e "x\n<END>" | timeout 15s ./output/analyzer 10 expander logger 2>&1 | grep -E "\[logger\]")
check_test_result "Single Character Handling" "$EXPECTED" "$ACTUAL"

# Whitespace character processing
EXPECTED="[logger]    "
ACTUAL=$(echo -e "  \n<END>" | timeout 15s ./output/analyzer 10 expander logger 2>&1 | grep -E "\[logger\]")
check_test_result "Whitespace Processing" "$EXPECTED" "$ACTUAL"

# Only termination signal
EXPECTED=""
ACTUAL=$(echo -e "<END>" | timeout 10s ./output/analyzer 5 logger 2>&1 | grep -E "\[logger\]")
check_test_result "Termination Signal Only" "$EXPECTED" "$ACTUAL"

display_test_category "Multiple Input Processing"

# uppercaser + logger
EXPECTED="[logger] HELLO
[logger] WORLD"
ACTUAL=$(echo -e "hello\nworld\n<END>" | timeout 25s ./output/analyzer 10 uppercaser logger 2>&1 | grep -E "\[logger\]")
check_test_result "uppercaser + logger" "$EXPECTED" "$ACTUAL"

# flipper + uppercaser + logger - first input check
EXPECTED="[logger] YEH"
ACTUAL=$(echo -e "hey\nthis\nis\nexample\n<END>" | timeout 25s ./output/analyzer 12 flipper uppercaser logger 2>&1 | grep -E "\[logger\]" | head -n1)
check_test_result "flipper + uppercaser + logger - first input check" "$EXPECTED" "$ACTUAL"

# flipper + uppercaser + logger - final input check
EXPECTED="[logger] ELPMAXE"
ACTUAL=$(echo -e "hey\nthis\nis\nexample\n<END>" | timeout 25s ./output/analyzer 12 flipper uppercaser logger 2>&1 | grep -E "\[logger\]" | tail -n1)
check_test_result "flipper + uppercaser + logger - final input check" "$EXPECTED" "$ACTUAL"

display_test_category "Performance and Scalability"

# Large queue capacity
EXPECTED="[logger] TEST"
ACTUAL=$(echo -e "test\n<END>" | timeout 25s ./output/analyzer 2000 uppercaser logger 2>&1 | grep -E "\[logger\]")
check_test_result "Large Queue Capacity Handling" "$EXPECTED" "$ACTUAL"

# Minimal queue capacity
EXPECTED="[logger] TEST"
ACTUAL=$(echo -e "test\n<END>" | timeout 25s ./output/analyzer 1 uppercaser logger 2>&1 | grep -E "\[logger\]")
check_test_result "Minimal Queue Capacity Handling" "$EXPECTED" "$ACTUAL"

# Testing high-volume input with minimal queue capacity
stress_input_count=200
EXPECTED_STRESS_ITEMS=$stress_input_count
ACTUAL_STRESS_ITEMS=$( (for k in $(seq 1 $stress_input_count); do echo "stress$k"; done; echo "<END>") \
    | timeout 60s ./output/analyzer 1 logger 2>&1 | grep -E '^(\[logger\])' | wc -l )

TESTS_TOTAL=$((TESTS_TOTAL + 1))
if [ "$ACTUAL_STRESS_ITEMS" -eq "$EXPECTED_STRESS_ITEMS" ]; then
    print_success "High-Volume Minimal Queue Stress Test ($stress_input_count items with queue=1)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Minimal Queue Stress Test: wanted $EXPECTED_STRESS_ITEMS items but processed $ACTUAL_STRESS_ITEMS"
fi

# Extended input string
EXPECTED="[logger] ABCDEFGHIJKLMNOPQRSTUVWXYZ"
ACTUAL=$(echo -e "abcdefghijklmnopqrstuvwxyz\n<END>" | timeout 25s ./output/analyzer 15 uppercaser logger 2>&1 | grep -E "\[logger\]")
check_test_result "Extended String Processing" "$EXPECTED" "$ACTUAL"

# Verifying typewriter timing behavior
TESTS_TOTAL=$((TESTS_TOTAL + 1))
start_time=$(date +%s)
echo -e "hi\n<END>" | timeout 8s ./output/analyzer 5 typewriter >/dev/null 2>&1
end_time=$(date +%s)
timing_duration=$((end_time - start_time))

if [ $timing_duration -ge 1 ] && [ $timing_duration -le 3 ]; then
    print_success "Typewriter Timing Behavior (took ${timing_duration} seconds - within expected range)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Typewriter Timing Behavior: took ${timing_duration} seconds (expected 1-3 seconds for 'hi')"
fi

display_test_category "Volume Processing Tests"

# Medium-volume processing
item_count=75
EXPECTED_ITEMS=$item_count
ACTUAL_ITEMS=$( (for i in $(seq 1 $item_count); do echo "data$i"; done; echo "<END>") \
    | timeout 35s ./output/analyzer 25 logger 2>&1 | grep -E "\[logger\]" | wc -l )

TESTS_TOTAL=$((TESTS_TOTAL + 1))
if [ "$ACTUAL_ITEMS" -eq "$EXPECTED_ITEMS" ]; then
    print_success "Medium-Volume Processing ($item_count items)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Medium-Volume Processing: wanted $EXPECTED_ITEMS items but processed $ACTUAL_ITEMS"
fi

# Running progressive scalability tests with exponential growth
batch_size=3
max_iterations=0
while [[ $batch_size -le 60000 ]]; do
    
    RESULT_OUTPUT=$( (for j in $(seq 1 $batch_size); do echo "word$j"; done; echo "<END>") \
        | ./output/analyzer 15 logger 2>&1 | grep -E '^(\[logger\])' )

    EXPECTED_BATCH_COUNT=$batch_size
    ACTUAL_BATCH_COUNT=$(echo "$RESULT_OUTPUT" | wc -l)

    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    if [[ "$ACTUAL_BATCH_COUNT" -eq "$EXPECTED_BATCH_COUNT" ]]; then
        print_success "Scalability test: $batch_size words processed correctly"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        print_error "Scalability test failed for $batch_size entries"
        print_warning "Wanted $EXPECTED_BATCH_COUNT words but processed $ACTUAL_BATCH_COUNT"
        break
    fi

    # Triple the batch size each iteration
    batch_size=$((batch_size * 3))
    max_iterations=$((max_iterations + 1))
done

# Queue capacity boundary test
EXPECTED_ITEMS=30
ACTUAL_ITEMS=$( (for i in $(seq 1 30); do echo "boundary$i"; done; echo "<END>") \
    | timeout 30s ./output/analyzer 30 logger 2>&1 | grep -E "\[logger\]" | wc -l )

TESTS_TOTAL=$((TESTS_TOTAL + 1))
if [ "$ACTUAL_ITEMS" -eq "$EXPECTED_ITEMS" ]; then
    print_success "Queue Capacity Boundary Test (capacity matches input count)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Queue Capacity Boundary: wanted $EXPECTED_ITEMS items but processed $ACTUAL_ITEMS"
fi

# Premature termination behavior
EXPECTED_ITEMS=7
ACTUAL_ITEMS=$( (for i in $(seq 1 7); do echo "process$i"; done; echo "<END>"; for i in $(seq 1 7); do echo "skip$i"; done) \
    | timeout 20s ./output/analyzer 20 logger 2>&1 | grep -E "\[logger\]" | wc -l )

TESTS_TOTAL=$((TESTS_TOTAL + 1))
if [ "$ACTUAL_ITEMS" -eq "$EXPECTED_ITEMS" ]; then
    print_success "Premature Termination (END signal halts processing)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    print_error "Premature Termination: wanted $EXPECTED_ITEMS processed but got $ACTUAL_ITEMS"
fi

# No <END> signal
if [ -n "$TIMEOUT_UTIL" ]; then
    print_info "No <END> signal (will timeout)..."
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo "hello" | "$TIMEOUT_UTIL" 5 ./output/analyzer 5 logger >/dev/null 2>&1
    if [ $? -eq 124 ]; then
        print_success "No <END> signal (correctly waits without END signal)"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        print_error "No <END> signal: application should wait without END signal"
    fi
else
    print_warning "Skipping indefinite wait test: timeout utility not available"
fi

display_test_category "Test Results Summary"

print_status "Test suite execution completed!"
print_info "Tests completed: $TESTS_PASSED/$TESTS_TOTAL"

if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
    print_success "All tests completed successfully!"
    exit 0
else
    print_error "Some tests failed - review the issues listed above"
    exit 1
fi