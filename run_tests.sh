#!/bin/bash
# FieryMUD Testing Script
# Provides convenient test execution for development and CI

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}FieryMUD Test Suite${NC}"
echo "================="

# Build tests
echo -e "${YELLOW}Building tests...${NC}"
cmake --build build --target tests

# Run tests based on argument
case "${1:-all}" in
    "unit")
        echo -e "${YELLOW}Running unit tests...${NC}"
        ctest --test-dir build -L unit
        ;;
    "integration")
        echo -e "${YELLOW}Running integration tests...${NC}"
        ctest --test-dir build -L integration
        ;;
    "session")
        echo -e "${YELLOW}Running session tests...${NC}"
        ctest --test-dir build -L session
        ;;
    "all"|"")
        echo -e "${YELLOW}Running all tests...${NC}"
        ctest --test-dir build
        ;;
    "verbose")
        echo -e "${YELLOW}Running all tests with verbose output...${NC}"
        ctest --test-dir build --verbose
        ;;
    *)
        echo -e "${RED}Usage: $0 [unit|integration|session|all|verbose]${NC}"
        exit 1
        ;;
esac

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
else
    echo -e "${RED}❌ Some tests failed!${NC}"
    exit 1
fi