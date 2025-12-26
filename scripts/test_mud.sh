#!/bin/bash
# Test script for MUD server
# Usage: ./test_mud.sh [timeout] [login] [commands...]
# Example: ./test_mud.sh 15 "admin@muditor.dev:admin123:AdminChar" "look" "inventory" "quit"

TIMEOUT=${1:-15}
shift

# Create a temp file for the command sequence
CMDFILE=$(mktemp)

# First command is login, needs longer delay
echo "$1" > "$CMDFILE"
shift

# Add remaining commands with sleep delays
for arg in "$@"; do
    echo "sleep 0.5" >> "$CMDFILE"
    echo "$arg" >> "$CMDFILE"
done

# Generate the command stream with delays
{
    while IFS= read -r line; do
        if [[ "$line" == sleep* ]]; then
            eval "$line"
        else
            echo "$line"
        fi
    done < "$CMDFILE"
    sleep 1  # Final delay before disconnect
} | timeout $TIMEOUT nc localhost 4003 2>&1

rm -f "$CMDFILE"
