#!/bin/bash
# Start Coral Web Dashboard

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Activate virtual environment
source venv/bin/activate

# Check if corald is running
if ! pgrep -x "corald" > /dev/null; then
    echo "Warning: corald is not running!"
    echo "Start it with: src/corald -daemon -datadir=/tmp/coral-test"
    echo ""
fi

echo "Starting Coral Web Dashboard..."
echo "Open http://localhost:5999 in your browser"
echo ""

python app.py
