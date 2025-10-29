#!/bin/bash
# Initialize PostgreSQL Primary Node for pgBalancer

set -e

echo "Initializing PostgreSQL Primary Node..."

# Configure PostgreSQL for replication
cat >> "$PGDATA/postgresql.conf" <<EOF

# Replication Settings
wal_level = replica
max_wal_senders = 10
max_replication_slots = 10
hot_standby = on
hot_standby_feedback = on
wal_log_hints = on

# Performance
shared_buffers = 256MB
effective_cache_size = 1GB
maintenance_work_mem = 64MB
checkpoint_completion_target = 0.9
wal_buffers = 16MB
default_statistics_target = 100
random_page_cost = 1.1
effective_io_concurrency = 200
work_mem = 4MB
min_wal_size = 1GB
max_wal_size = 4GB
max_worker_processes = 4
max_parallel_workers_per_gather = 2
max_parallel_workers = 4
max_parallel_maintenance_workers = 2
EOF

# Configure pg_hba.conf for replication
cat >> "$PGDATA/pg_hba.conf" <<EOF

# Replication connections
host    replication     all             0.0.0.0/0               trust
host    replication     all             ::/0                    trust
EOF

echo "✅ Primary node configured"

