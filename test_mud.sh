#!/bin/bash
# Test script for MUD server
# Usage: ./test_mud.sh [timeout] [commands...]
# Example: ./test_mud.sh 10 "samui:matthias" "abilities" "quit"

TIMEOUT=${1:-10}
shift

# Build command string with newlines between each command
CMD=""
for arg in "$@"; do
    CMD="${CMD}${arg}\n"
done

# Send commands to MUD
printf "$CMD" | timeout $TIMEOUT nc localhost 4444 2>&1
