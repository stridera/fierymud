#!/bin/bash
# FieryMUD Testing Script
# Provides convenient test execution for development and CI

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Limit parallel jobs to be nice to WSL/system (default: half of available cores, minimum 2)
TOTAL_CORES=$(nproc 2>/dev/null || echo 4)
MAX_JOBS=$(( (TOTAL_CORES + 1) / 2 ))
MAX_JOBS=$(( MAX_JOBS < 2 ? 2 : MAX_JOBS ))

echo -e "${YELLOW}FieryMUD Test Suite${NC}"
echo "================="

# Build tests
echo -e "${YELLOW}Building tests (using $MAX_JOBS parallel jobs)...${NC}"
cmake --build build --target stable_tests unit_tests -j "$MAX_JOBS"

# Run tests based on argument
case "${1:-recommended}" in
    "unit")
        echo -e "${YELLOW}Running unit tests...${NC}"
        ctest --test-dir build -L unit
        ;;
    "stable"|"integration")
        echo -e "${YELLOW}Running stable integration tests...${NC}"
        ctest --test-dir build -L stable
        ;;
    "session")
        echo -e "${YELLOW}Running session tests...${NC}"
        ctest --test-dir build -L session
        ;;
    "recommended"|""|"all")
        echo -e "${YELLOW}Running all tests (unit + stable)...${NC}"
        ctest --test-dir build -L unit
        ctest --test-dir build -L stable
        ;;
    "verbose")
        echo -e "${YELLOW}Running all tests with verbose output...${NC}"
        ctest --test-dir build -L unit --verbose
        ctest --test-dir build -L stable --verbose
        ;;
    *)
        echo -e "${RED}Usage: $0 [unit|stable|session|all|verbose]${NC}"
        echo -e "${YELLOW}Default: runs all tests (unit + stable)${NC}"
        exit 1
        ;;
esac

if [ $? -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
