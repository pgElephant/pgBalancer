#!/bin/bash
# Initialize PostgreSQL Standby Node for pgBalancer

set -e

echo "Initializing PostgreSQL Standby Node..."

# Configure PostgreSQL for hot standby
cat >> "$PGDATA/postgresql.conf" <<EOF

# Hot Standby Settings
hot_standby = on
hot_standby_feedback = on
wal_receiver_timeout = 60s
max_standby_streaming_delay = 30s
max_standby_archive_delay = 30s

# Replication
wal_level = replica
max_wal_senders = 10
max_replication_slots = 10
wal_log_hints = on

# Performance
shared_buffers = 256MB
effective_cache_size = 1GB
EOF

# Note: Actual standby setup would require pg_basebackup or streaming replication setup
# For demo purposes, this just configures the standby-ready settings

echo "✅ Standby node configured (ready for replication setup)"

