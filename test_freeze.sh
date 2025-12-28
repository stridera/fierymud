#!/bin/bash
# Script to test the MUD freeze issue

cd /home/strider/Code/mud/fierymud

# Kill any existing server
pkill -9 fierymud 2>/dev/null

# Start the server in background with GDB, with commands to log backtraces
echo "Starting MUD under GDB..."
gdb -batch -ex "run" -ex "bt" -ex "thread apply all bt" ./build/fierymud 2>&1 &
GDB_PID=$!

sleep 5

# Wait for server to start
echo "Waiting for server to start..."
sleep 3

echo "Server should be running. Connect to localhost:4000 and test with:"
echo "  login -> load obj 12:40 -> wie sword -> s -> s -> kill guard"
echo ""
echo "If it freezes, press Ctrl-C in the GDB terminal or kill $GDB_PID"
echo "Then check the backtrace output"

wait $GDB_PID
