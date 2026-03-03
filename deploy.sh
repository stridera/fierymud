#!/bin/bash
# Rebuild and restart FieryMUD
set -e

cd /home/strider/Code/mud/fierymud

echo "Building FieryMUD..."
./build.sh

echo "Restarting FieryMUD service..."
sudo systemctl restart fierymud

echo "Done. Status:"
systemctl status fierymud --no-pager | head -10
