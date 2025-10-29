#!/bin/bash
set -e

echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║          pgBalancer Docker Cluster - Validation Test                     ║"
echo "╚══════════════════════════════════════════════════════════════════════════╝"
echo ""

PASSED=0
FAILED=0

# Test function
test_check() {
    local name="$1"
    local command="$2"
    
    echo -n "Testing: $name... "
    if eval "$command" > /dev/null 2>&1; then
        echo "✅ PASSED"
        PASSED=$((PASSED + 1))
    else
        echo "❌ FAILED"
        FAILED=$((FAILED + 1))
    fi
}

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1️⃣  File Structure Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Dockerfile exists" "[ -f Dockerfile ]"
test_check "docker-compose.yml exists" "[ -f docker-compose.yml ]"
test_check "Makefile exists" "[ -f Makefile ]"
test_check "README.md exists" "[ -f README.md ]"
test_check "env.example exists" "[ -f env.example ]"
test_check ".dockerignore exists" "[ -f .dockerignore ]"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2️⃣  Scripts Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Entrypoint script exists" "[ -f scripts/docker-entrypoint.sh ]"
test_check "Healthcheck script exists" "[ -f scripts/healthcheck.sh ]"
test_check "bctl wrapper exists" "[ -f scripts/bctl-wrapper.sh ]"
test_check "Init primary script exists" "[ -f scripts/init-primary.sh ]"
test_check "Init standby script exists" "[ -f scripts/init-standby.sh ]"

test_check "Entrypoint is executable" "[ -x scripts/docker-entrypoint.sh ]"
test_check "Healthcheck is executable" "[ -x scripts/healthcheck.sh ]"
test_check "bctl wrapper is executable" "[ -x scripts/bctl-wrapper.sh ]"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3️⃣  PostgreSQL Configuration Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Primary init.sql exists" "[ -f postgres/primary/init.sql ]"
test_check "Primary postgresql.conf exists" "[ -f postgres/primary/postgresql.conf ]"
test_check "Standby1 init.sql exists" "[ -f postgres/standby1/init.sql ]"
test_check "Standby2 init.sql exists" "[ -f postgres/standby2/init.sql ]"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4️⃣  Examples Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Basic usage example exists" "[ -f examples/basic-usage.sh ]"
test_check "Basic usage is executable" "[ -x examples/basic-usage.sh ]"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5️⃣  Docker Configuration Validation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Dockerfile is valid" "grep -q 'FROM ubuntu:22.04' Dockerfile"
test_check "Multi-stage build defined" "grep -q 'AS builder' Dockerfile"
test_check "Entrypoint defined" "grep -q 'ENTRYPOINT' Dockerfile"
test_check "Health check defined" "grep -q 'HEALTHCHECK' Dockerfile"

test_check "docker-compose version 3.8" "grep -q 'version.*3.8' docker-compose.yml"
test_check "pgbalancer service defined" "grep -q 'pgbalancer:' docker-compose.yml"
test_check "postgres-primary defined" "grep -q 'postgres-primary:' docker-compose.yml"
test_check "postgres-standby1 defined" "grep -q 'postgres-standby1:' docker-compose.yml"
test_check "postgres-standby2 defined" "grep -q 'postgres-standby2:' docker-compose.yml"
test_check "bctl service defined" "grep -q 'bctl:' docker-compose.yml"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "6️⃣  Makefile Validation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Makefile help target" "grep -q '^help:' Makefile"
test_check "Makefile build target" "grep -q '^build:' Makefile"
test_check "Makefile up target" "grep -q '^up:' Makefile"
test_check "Makefile down target" "grep -q '^down:' Makefile"
test_check "Makefile bctl target" "grep -q '^bctl:' Makefile"
test_check "Makefile psql target" "grep -q '^psql:' Makefile"
test_check "Makefile test target" "grep -q '^test:' Makefile"
test_check "Makefile clean target" "grep -q '^clean:' Makefile"
test_check "Makefile init target" "grep -q '^init:' Makefile"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "7️⃣  Content Validation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Entrypoint has shebang" "head -1 scripts/docker-entrypoint.sh | grep -q '^#!/bin/bash'"
test_check "Healthcheck has shebang" "head -1 scripts/healthcheck.sh | grep -q '^#!/bin/bash'"
test_check "Primary init has SQL" "grep -q 'CREATE' postgres/primary/init.sql"
test_check "Primary conf has replication" "grep -q 'wal_level' postgres/primary/postgresql.conf"
test_check "README has Quick Start" "grep -q 'Quick Start' README.md"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "8️⃣  Docker Prerequisites Check"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_check "Docker is installed" "command -v docker"
test_check "Docker Compose is installed" "command -v docker-compose || docker compose version"
test_check "Docker daemon is running" "docker ps"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 TEST SUMMARY"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Total Tests: $((PASSED + FAILED))"
echo "Passed: $PASSED ✅"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "╔══════════════════════════════════════════════════════════════════════════╗"
    echo "║                   ✅ ALL TESTS PASSED! ✅                                ║"
    echo "║                                                                          ║"
    echo "║  The pgBalancer Docker cluster is properly configured and ready to use!  ║"
    echo "║                                                                          ║"
    echo "║  Next steps:                                                             ║"
    echo "║    1. make init      - Initialize the cluster                           ║"
    echo "║    2. make up        - Start all services                               ║"
    echo "║    3. make bctl      - Start management shell                           ║"
    echo "║                                                                          ║"
    echo "╚══════════════════════════════════════════════════════════════════════════╝"
    exit 0
else
    echo "╔══════════════════════════════════════════════════════════════════════════╗"
    echo "║                    ⚠️  SOME TESTS FAILED ⚠️                              ║"
    echo "║                                                                          ║"
    echo "║  Please review the failed tests above and fix any issues.               ║"
    echo "║                                                                          ║"
    echo "╚══════════════════════════════════════════════════════════════════════════╝"
    exit 1
fi

