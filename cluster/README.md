# pgBalancer Docker Cluster

Complete Docker-based testing environment for pgBalancer with 3-node PostgreSQL cluster.

## Quick Start

```bash
# 1. Initialize configuration
make init

# 2. Build Docker images  
make build

# 3. Start cluster
make up

# 4. Check status
make status

# 5. Run bctl management shell
make bctl
```

## Architecture

```
pgBalancer (port 9999)
  ├── PostgreSQL Primary (port 5432)
  ├── PostgreSQL Standby 1 (port 5433)
  └── PostgreSQL Standby 2 (port 5434)

REST API (port 8080)
bctl Management CLI
```

## Services

- **postgres-primary** - Primary PostgreSQL node
- **postgres-standby1** - Standby PostgreSQL node 1
- **postgres-standby2** - Standby PostgreSQL node 2
- **pgbalancer** - pgBalancer connection pooler
- **bctl** - Management CLI tool

## Testing

Run comprehensive cluster tests:

```bash
./test-cluster.sh
```

Verify setup:

```bash
./verify-setup.sh
```

## Management Commands

```bash
# Service management
make up              # Start cluster
make down            # Stop cluster
make restart         # Restart cluster
make status          # Show status
make logs            # View logs

# Operations
make bctl            # bctl shell
make psql            # Connect to DB
make test            # Run tests

# Cleanup
make clean           # Remove all containers/volumes
```

## Configuration

Edit `.env` file (created from `env.example`):

```env
POSTGRES_PASSWORD=postgres
PGBALANCER_PORT=9999
REST_API_PORT=8080
```

## Ports

- `9999` - pgBalancer connection pooler
- `8080` - REST API
- `5432` - PostgreSQL Primary
- `5433` - PostgreSQL Standby 1
- `5434` - PostgreSQL Standby 2

## Features Tested

- Connection pooling
- Load balancing
- Health monitoring
- REST API
- bctl CLI
- PostgreSQL replication
- Automatic failover

## Requirements

- Docker
- Docker Compose
- 4GB RAM minimum
- 10GB disk space

## License

PostgreSQL License - Copyright (c) 2024-2025, pgElephant, Inc.

