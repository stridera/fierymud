#!/bin/bash

#############################################################################
# FieryMUD Test Validation System
# Comprehensive test runner with stability analysis and performance tracking
#############################################################################

set -e

# Configuration
BUILD_DIR="build"
RESULTS_DIR="test_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TIMEOUT_DURATION=60
PERFORMANCE_LOG="$RESULTS_DIR/performance_$TIMESTAMP.json"
STABILITY_LOG="$RESULTS_DIR/stability_$TIMESTAMP.json"

# Create results directory
mkdir -p "$RESULTS_DIR"

# Color output functions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Build tests if needed
build_tests() {
    log_info "Building test executable..."
    if cmake --build "$BUILD_DIR" --target tests; then
        log_success "Tests built successfully"
    else
        log_error "Failed to build tests"
        exit 1
    fi
}

# Test category definitions
declare -A TEST_CATEGORIES=(
    ["stable"]="[stable]"
    ["unit"]="[unit]"
    ["integration"]="[integration]"
    ["session"]="[session]"
    ["combat"]="[combat]"
    ["performance"]="[performance]"
)

# Run specific test category
run_test_category() {
    local category="$1"
    local filter="${TEST_CATEGORIES[$category]}"
    local start_time=$(date +%s.%3N)
    local exit_code=0
    
    log_info "Running $category tests with filter: $filter"
    
    # Run tests with timeout
    if timeout "$TIMEOUT_DURATION" "./$BUILD_DIR/tests" "$filter" > "$RESULTS_DIR/${category}_$TIMESTAMP.log" 2>&1; then
        local end_time=$(date +%s.%3N)
        local duration=$(echo "$end_time - $start_time" | bc -l)
        
        # Parse results
        local total_tests=$(grep "test cases:" "$RESULTS_DIR/${category}_$TIMESTAMP.log" | tail -1 | awk '{print $3}')
        local passed_tests=$(grep "test cases:" "$RESULTS_DIR/${category}_$TIMESTAMP.log" | tail -1 | awk '{print $6}')
        local failed_tests=$(grep "test cases:" "$RESULTS_DIR/${category}_$TIMESTAMP.log" | tail -1 | awk '{print $9}')
        
        # Record performance data
        cat >> "$PERFORMANCE_LOG" << EOF
{
  "category": "$category",
  "timestamp": "$TIMESTAMP",
  "duration_seconds": $duration,
  "total_tests": $total_tests,
  "passed_tests": $passed_tests,
  "failed_tests": $failed_tests,
  "exit_code": 0,
  "status": "completed"
}
EOF
        
        log_success "$category tests completed in ${duration}s ($passed_tests/$total_tests passed)"
        return 0
    else
        exit_code=$?
        local end_time=$(date +%s.%3N)
        local duration=$(echo "$end_time - $start_time" | bc -l)
        
        # Determine failure type
        local failure_type="unknown"
        if [ $exit_code -eq 124 ]; then
            failure_type="timeout"
            log_error "$category tests timed out after ${TIMEOUT_DURATION}s"
        elif [ $exit_code -eq 139 ]; then
            failure_type="segfault"
            log_error "$category tests crashed with segfault"
        else
            failure_type="test_failure"
            log_warning "$category tests completed with failures (exit code: $exit_code)"
        fi
        
        # Record failure data
        cat >> "$STABILITY_LOG" << EOF
{
  "category": "$category",
  "timestamp": "$TIMESTAMP",
  "duration_seconds": $duration,
  "exit_code": $exit_code,
  "failure_type": "$failure_type",
  "status": "failed"
}
EOF
        
        return $exit_code
    fi
}

# Stability analysis
analyze_stability() {
    log_info "Analyzing test stability..."
    
    local stable_categories=("stable" "unit")
    local integration_categories=("integration" "combat")
    local experimental_categories=("session" "performance")
    
    local stable_score=0
    local total_stable=0
    
    # Check stable categories
    for category in "${stable_categories[@]}"; do
        if run_test_category "$category"; then
            ((stable_score++))
        fi
        ((total_stable++))
    done
    
    # Stability percentage
    local stability_percent=$(echo "scale=1; $stable_score * 100 / $total_stable" | bc -l)
    
    if (( $(echo "$stability_percent >= 80" | bc -l) )); then
        log_success "High stability: $stability_percent% of core tests passing"
    elif (( $(echo "$stability_percent >= 60" | bc -l) )); then
        log_warning "Moderate stability: $stability_percent% of core tests passing"
    else
        log_error "Low stability: $stability_percent% of core tests passing"
    fi
    
    # Run integration tests with detailed reporting
    log_info "Running integration stability tests..."
    for category in "${integration_categories[@]}"; do
        run_test_category "$category"
    done
}

# Performance regression detection
check_performance_regression() {
    log_info "Checking for performance regressions..."
    
    # Find previous performance logs
    local prev_logs=($(ls -t "$RESULTS_DIR"/performance_*.json 2>/dev/null | tail -n +2))
    
    if [ ${#prev_logs[@]} -eq 0 ]; then
        log_info "No previous performance data found - establishing baseline"
        return 0
    fi
    
    # Simple regression check (compare with most recent previous run)
    local prev_log="${prev_logs[0]}"
    local current_stable_time=$(grep '"category": "stable"' "$PERFORMANCE_LOG" | grep -o '"duration_seconds": [0-9.]*' | grep -o '[0-9.]*')
    local prev_stable_time=$(grep '"category": "stable"' "$prev_log" | grep -o '"duration_seconds": [0-9.]*' | grep -o '[0-9.]*')
    
    if [ -n "$current_stable_time" ] && [ -n "$prev_stable_time" ]; then
        local regression_threshold=1.5  # 50% performance degradation threshold
        local ratio=$(echo "scale=2; $current_stable_time / $prev_stable_time" | bc -l)
        
        if (( $(echo "$ratio > $regression_threshold" | bc -l) )); then
            log_warning "Performance regression detected: stable tests ${ratio}x slower than previous run"
        else
            log_success "No significant performance regression detected"
        fi
    fi
}

# Generate comprehensive report
generate_report() {
    local report_file="$RESULTS_DIR/validation_report_$TIMESTAMP.md"
    
    log_info "Generating comprehensive test validation report..."
    
    cat > "$report_file" << EOF
# FieryMUD Test Validation Report

**Generated:** $(date)  
**Timestamp:** $TIMESTAMP

## Executive Summary

### Test Categories Analyzed
$(for category in "${!TEST_CATEGORIES[@]}"; do echo "- $category: ${TEST_CATEGORIES[$category]}"; done)

### Performance Metrics
\`\`\`json
$(cat "$PERFORMANCE_LOG" 2>/dev/null || echo "No performance data available")
\`\`\`

### Stability Analysis
\`\`\`json
$(cat "$STABILITY_LOG" 2>/dev/null || echo "No stability issues recorded")
\`\`\`

## Detailed Results

EOF

    # Append individual test logs
    for category in "${!TEST_CATEGORIES[@]}"; do
        if [ -f "$RESULTS_DIR/${category}_$TIMESTAMP.log" ]; then
            cat >> "$report_file" << EOF

### $category Tests
\`\`\`
$(tail -20 "$RESULTS_DIR/${category}_$TIMESTAMP.log")
\`\`\`

EOF
        fi
    done
    
    log_success "Report generated: $report_file"
}

# Main execution
main() {
    log_info "Starting FieryMUD Test Validation System"
    log_info "Timestamp: $TIMESTAMP"
    
    # Initialize JSON files
    echo "[]" > "$PERFORMANCE_LOG"
    echo "[]" > "$STABILITY_LOG"
    
    # Build tests
    build_tests
    
    # Run stability analysis
    analyze_stability
    
    # Check for performance regressions
    check_performance_regression
    
    # Generate comprehensive report
    generate_report
    
    log_success "Test validation completed. Check $RESULTS_DIR/ for detailed results."
}

# Execute main function
main "$@"