#!/bin/bash
# dev-watch.sh - Watch fierymud binary and restart on rebuild
# Usage: ./scripts/dev-watch.sh

BINARY="./build/fierymud"
PID=""

cleanup() {
    echo -e "\n[watch] Shutting down..."
    if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        kill "$PID" 2>/dev/null
        wait "$PID" 2>/dev/null
    fi
    exit 0
}

trap cleanup SIGINT SIGTERM

start_server() {
    echo "[watch] Starting fierymud..."
    $BINARY &
    PID=$!
    echo "[watch] Server started (PID: $PID)"
}

stop_server() {
    if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        echo "[watch] Stopping server (PID: $PID)..."
        kill "$PID" 2>/dev/null
        wait "$PID" 2>/dev/null
        PID=""
    fi
}

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo "[watch] Error: $BINARY not found. Build first with: cmake --build build"
    exit 1
fi

# Check for inotifywait
if ! command -v inotifywait &> /dev/null; then
    echo "[watch] Error: inotifywait not found. Install with: sudo apt install inotify-tools"
    exit 1
fi

echo "[watch] Watching $BINARY for changes..."
echo "[watch] Press Ctrl+C to stop"
echo ""

# Initial start
start_server

# Track binary modification time
LAST_MTIME=$(stat -c %Y "$BINARY" 2>/dev/null || echo "0")

# Watch for changes
while true; do
    # Wait for any change in the build directory
    inotifywait -q -q -e close_write,moved_to,create,modify "$(dirname $BINARY)" 2>/dev/null

    # Brief pause to let the build fully complete
    sleep 0.2

    # Check if the binary's modification time actually changed
    CURRENT_MTIME=$(stat -c %Y "$BINARY" 2>/dev/null || echo "0")

    if [ "$CURRENT_MTIME" != "$LAST_MTIME" ] && [ -f "$BINARY" ]; then
        echo ""
        echo "[watch] Binary changed, restarting..."
        stop_server
        sleep 0.3  # Brief pause to ensure file is fully written
        start_server
        LAST_MTIME="$CURRENT_MTIME"
    fi
done
