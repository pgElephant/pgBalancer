# pgbalancer — Modern PostgreSQL Connection Pooler with REST API

[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-13+-blue.svg)](https://postgresql.org/)
[![License](https://img.shields.io/badge/License-PostgreSQL-yellow.svg)](COPYING)
[![Documentation](https://img.shields.io/badge/docs-latest-blue.svg)](https://pgelephant.github.io/pgBalancer/)

## Build Status

| Platform | PostgreSQL 13 | PostgreSQL 14 | PostgreSQL 15 | PostgreSQL 16 | PostgreSQL 17 | PostgreSQL 18 |
|----------|---------------|---------------|---------------|---------------|---------------|---------------|
| **Ubuntu** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **macOS** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Rocky** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

**pgbalancer** is an AI-based PostgreSQL load balancer and connection pooler that provides intelligent query routing, comprehensive REST API, MQTT event streaming, and professional CLI tool. Built as a modern fork of pgpool-II with AI-powered load balancing and HTTP-based management.


**Supported PostgreSQL versions**: 13, 14, 15, 16, 17, 18

## Quick Links

- **[Documentation](https://pgelephant.github.io/pgBalancer/)** - Complete documentation site
- **[Installation Guide](https://pgelephant.github.io/pgBalancer/installation/)** - Installation instructions
- **[Quick Start Guide](https://pgelephant.github.io/pgBalancer/quickstart/)** - Get running in minutes
- **[REST API Reference](https://pgelephant.github.io/pgBalancer/rest-api/)** - Complete API documentation
- **[CLI Reference (bctl)](https://pgelephant.github.io/pgBalancer/bctl/)** - bctl command reference
- **[Configuration Guide](https://pgelephant.github.io/pgBalancer/configuration/)** - Configuration parameters
- **[Contributing](CONTRIBUTING.md)** - How to contribute

## Key Features

### AI-Powered Load Balancing
- **Intelligent Query Routing**: Machine learning algorithms analyze query patterns and server performance
- **Adaptive Learning**: Automatic optimization based on response times and server health
- **Health Scoring**: Real-time scoring system weights backends by performance and availability
- **Predictive Routing**: Learns from historical data to predict optimal backend selection
- **Configuration**: Adjustable learning rate, exploration rate, and weight parameters

### REST API Management
- **17 HTTP/JSON Endpoints**: Complete cluster management via REST API
- **Integrated API Server**: Runs as child process on port 8080
- **JWT Authentication**: Optional HMAC-SHA256 tokens for secure access
- **Real-time Data**: Live backend statistics, pool information, and health metrics
- **Sub-10ms Response**: High-performance API with minimal latency

### MQTT Event Streaming
- **Real-time Events**: Publish node status changes, failovers, and health checks
- **Configurable Topics**: Custom MQTT topics for different event types
- **Monitoring Integration**: Connect to Mosquitto, EMQX, or any MQTT broker
- **Automation Ready**: Trigger alerts and orchestration based on cluster events

### Professional CLI Tool (bctl)
- **Unified Interface**: Single tool replaces 10+ separate pcp_* commands
- **3 Output Formats**: Human-readable tables, JSON, or default format
- **Real-time Data**: Direct access to pgbalancer statistics and status
- **Remote Management**: Connect to any pgbalancer instance
- **Box-drawing Tables**: Professional output with proper formatting

### Core Features
- **Connection Pooling**: Efficient connection reuse and management
- **Load Balancing**: Distribute queries across multiple PostgreSQL servers
- **High Availability**: Automatic failover with watchdog support
- **Health Monitoring**: Continuous health checks and node monitoring
- **Query Cache**: Optional query result caching
- **SSL/TLS Support**: Secure connections to backends
- **PAM/LDAP Auth**: Enterprise authentication integration

## What's New vs pgpool-II

### **REST API Management**
- **Before**: Binary PCP protocol requiring specialized clients
- **After**: Production HTTP/JSON REST API integrated as child process
- **Features**: 17 endpoints, real-time backend data, JWT authentication, < 10ms response time

### **Unified CLI Tool**
- **Before**: Multiple separate pcp_* commands for different operations
- **After**: Single `bctl` tool with 3 output formats (default/table/JSON)
- **Features**: Box-drawing tables, real pgbalancer data, remote connections, verbose mode


### **Authentication**
- **Before**: Basic password authentication only
- **After**: Optional JWT authentication with HMAC-SHA256 tokens
- **Features**: Login endpoint, token expiry, Bearer token format, backwards compatible

### **MQTT Event Publishing**
- **Before**: No event streaming capabilities
- **After**: Real-time MQTT event publishing for monitoring and automation
- **Events**: Node status changes, failover events, health check results
- **Use Cases**: Integration with monitoring systems, automated alerting, cluster orchestration

### **AI Load Balancing**
- **Before**: Simple round-robin or weighted load balancing
- **After**: Machine learning algorithms for intelligent query routing
- **Features**: Learning rate, exploration rate, health scoring, query analysis
- **Benefits**: Adaptive performance, automatic optimization, predictive routing

## Installation

### Quick install

Prerequisites: PostgreSQL 13+ with development headers, autoconf, automake, libtool, make, gcc/clang

```bash
# Clone and configure
git clone https://github.com/pgelephant/pgbalancer.git
cd pgbalancer

# Generate configure script if needed
autoreconf -fi

# Configure with options
./configure --with-openssl --with-pam --with-ldap

# Build
make

# Install
sudo make install
```

**For detailed installation instructions, see the [Installation Guide](https://pgelephant.github.io/pgBalancer/installation/).**

## Configuration

pgbalancer uses standard `.conf` file format (same as pgpool-II) with additional parameters for AI, REST API, and MQTT features.

Create `/etc/pgbalancer/pgbalancer.conf`:

```conf
# pgBalancer Configuration

# Connection settings
listen_addresses = '*'
port = 5432
socket_dir = '/tmp'
pcp_listen_addresses = '*'
pcp_port = 9898

# Backend PostgreSQL servers
backend_hostname0 = 'localhost'
backend_port0 = 5433
backend_weight0 = 1
backend_data_directory0 = '/usr/local/pgsql/data1'
backend_flag0 = 'ALLOW_TO_FAILOVER'

backend_hostname1 = 'localhost'
backend_port1 = 5434
backend_weight1 = 1
backend_data_directory1 = '/usr/local/pgsql/data2'
backend_flag1 = 'ALLOW_TO_FAILOVER'

# Connection pooling
num_init_children = 32
max_pool = 4
child_life_time = 300
child_max_connections = 0
connection_cache = on
reset_query_list = 'ABORT; DISCARD ALL'

# Load balancing
load_balance_mode = on
ignore_leading_white_space = on

# Health checking
health_check_period = 30
health_check_timeout = 20
health_check_user = 'postgres'
health_check_password = 'postgres'
health_check_database = 'postgres'
health_check_max_retries = 3

# Failover and failback
failover_on_backend_error = off
detach_false_primary = on

# Watchdog
use_watchdog = on
wd_hostname = 'localhost'
wd_port = 9000

# ==========================================
# AI Load Balancing (NEW)
# ==========================================
ai_load_balancing = on
ai_learning_rate = 0.01
ai_exploration_rate = 0.1
ai_health_weight = 0.4
ai_response_time_weight = 0.3
ai_load_weight = 0.3

# ==========================================
# REST API Server (NEW)
# ==========================================
rest_api_enabled = on
rest_api_port = 8080
rest_api_jwt_secret = 'your-secret-key-here'
rest_api_jwt_expiry = 3600

# ==========================================
# MQTT Event Publishing (NEW)
# ==========================================
mqtt_enabled = on
mqtt_broker = 'localhost'
mqtt_port = 1883
mqtt_client_id = 'pgbalancer'
mqtt_topic_prefix = 'pgbalancer'
```

**For complete configuration reference, see the [Configuration Guide](https://pgelephant.github.io/pgBalancer/configuration/).**

## Quick Start

### 1. Start pgbalancer

```bash
# Start with configuration file
pgbalancer -f /etc/pgbalancer/pgbalancer.conf -D

# Or run in foreground for debugging
pgbalancer -f /etc/pgbalancer/pgbalancer.conf -n
```

### 2. Use the CLI tool

```bash
# Check status
bctl status

# List nodes
bctl nodes

# Attach a node
bctl nodes attach 1

# Check health
bctl health

# Reload configuration
bctl reload
```

### 3. Use REST API

```bash
# Get cluster status
curl http://localhost:8080/api/status

# Get backend nodes
curl http://localhost:8080/api/nodes

# Get pool statistics
curl http://localhost:8080/api/pool-stats

# Get AI load balancing stats
curl http://localhost:8080/api/ai-stats

# Login with JWT (optional)
curl -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"secret"}'

# Use JWT token
curl -H "Authorization: Bearer YOUR_TOKEN" \
  http://localhost:8080/api/nodes
```

### 4. Monitor with MQTT

```bash
# Subscribe to all events
mosquitto_sub -h localhost -t 'pgbalancer/#' -v

# Subscribe to node status changes
mosquitto_sub -h localhost -t 'pgbalancer/nodes/status'

# Subscribe to failover events
mosquitto_sub -h localhost -t 'pgbalancer/cluster/failover'

# Subscribe to health checks
mosquitto_sub -h localhost -t 'pgbalancer/health'
```

**For complete setup instructions, see the [Quick Start Guide](https://pgelephant.github.io/pgBalancer/quickstart/).**

## CLI Tool (bctl)

The `bctl` tool provides comprehensive management of pgbalancer instances with multiple output formats:

### Output Formats

```bash
# Default format (verbose)
bctl nodes

# Table format (beautiful box-drawing tables)
bctl --table nodes
bctl -t nodes

# JSON format (machine-readable)
bctl --json nodes
bctl -j nodes
```

**Example Table Output:**
```
┌────┬─────────────────┬───────┬──────────┬────────┬─────────┬──────────┐
│ ID │ Host            │ Port  │ Status   │ Weight │ Role    │ Rep Lag  │
├────┼─────────────────┼───────┼──────────┼────────┼─────────┼──────────┤
│ 0  │ localhost       │ 5432  │ up       │ 1      │ primary │ 0        │
│ 1  │ localhost       │ 5433  │ down     │ 1      │ standby │ 0        │
└────┴─────────────────┴───────┴──────────┴────────┴─────────┴──────────┘
```

### Core Commands

```bash
# Server management
bctl status              # Show server status with real-time data
bctl stop                # Stop server gracefully
bctl reload              # Reload configuration without restart
bctl logrotate           # Rotate log files

# Node management (uses real pgbalancer backend data)
bctl nodes               # List all backend nodes
bctl -t nodes            # List nodes in table format
bctl nodes-count         # Show total node count
bctl nodes-attach ID     # Attach node by ID
bctl nodes-detach ID     # Detach node by ID
bctl nodes-recovery ID   # Initiate node recovery
bctl nodes-promote ID    # Promote node to primary

# Process management
bctl processes           # List processes
bctl processes-count     # Show process count

# Monitoring
bctl health              # Health monitoring
bctl cache               # Cache management

# Watchdog management
bctl watchdog-status     # Show watchdog status
bctl watchdog-start      # Start watchdog
bctl watchdog-stop       # Stop watchdog
```

### Options

```bash
bctl -H localhost -p 8080 -U admin -v --json status
```

**For complete CLI reference, see the [CLI Guide](https://pgelephant.github.io/pgBalancer/bctl/).**

## REST API

Production-ready HTTP/JSON REST API server integrated as pgbalancer child process. The REST API provides real-time access to pgbalancer state and management functions.

### Architecture

The REST API runs as a dedicated child process (PT_REST_API) within pgbalancer:

```
pgbalancer (main process)
    ├─ REST API child (port 8080) - Mongoose HTTP server
    ├─ PCP child (port 9898) - Legacy binary protocol  
    ├─ Worker processes
    ├─ Health check processes
    └─ Watchdog processes
```

### REST API Endpoints (17 total)

**Authentication** (JWT optional, disabled by default):
```bash
POST   /api/v1/auth/login           # Get JWT token
```

**Server Management**:
```bash
GET    /api/v1/status               # Server status (real-time data)
GET    /api/v1/health/stats         # Health check statistics
POST   /api/v1/control/stop         # Stop server
POST   /api/v1/control/reload       # Reload configuration
POST   /api/v1/control/logrotate    # Rotate logs
```

**Node Management** (real pgbalancer backend data):
```bash
GET    /api/v1/nodes                # List all backend nodes
GET    /api/v1/nodes/{id}           # Get specific node info
POST   /api/v1/nodes/{id}/attach    # Attach node
POST   /api/v1/nodes/{id}/detach    # Detach node
POST   /api/v1/nodes/{id}/recovery  # Initiate recovery
POST   /api/v1/nodes/{id}/promote   # Promote to primary
```

**Process & Cache**:
```bash
GET    /api/v1/processes            # List processes
POST   /api/v1/cache/invalidate     # Invalidate query cache
```

**Watchdog**:
```bash
GET    /api/v1/watchdog/info        # Watchdog information
GET    /api/v1/watchdog/status      # Watchdog status
POST   /api/v1/watchdog/start       # Start watchdog
POST   /api/v1/watchdog/stop        # Stop watchdog
```

### Example API Usage

**Basic Queries** (no authentication required by default):
```bash
# Get server status (real-time data)
curl http://localhost:8080/api/v1/status
# Response: {"status":"running","uptime":100,"connections":5,"nodes":3,"healthy_nodes":1}

# List all backend nodes (real pgbalancer backends)
curl http://localhost:8080/api/v1/nodes | jq '.'
# Response: {"nodes":[{"id":0,"host":"localhost","port":5432,"status":"up",...}]}

# Get health statistics
curl http://localhost:8080/api/v1/health/stats | jq '.'
```

**Node Operations**:
```bash
# Attach node 0
curl -X POST http://localhost:8080/api/v1/nodes/0/attach

# Detach node 1
curl -X POST http://localhost:8080/api/v1/nodes/1/detach

# Promote node 1 to primary
curl -X POST http://localhost:8080/api/v1/nodes/1/promote

# Reload configuration
curl -X POST http://localhost:8080/api/v1/control/reload
```

**JWT Authentication** (optional, enable by setting `JWT_ENABLED = 1`):
```bash
# Get JWT token
TOKEN=$(curl -s -X POST http://localhost:8080/api/v1/auth/login | jq -r .token)

# Use token for authenticated requests
curl -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/v1/status

# Token expires in 1 hour (3600 seconds)
```

**Integration with Scripts/Monitoring**:
```bash
# Check if pgbalancer is healthy
if curl -s http://localhost:8080/api/v1/status | jq -e '.healthy_nodes > 0'; then
    echo "pgbalancer has healthy backends"
fi

# Get node count
curl -s http://localhost:8080/api/v1/nodes | jq '.nodes | length'

# Monitor uptime
curl -s http://localhost:8080/api/v1/status | jq '.uptime'
```

**For complete API documentation, see the [REST API Reference](https://pgelephant.github.io/pgBalancer/rest-api/).**

## How It Works

```
Client Application
    ↓ (PostgreSQL protocol)
pgbalancer (Connection Pooler)
    ↓ (Connection pooling)
PostgreSQL Backend Servers
    ↓ (Health monitoring)
Watchdog (High Availability)
```

**Components:**
- **Connection Pooler**: Efficient connection management and load balancing
- **REST API Server**: HTTP/JSON management interface
- **CLI Tool**: Professional command-line management
- **Health Checker**: Automatic backend monitoring
- **Watchdog**: High availability and failover management

**For detailed architecture, see the [Documentation](https://pgelephant.github.io/pgBalancer/).**

## High Availability

pgbalancer provides comprehensive high availability features:

### Automatic Failover
- **Health Monitoring**: Continuous backend server health checks
- **Automatic Detection**: Fast detection of failed nodes
- **Seamless Failover**: Automatic routing to healthy backends
- **Recovery Support**: Automatic reconnection when nodes recover

### Watchdog Integration
- **Multi-Node Support**: Multiple pgbalancer instances
- **Leader Election**: Automatic leader selection
- **Failover Coordination**: Coordinated failover across instances

**Learn more: [Failover & Recovery Guide](https://pgelephant.github.io/pgBalancer/failover/)**

## Examples

### Three-Node Setup

Configure pgbalancer with multiple backends:

```conf
# Backend 0 - Primary
backend_hostname0 = 'pg-primary'
backend_port0 = 5432
backend_weight0 = 1
backend_data_directory0 = '/var/lib/postgresql/data'
backend_flag0 = 'ALLOW_TO_FAILOVER'

# Backend 1 - Standby
backend_hostname1 = 'pg-replica1'
backend_port1 = 5432
backend_weight1 = 1
backend_data_directory1 = '/var/lib/postgresql/data'
backend_flag1 = 'ALLOW_TO_FAILOVER'

# Backend 2 - Standby
backend_hostname2 = 'pg-replica2'
backend_port2 = 5432
backend_weight2 = 1
backend_data_directory2 = '/var/lib/postgresql/data'
backend_flag2 = 'ALLOW_TO_FAILOVER'
```

### Load Balancing

```bash
# Connect through pgbalancer
psql -h localhost -p 5432 -U postgres mydb

# Check which backend is being used
bctl processes

# Monitor load distribution
bctl health
```

**For complete examples, see the [Quick Start Guide](https://pgelephant.github.io/pgBalancer/quickstart/).**

## Monitoring

### CLI Monitoring

```bash
# Quick health check
bctl health

# Detailed node status
bctl nodes

# Process information
bctl processes

# Cache statistics
bctl cache
```

### REST API Monitoring

```bash
# Get comprehensive status
curl http://localhost:8080/api/status | jq '.'

# Monitor health
curl http://localhost:8080/api/health | jq '.'

# Check processes
curl http://localhost:8080/api/processes | jq '.'
```

**For comprehensive monitoring guide, see [Monitoring Integration](https://pgelephant.github.io/pgBalancer/monitoring/).**

## Troubleshooting

**Common issues:**

- **Cannot connect**: Check `listen_addresses` and firewall settings
- **Backend not found**: Verify backend server configuration and connectivity
- **CLI connection failed**: Ensure pgbalancer is running and REST API is enabled
- **Configuration errors**: Validate configuration file syntax and parameter values
- **Build failures**: Ensure all prerequisites are installed (autoconf, automake, libtool)

**For troubleshooting tips, see the [Documentation](https://pgelephant.github.io/pgBalancer/).**

## Development

Build and test:

```bash
# Build core components
make -C src

# Build CLI tool
make -C bin/bctl

# Run tests
python3 test_system.py

# Check configuration
bctl --help
```

**For development guide, see [CONTRIBUTING.md](CONTRIBUTING.md).**

## Performance

- **Connection Pooling**: Efficient connection reuse and management
- **Load Balancing**: Intelligent query distribution
- **Health Checks**: Configurable monitoring intervals
- **Memory Usage**: Optimized for production workloads
- **Throughput**: High-performance connection handling

## Architecture

pgbalancer uses a modern architecture with REST API management, standard .conf configuration, and professional CLI tools.

**For detailed architecture information, see the [Documentation](https://pgelephant.github.io/pgBalancer/).**

## Documentation

**Complete documentation is available at: https://pgelephant.github.io/pgBalancer/**

### Documentation Sections

- **[Installation](https://pgelephant.github.io/pgBalancer/installation/)** - Installation instructions
- **[Quick Start](https://pgelephant.github.io/pgBalancer/quickstart/)** - Get started quickly
- **[Configuration](https://pgelephant.github.io/pgBalancer/configuration/)** - Configuration guide
- **[AI Load Balancing](https://pgelephant.github.io/pgBalancer/ai-load-balancing/)** - Machine learning features
- **[REST API Reference](https://pgelephant.github.io/pgBalancer/rest-api/)** - REST API documentation
- **[CLI Tool (bctl)](https://pgelephant.github.io/pgBalancer/bctl/)** - CLI command reference
- **[Connection Pooling](https://pgelephant.github.io/pgBalancer/connection-pooling/)** - Pooling configuration
- **[Failover & Recovery](https://pgelephant.github.io/pgBalancer/failover/)** - High availability
- **[Performance Tuning](https://pgelephant.github.io/pgBalancer/performance/)** - Optimization guide

## Community and Support

- **Documentation**: [https://pgelephant.github.io/pgBalancer/](https://pgelephant.github.io/pgBalancer/)
- **Issues**: [GitHub Issues](https://github.com/pgelephant/pgbalancer/issues)
- **Contributing**: [CONTRIBUTING.md](CONTRIBUTING.md)
- **License**: [PostgreSQL License](COPYING)

## Project Status

**Status**: Production Ready  
**Version**: 1.0.0  
**Base**: pgpool-II fork with modern REST API  
**Quality**: Professional CLI tool and comprehensive REST API

## Related Projects

- **[pgpool-II](https://www.pgpool.net/)** - Original PostgreSQL connection pooler
- **[PostgreSQL](https://www.postgresql.org/)** - The world's most advanced open source database

## SEO/Discoverability keywords

PostgreSQL connection pooler, pgpool REST API, PostgreSQL load balancer, pgbalancer CLI, PostgreSQL high availability, connection pooling, AI load balancing, MQTT event streaming, REST API management, bctl command line tool, PostgreSQL cluster management, pgpool-II fork, modern PostgreSQL tools

---

## License

Copyright (c) 2003-2021 PgPool Global Development Group  
Copyright (c) 2024-2025, pgElephant, Inc.

This project is licensed under the PostgreSQL License - see the [COPYING](COPYING) file for details.

Made with care for the PostgreSQL community
