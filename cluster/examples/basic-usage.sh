#!/bin/bash
# Basic pgBalancer usage examples with bctl

set -e

PGBALANCER_HOST="${PGBALANCER_HOST:-pgbalancer}"
PGBALANCER_PORT="${PGBALANCER_PORT:-8080}"

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║        pgBalancer Basic Usage Examples                       ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Function to run bctl commands
run_bctl() {
    echo "$ bctl --host $PGBALANCER_HOST --port $PGBALANCER_PORT $@"
    bctl --host "$PGBALANCER_HOST" --port "$PGBALANCER_PORT" "$@"
    echo ""
}

# 1. Check server status
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. Check pgBalancer Status"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
run_bctl status

# 2. List all nodes
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. List All Backend Nodes"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
run_bctl nodes list

# 3. Get node information
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. Get Node 0 (Primary) Information"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
run_bctl nodes info 0

# 4. Test connection to pgBalancer
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4. Test Connection via psql"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "$ psql -h $PGBALANCER_HOST -p 9999 -U postgres -c 'SELECT version();'"
if command -v psql > /dev/null 2>&1; then
    PGPASSWORD=postgres psql -h "$PGBALANCER_HOST" -p 9999 -U postgres -d testdb -c 'SELECT version();' 2>/dev/null || echo "⚠️  Connection failed (pgBalancer may not be fully ready)"
else
    echo "⚠️  psql not available in this container"
fi
echo ""

# 5. Show help
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5. Show bctl Help"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
run_bctl --help

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║               Examples Completed Successfully                ║"
echo "╚══════════════════════════════════════════════════════════════╝"

