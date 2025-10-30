#!/bin/bash
# bctl Interactive Wrapper for Docker

cat << 'EOF'
╔══════════════════════════════════════════════════════════════════════════╗
║                      pgBalancer Management Shell                         ║
║                          Powered by bctl                                 ║
╚══════════════════════════════════════════════════════════════════════════╝

Welcome to the pgBalancer management shell!

Quick Commands:
  bctl status              - Show pgBalancer status
  bctl nodes list          - List all backend nodes
  bctl nodes info <id>     - Get node information
  bctl nodes attach <id>   - Attach a node
  bctl nodes detach <id>   - Detach a node
  bctl reload              - Reload configuration
  bctl cache invalidate    - Invalidate query cache
  bctl --help              - Show all commands

Environment:
  PGBALANCER_HOST: ${PGBALANCER_HOST:-localhost}
  PGBALANCER_PORT: ${PGBALANCER_PORT:-8080}
  REST API: ${PGBALANCER_REST_API:-http://pgbalancer:8080}

Examples are available in /examples directory.

Type 'exit' to quit.

EOF

# Create alias for easy bctl usage
alias bctl='bctl --host ${PGBALANCER_HOST:-pgbalancer} --port ${PGBALANCER_PORT:-8080}'

# Show initial status
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Current Status:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
bctl --host ${PGBALANCER_HOST:-pgbalancer} --port ${PGBALANCER_PORT:-8080} status 2>/dev/null || echo "⚠️  Unable to connect to pgBalancer. Is it running?"
echo ""

# Start interactive shell
exec /bin/bash

