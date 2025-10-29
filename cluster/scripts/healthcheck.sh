#!/bin/bash
# pgBalancer Health Check Script

set -e

# Check if pgbalancer process is running
if ! pgrep -x pgbalancer > /dev/null; then
    echo "ERROR: pgbalancer process not running"
    exit 1
fi

# Check REST API if enabled
if command -v bctl > /dev/null 2>&1; then
    if bctl --host localhost --port 8080 status > /dev/null 2>&1; then
        exit 0
    else
        echo "WARNING: bctl status check failed, but process is running"
        # Still return success if process is running
        exit 0
    fi
fi

# Process is running
exit 0

