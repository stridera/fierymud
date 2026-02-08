#!/bin/bash
# Format C++ source files with clang-format

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Find clang-format (prefer newer versions for C++23 support)
CLANG_FORMAT=""
for version in 19 18 17 ""; do
    candidate="clang-format${version:+-$version}"
    if command -v "$candidate" &>/dev/null; then
        CLANG_FORMAT="$candidate"
        break
    fi
done

if [[ -z "$CLANG_FORMAT" ]]; then
    echo -e "${RED}Error: clang-format not found. Install with: sudo apt install clang-format-19${NC}"
    exit 1
fi

echo -e "${GREEN}Using: $($CLANG_FORMAT --version)${NC}"

# Default: format in-place
CHECK_ONLY=false
FIX_WHITESPACE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --check)
            CHECK_ONLY=true
            shift
            ;;
        --fix-whitespace)
            FIX_WHITESPACE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --check           Check formatting without modifying files"
            echo "  --fix-whitespace  Also fix trailing whitespace and missing final newlines"
            echo "  -h, --help        Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Find all C++ source files (excluding build directory and third-party)
FILES=$(find src include -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) 2>/dev/null || true)

if [[ -z "$FILES" ]]; then
    echo -e "${YELLOW}No C++ files found in src/ or include/${NC}"
    exit 0
fi

FILE_COUNT=$(echo "$FILES" | wc -l)
echo -e "${GREEN}Found $FILE_COUNT C++ files${NC}"

# Fix trailing whitespace and final newlines
if [[ "$FIX_WHITESPACE" == true ]]; then
    echo -e "${YELLOW}Fixing trailing whitespace and final newlines...${NC}"
    for file in $FILES; do
        # Remove trailing whitespace
        sed -i 's/[[:space:]]*$//' "$file"
        # Ensure file ends with exactly one newline
        sed -i -e '$a\' "$file"
        # Remove multiple trailing newlines, keep just one
        sed -i -e :a -e '/^\n*$/{$d;N;ba' -e '}' "$file"
    done
    echo -e "${GREEN}Whitespace fixes applied${NC}"
fi

if [[ "$CHECK_ONLY" == true ]]; then
    echo -e "${YELLOW}Checking formatting...${NC}"
    FAILED=0
    for file in $FILES; do
        if ! $CLANG_FORMAT --dry-run --Werror "$file" 2>/dev/null; then
            echo -e "${RED}  $file${NC}"
            FAILED=$((FAILED + 1))
        fi
    done
    if [[ $FAILED -gt 0 ]]; then
        echo -e "${RED}$FAILED files need formatting. Run: ./format.sh${NC}"
        exit 1
    fi
    echo -e "${GREEN}All files are properly formatted${NC}"
else
    echo -e "${YELLOW}Formatting files...${NC}"
    echo "$FILES" | xargs $CLANG_FORMAT -i
    echo -e "${GREEN}Done! Formatted $FILE_COUNT files${NC}"
fi
