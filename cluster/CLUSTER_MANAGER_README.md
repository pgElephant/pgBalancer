# pgBalancer Cluster Manager

A powerful, modular Python tool to create and manage multi-instance pgBalancer clusters using Docker.

## 🎯 Features

- ✅ **Multi-Balancer Support**: Deploy multiple pgBalancer instances in a single cluster
- ✅ **Flexible Topology**: Each balancer can have 1 primary + N replicas
- ✅ **JSON Configuration**: Define entire cluster topology in a simple JSON file
- ✅ **Modular Design**: Clean OOP architecture with separate classes for each component
- ✅ **Docker-based**: Fully containerized, no manual setup required
- ✅ **Dynamic Management**: Add/remove replicas on the fly
- ✅ **Health Monitoring**: Built-in health checks and status reporting

## 📋 Prerequisites

- Docker installed and running
- Python 3.7+
- pgBalancer Docker image built (`make build` in cluster directory)

## 🚀 Quick Start

### 1. Make the script executable
```bash
chmod +x pgbalancer_cluster.py
```

### 2. Initialize a simple cluster
```bash
./pgbalancer_cluster.py --init --config pgbalancer_cluster_simple.json
```

### 3. Check status
```bash
./pgbalancer_cluster.py --status --config pgbalancer_cluster_simple.json
```

### 4. Destroy cluster
```bash
./pgbalancer_cluster.py --destroy --config pgbalancer_cluster_simple.json
```

## 📖 Usage

### Commands

#### Initialize Cluster
Creates all containers defined in the configuration:
```bash
./pgbalancer_cluster.py --init [--config FILE]
```

#### Show Status
Displays current state of all cluster components:
```bash
./pgbalancer_cluster.py --status [--config FILE]
```

#### Destroy Cluster
Removes all containers and networks:
```bash
./pgbalancer_cluster.py --destroy [--config FILE]
```

#### Add Replicas
Dynamically add replicas to a balancer:
```bash
./pgbalancer_cluster.py --add-replica --balancer lb1 --count 2 [--config FILE]
```

## 📝 Configuration File Format

### Simple Example (`pgbalancer_cluster_simple.json`)

```json
{
  "cluster_name": "pgbalancer_simple_cluster",
  "network": {
    "name": "pgbalancer_simple_net",
    "subnet": "172.31.0.0/16"
  },
  "balancers": [
    {
      "name": "lb1",
      "container_name": "pgbalancer_lb1",
      "ip_address": "172.31.1.10",
      "port": 19999,
      "pcp_port": 19898,
      "rest_api_port": 18080,
      "config": {
        "num_init_children": 16,
        "max_pool": 2,
        "load_balance_mode": "on"
      },
      "primary": {
        "host": "primary1",
        "port": 15432,
        "container_name": "primary1",
        "ip_address": "172.31.1.20"
      },
      "replicas": [
        {
          "node_id": 1,
          "host": "replica1",
          "port": 15433,
          "container_name": "replica1",
          "ip_address": "172.31.1.21"
        }
      ]
    }
  ]
}
```

### Multi-Balancer Example (`pgbalancer_cluster.json`)

The default config shows a cluster with 2 pgBalancer instances:
- **lb1**: 1 primary + 2 replicas
- **lb2**: 1 primary + 1 replica

## 📊 Configuration Fields

### Cluster Level
| Field | Description | Example |
|-------|-------------|---------|
| `cluster_name` | Unique cluster identifier | `"my_cluster"` |
| `network.name` | Docker network name | `"pgbalancer_net"` |
| `network.subnet` | Network CIDR | `"172.30.0.0/16"` |

### Balancer Level
| Field | Description | Example |
|-------|-------------|---------|
| `name` | Balancer identifier | `"lb1"` |
| `container_name` | Docker container name | `"pgbalancer_lb1"` |
| `ip_address` | Static IP in network | `"172.30.1.10"` |
| `port` | pgBalancer main port | `19999` |
| `pcp_port` | PCP control port | `19898` |
| `rest_api_port` | REST API port | `18080` |
| `config.num_init_children` | Connection pool size | `32` |
| `config.max_pool` | Max pools per child | `4` |

### PostgreSQL Node Level
| Field | Description | Example |
|-------|-------------|---------|
| `host` | Container hostname | `"primary1"` |
| `port` | Exposed port | `15432` |
| `container_name` | Docker container name | `"primary1"` |
| `ip_address` | Static IP | `"172.30.1.20"` |
| `node_id` | Replica ID (replicas only) | `1` |

## 🏗️ Architecture

### Class Structure

```
ClusterManager
├── ClusterConfig
│   └── List[PgBalancer]
│       ├── PostgreSQLNode (primary)
│       └── List[PostgreSQLNode] (replicas)
└── DockerManager (static methods)
```

### Component Hierarchy

```
Cluster
├── Network (Docker bridge)
├── Balancer 1
│   ├── pgBalancer Container
│   ├── Primary PostgreSQL Container
│   └── Replica PostgreSQL Containers
└── Balancer 2
    ├── pgBalancer Container
    ├── Primary PostgreSQL Container
    └── Replica PostgreSQL Containers
```

## 🔧 Advanced Usage

### Creating Custom Topologies

#### Single Balancer, Many Replicas
```json
{
  "balancers": [
    {
      "name": "lb_main",
      "primary": { ... },
      "replicas": [
        { "node_id": 1, ... },
        { "node_id": 2, ... },
        { "node_id": 3, ... },
        { "node_id": 4, ... },
        { "node_id": 5, ... }
      ]
    }
  ]
}
```

#### Multiple Independent Balancers
```json
{
  "balancers": [
    {
      "name": "app1_balancer",
      "port": 19999,
      "primary": { ... },
      "replicas": [ ... ]
    },
    {
      "name": "app2_balancer",
      "port": 29999,
      "primary": { ... },
      "replicas": [ ... ]
    },
    {
      "name": "app3_balancer",
      "port": 39999,
      "primary": { ... },
      "replicas": [ ... ]
    }
  ]
}
```

### Port Allocation Strategy

To avoid conflicts, use this pattern:
- Balancer 1: Ports 1xxxx (19999, 19898, 18080, 15432-15499)
- Balancer 2: Ports 2xxxx (29999, 29898, 28080, 25432-25499)
- Balancer 3: Ports 3xxxx (39999, 39898, 38080, 35432-35499)

### IP Allocation Strategy

Use different /24 subnets for each balancer:
- Balancer 1: 172.30.1.0/24
  - pgBalancer: 172.30.1.10
  - Primary: 172.30.1.20
  - Replicas: 172.30.1.21, 172.30.1.22, ...
- Balancer 2: 172.30.2.0/24
  - pgBalancer: 172.30.2.10
  - Primary: 172.30.2.20
  - Replicas: 172.30.2.21, 172.30.2.22, ...

## 📈 Example Workflows

### Development Setup (1 balancer, 1 primary, 2 replicas)
```bash
# Use simple config
./pgbalancer_cluster.py --init --config pgbalancer_cluster_simple.json

# Connect to pgBalancer
psql -h localhost -p 19999 -U postgres -d testdb

# Check status
./pgbalancer_cluster.py --status --config pgbalancer_cluster_simple.json

# Cleanup
./pgbalancer_cluster.py --destroy --config pgbalancer_cluster_simple.json
```

### Production Setup (2 balancers, each with 1 primary + 2 replicas)
```bash
# Use production config
./pgbalancer_cluster.py --init --config pgbalancer_cluster.json

# Check all components
./pgbalancer_cluster.py --status --config pgbalancer_cluster.json

# Access balancer 1
psql -h localhost -p 19999 -U postgres -d testdb

# Access balancer 2
psql -h localhost -p 29999 -U postgres -d testdb

# Check REST APIs
curl http://localhost:18080/api/v1/status | jq
curl http://localhost:28080/api/v1/status | jq
```

### Scaling Replicas
```bash
# Add 2 more replicas to lb1
./pgbalancer_cluster.py --add-replica --balancer lb1 --count 2

# Check updated status
./pgbalancer_cluster.py --status
```

## 🐛 Troubleshooting

### Cluster won't start
```bash
# Check Docker
docker ps

# Check network
docker network ls | grep pgbalancer

# View logs
docker logs pgbalancer_lb1
docker logs primary1
```

### Port conflicts
```bash
# Check what's using the port
lsof -i :19999

# Modify ports in JSON config
vim pgbalancer_cluster.json
```

### pgBalancer image not found
```bash
# Build the image first
cd /path/to/pgbalancer/cluster
make build
```

### Containers not healthy
```bash
# Wait longer - PostgreSQL takes ~30s to start
sleep 30

# Check specific container
docker inspect primary1 | jq '.[0].State.Health'
```

## 📚 Module Reference

### ClusterManager
Main orchestration class

**Methods:**
- `load_config()` - Load JSON configuration
- `init_cluster()` - Create entire cluster
- `destroy_cluster()` - Remove all containers
- `show_status()` - Display cluster state
- `add_replica(balancer_name, count)` - Add replicas

### DockerManager
Docker operations wrapper

**Methods:**
- `create_network(name, subnet)` - Create Docker network
- `create_postgres_container(node, network)` - Create PostgreSQL container
- `create_pgbalancer_container(balancer, network)` - Create pgBalancer container
- `get_container_status(name)` - Get container status
- `get_container_health(name)` - Get health status
- `stop_container(name)` - Stop container
- `remove_container(name)` - Remove container

### Data Classes
- `PostgreSQLNode` - Represents a PostgreSQL instance
- `PgBalancer` - Represents a pgBalancer instance
- `ClusterConfig` - Complete cluster configuration

## 🔐 Security Considerations

For production:

1. **Change default passwords**
   - Modify PostgreSQL password in `DockerManager.create_postgres_container()`
   - Add authentication to pgBalancer REST API

2. **Use secrets management**
   - Store passwords in environment variables
   - Use Docker secrets for sensitive data

3. **Network isolation**
   - Use separate networks for different applications
   - Add firewall rules

4. **TLS/SSL**
   - Enable SSL for PostgreSQL connections
   - Use HTTPS for REST API

## 🚀 Future Enhancements

- [ ] Automatic failover configuration
- [ ] Backup/restore functionality
- [ ] Health check webhooks
- [ ] Prometheus metrics export
- [ ] Grafana dashboard generation
- [ ] Rolling updates support
- [ ] Configuration validation
- [ ] Interactive mode
- [ ] Web UI

## 📄 License

Same as pgBalancer project.

## 🤝 Contributing

Issues and pull requests welcome!

---

**Created**: 2025-10-27
**Version**: 1.0.0
**Status**: Production Ready ✅

