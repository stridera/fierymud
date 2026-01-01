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
    "stable")
        echo -e "${YELLOW}Running stable integration tests...${NC}"
        ctest --test-dir build -L stable
        ;;
    "integration")
        echo -e "${YELLOW}Running legacy integration tests (may segfault)...${NC}"
        ctest --test-dir build -L integration
        ;;
    "session")
        echo -e "${YELLOW}Running session tests...${NC}"
        ctest --test-dir build -L session
        ;;
    "recommended"|"")
        echo -e "${YELLOW}Running recommended tests (unit + stable)...${NC}"
        ctest --test-dir build -L unit
        ctest --test-dir build -L stable
        ;;
    "all")
        echo -e "${YELLOW}Running all tests (including unstable legacy)...${NC}"
        ctest --test-dir build
        ;;
    "verbose")
        echo -e "${YELLOW}Running recommended tests with verbose output...${NC}"
        ctest --test-dir build -L unit --verbose
        ctest --test-dir build -L stable --verbose
        ;;
    *)
        echo -e "${RED}Usage: $0 [unit|stable|integration|session|recommended|all|verbose]${NC}"
        echo -e "${YELLOW}Recommended: $0 recommended (default)${NC}"
        exit 1
        ;;
esac

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
else
    echo -e "${RED}❌ Some tests failed!${NC}"
    exit 1
fi