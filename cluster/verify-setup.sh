#!/bin/bash
set -e

echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║        pgBalancer Cluster - Configuration Verification                   ║"
echo "╚══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Test PostgreSQL nodes only (quick test)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1️⃣  Starting PostgreSQL Cluster (3 nodes)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

docker compose up -d postgres-primary postgres-standby1 postgres-standby2

echo ""
echo "Waiting for PostgreSQL nodes to be healthy..."
for i in {1..30}; do
    if docker compose ps | grep -q "healthy"; then
        echo "✅ PostgreSQL nodes are starting..."
        break
    fi
    echo -n "."
    sleep 2
done

echo ""
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2️⃣  PostgreSQL Cluster Status"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

docker compose ps postgres-primary postgres-standby1 postgres-standby2

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3️⃣  Testing PostgreSQL Connections"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo ""
echo "Testing Primary (port 5432)..."
if docker compose exec -T postgres-primary pg_isready -U postgres > /dev/null 2>&1; then
    echo "✅ Primary node is ready"
    docker compose exec -T postgres-primary psql -U postgres -d testdb -c "SELECT 'Primary node: ' || version();" 2>/dev/null | head -3
else
    echo "⚠️  Primary node not ready yet"
fi

echo ""
echo "Testing Standby 1 (port 5433)..."
if docker compose exec -T postgres-standby1 pg_isready -U postgres > /dev/null 2>&1; then
    echo "✅ Standby 1 is ready"
else
    echo "⚠️  Standby 1 not ready yet"
fi

echo ""
echo "Testing Standby 2 (port 5434)..."
if docker compose exec -T postgres-standby2 pg_isready -U postgres > /dev/null 2>&1; then
    echo "✅ Standby 2 is ready"
else
    echo "⚠️  Standby 2 not ready yet"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4️⃣  Sample Data Verification"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo ""
echo "Checking sample tables..."
docker compose exec -T postgres-primary psql -U postgres -d testdb << 'SQL' 2>/dev/null || echo "⚠️  Sample data not loaded yet"
\dt
SELECT 'Users:', COUNT(*) FROM users;
SELECT 'Orders:', COUNT(*) FROM orders;
SQL

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5️⃣  bctl Command Reference"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cat << 'BCTL'

Once pgBalancer is running, bctl commands would be:

Status Commands:
  bctl status                    - Show pgBalancer status
  bctl nodes list                - List all backend nodes
  bctl nodes info 0              - Get primary node details
  bctl nodes info 1              - Get standby1 details
  bctl nodes info 2              - Get standby2 details

Management Commands:
  bctl nodes detach 1            - Detach node 1
  bctl nodes attach 1            - Attach node 1
  bctl nodes promote 2           - Promote node 2 to primary
  bctl reload                    - Reload configuration
  bctl cache invalidate          - Clear query cache

Example REST API calls (port 8080):
  curl http://localhost:8080/api/v1/status
  curl http://localhost:8080/api/v1/nodes
  curl http://localhost:8080/api/v1/nodes/0

BCTL

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "6️⃣  Next Steps"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cat << 'NEXT'

To fully test pgBalancer with bctl:

1. Build pgBalancer image (takes ~10 minutes):
   make build

2. Start complete cluster:
   make up

3. Wait for all services to be healthy:
   watch make status

4. Run bctl management shell:
   make bctl
   
   Inside shell:
   $ bctl status
   $ bctl nodes list
   $ bctl nodes info 0

5. Connect to database via pgBalancer:
   make psql
   
   Or:
   psql -h localhost -p 9999 -U postgres -d testdb

6. Test REST API:
   curl http://localhost:8080/api/v1/status | jq
   curl http://localhost:8080/api/v1/nodes | jq

NEXT

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "7️⃣  Cleanup"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo ""
read -p "Stop PostgreSQL test nodes? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    docker compose down
    echo "✅ Test nodes stopped"
else
    echo "ℹ️  Nodes still running. Use 'make down' to stop them."
fi

echo ""
echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║               Configuration Verification Complete                        ║"
echo "╚══════════════════════════════════════════════════════════════════════════╝"

