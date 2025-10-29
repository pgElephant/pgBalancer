# pgBalancer Docker Cluster

Complete Docker-based pgBalancer cluster setup with bctl management.

## 🚀 Features

- **pgBalancer**: PostgreSQL connection pooler and load balancer
- **bctl**: Command-line management tool for pgBalancer
- **3-Node PostgreSQL Cluster**: 1 primary + 2 standby nodes
- **REST API**: Modern HTTP/JSON API for management
- **Health Checks**: Automatic monitoring of all services
- **Production-Ready**: Optimized configuration and resource management

## 📋 Prerequisites

- Docker 20.10+
- Docker Compose 2.0+
- 4GB RAM minimum
- 10GB disk space

## 🎯 Quick Start

### 1. Start the Cluster

```bash
# Build and start all services
docker-compose up -d

# View logs
docker-compose logs -f pgbalancer

# Check status
docker-compose ps
```

### 2. Use bctl to Manage pgBalancer

```bash
# Start interactive management shell
docker-compose run --rm bctl

# Inside the shell:
bctl status
bctl nodes list
bctl nodes info 0
```

### 3. Connect to pgBalancer

```bash
# Connect via psql
psql -h localhost -p 9999 -U postgres -d testdb

# Run test queries
SELECT * FROM users;
SELECT * FROM orders;
```

### 4. Access REST API

```bash
# Check status via REST API
curl http://localhost:8080/api/v1/status

# List nodes
curl http://localhost:8080/api/v1/nodes
```

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Client Applications                   │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
          ┌──────────────────────┐
          │    pgBalancer        │  ◄── bctl (management)
          │  Port: 9999          │
          │  REST API: 8080      │
          └──────────┬───────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
        ▼            ▼            ▼
  ┌─────────┐  ┌─────────┐  ┌─────────┐
  │Primary  │  │Standby 1│  │Standby 2│
  │5432     │  │5433     │  │5434     │
  └─────────┘  └─────────┘  └─────────┘
```

## 📁 Directory Structure

```
cluster/
├── Dockerfile                 # Multi-stage pgBalancer build
├── docker-compose.yml         # Full cluster orchestration
├── README.md                  # This file
├── .env.example              # Environment variables template
├── config/                    # Configuration files
│   └── pgbalancer.conf.sample
├── scripts/                   # Helper scripts
│   ├── docker-entrypoint.sh  # Container initialization
│   ├── healthcheck.sh        # Health check script
│   └── bctl-wrapper.sh       # bctl interactive shell
├── postgres/                  # PostgreSQL initialization
│   ├── primary/
│   │   ├── init.sql
│   │   └── postgresql.conf
│   ├── standby1/
│   │   └── init.sql
│   └── standby2/
│       └── init.sql
└── examples/                  # Usage examples
    ├── basic-usage.sh
    ├── failover-test.sh
    └── load-test.sh
```

## 🔧 Configuration

### Environment Variables

Create a `.env` file based on `.env.example`:

```env
# PostgreSQL Backend Configuration
BACKEND0_HOST=postgres-primary
BACKEND0_PORT=5432
BACKEND1_HOST=postgres-standby1
BACKEND1_PORT=5432
BACKEND2_HOST=postgres-standby2
BACKEND2_PORT=5432

# Connection Pool
NUM_INIT_CHILDREN=32
MAX_POOL=4

# Load Balancing
LOAD_BALANCE_MODE=on

# REST API
ENABLE_REST_API=on
REST_API_PORT=8080

# Health Check
HEALTH_CHECK_PERIOD=10
HEALTH_CHECK_USER=postgres
HEALTH_CHECK_PASSWORD=postgres
```

### Custom Configuration

Mount custom configuration files:

```yaml
volumes:
  - ./config/custom-pgbalancer.conf:/etc/pgbalancer/pgbalancer.conf:ro
```

## 📊 Management with bctl

### Interactive Shell

```bash
# Start bctl container
docker-compose run --rm bctl

# Inside the container:
bctl status                    # Show status
bctl nodes list                # List all nodes
bctl nodes info 0              # Get node 0 info
bctl nodes detach 1            # Detach node 1
bctl nodes attach 1            # Attach node 1
bctl reload                    # Reload config
bctl cache invalidate          # Clear query cache
```

### Direct Commands

```bash
# Check status from host
docker-compose exec pgbalancer bctl --host localhost --port 8080 status

# List nodes
docker-compose exec pgbalancer bctl --host localhost --port 8080 nodes list
```

## 🧪 Testing

### Basic Functionality Test

```bash
# Run basic usage examples
docker-compose exec bctl /examples/basic-usage.sh
```

### Load Testing

```bash
# Generate load
docker-compose exec bctl /examples/load-test.sh
```

### Failover Testing

```bash
# Test failover scenario
docker-compose exec bctl /examples/failover-test.sh
```

## 📈 Monitoring

### Health Checks

```bash
# Check container health
docker-compose ps

# View health check logs
docker inspect --format='{{json .State.Health}}' pgbalancer | jq
```

### Logs

```bash
# View pgBalancer logs
docker-compose logs -f pgbalancer

# View all logs
docker-compose logs -f

# View specific service logs
docker-compose logs -f postgres-primary
```

### Metrics

```bash
# Enable monitoring profile
docker-compose --profile monitoring up -d pgbalancer-exporter

# Access metrics
curl http://localhost:8080/api/v1/status | jq
```

## 🔒 Security

### Production Checklist

- [ ] Enable `pool_hba.conf` authentication
- [ ] Set strong PCP password
- [ ] Enable SSL/TLS
- [ ] Configure REST API authentication
- [ ] Restrict network access
- [ ] Use secrets management
- [ ] Enable audit logging
- [ ] Regular security updates

### Enable Authentication

```env
ENABLE_POOL_HBA=on
REST_API_AUTH_USER=admin
REST_API_AUTH_PASSWORD=secure_password
PCP_PASSWORD=secure_pcp_password
```

## 🐛 Troubleshooting

### pgBalancer Won't Start

```bash
# Check logs
docker-compose logs pgbalancer

# Verify backends are accessible
docker-compose exec pgbalancer pg_isready -h postgres-primary -p 5432
```

### Connection Refused

```bash
# Check if pgBalancer is listening
docker-compose exec pgbalancer netstat -tulpn | grep 9999

# Test REST API
curl -v http://localhost:8080/api/v1/status
```

### Backend Node Down

```bash
# Check node status
bctl nodes list

# Detach failed node
bctl nodes detach <node_id>

# Reattach when ready
bctl nodes attach <node_id>
```

## 📚 Advanced Usage

### Watchdog Configuration

Enable watchdog for automatic failover:

```env
USE_WATCHDOG=on
WD_HOSTNAME=pgbalancer
WD_PORT=9000
WD_PRIORITY=1
```

### Query Cache

Enable in-memory query cache:

```env
MEMQCACHE_ENABLED=on
MEMQCACHE_METHOD=shmem
MEMQCACHE_TOTAL_SIZE=67108864
```

### Scaling

Scale standby nodes:

```bash
# Add more standby nodes in docker-compose.yml
# Then update pgbalancer configuration
docker-compose up -d --scale postgres-standby=5
```

## 🚦 Stopping the Cluster

```bash
# Stop all services
docker-compose down

# Stop and remove volumes (WARNING: deletes data)
docker-compose down -v

# Stop and remove images
docker-compose down --rmi all
```

## 📖 Documentation

- [pgBalancer Documentation](../README.md)
- [bctl User Guide](../bctl/README.md)
- [REST API Reference](../docs/rest-api.md)
- [Configuration Reference](../docs/configuration.md)

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](../CONTRIBUTING.md)

## 📄 License

Same as pgBalancer project.

## 🆘 Support

- GitHub Issues: [pgBalancer Repository]
- Documentation: [pgBalancer Docs]
- Community: [pgBalancer Community]

---

**Happy Load Balancing! 🐘⚡**

