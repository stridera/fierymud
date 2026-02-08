#!/bin/bash
# Install git hooks for this repository

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GIT_HOOKS="$REPO_ROOT/.git/hooks"

echo "Installing git hooks..."

for hook in "$SCRIPT_DIR"/*; do
    if [[ -f "$hook" && "$(basename "$hook")" != "install.sh" ]]; then
        hookname=$(basename "$hook")
        cp "$hook" "$GIT_HOOKS/$hookname"
        chmod +x "$GIT_HOOKS/$hookname"
        echo "  Installed: $hookname"
    fi
done

echo "Done!"
