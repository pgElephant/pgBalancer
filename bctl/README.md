# bctl - PgBalancer Control Client

**bctl** is a completely independent, professional Linux-style command-line client for managing PgBalancer instances via REST API.

## Features

- **Standalone**: Completely independent of pgbalancer build system
- **Professional**: Clean, Linux-style command-line interface
- **REST API**: Modern HTTP/JSON communication (no legacy protocols)
- **Zero Dependencies**: Only requires libcurl and json-c
- **Lightweight**: Single binary with minimal footprint

## Installation

### Prerequisites

```bash
# macOS (using Homebrew)
brew install curl json-c

# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev libjson-c-dev

# CentOS/RHEL
sudo yum install libcurl-devel json-c-devel
```

### Build

```bash
# Clone and build
cd bctl
make

# Or build with custom options
CC=clang make
```

### Install

```bash
# Install to system
sudo make install

# Or install to custom location
make install DESTDIR=/opt/pgbalancer
```

## Usage

### Basic Commands

```bash
# Show help
bctl --help

# Show server status
bctl status

# Show server status (custom host/port)
bctl status --host 192.168.1.100 --port 8080

# Stop server
bctl stop

# Reload configuration
bctl reload

# Invalidate query cache
bctl cache invalidate
```

### Node Management

```bash
# List all nodes
bctl nodes list

# Get node information
bctl nodes info 1

# Attach node
bctl nodes attach 1

# Detach node
bctl nodes detach 2

# Promote node to primary
bctl nodes promote 2

# Recover node
bctl nodes recovery 3
```

### Global Options

```bash
--host HOST     # PgBalancer host (default: localhost)
--port PORT     # REST API port (default: 8080)
--user USER     # Authentication user
--password PASS # Authentication password
--verbose       # Verbose output
--help          # Show help
```

## Examples

### Check PgBalancer Status

```bash
$ bctl status
Server: pgbalancer
Status: running
Version: 1.0.0
Uptime: 3600 seconds
Connections: 25
Nodes: 3
Processes: 32
```

### Manage Backend Nodes

```bash
# List all nodes
$ bctl nodes list
[
  {
    "id": 1,
    "host": "db1.example.com",
    "port": 5432,
    "status": "up",
    "role": "primary"
  },
  {
    "id": 2,
    "host": "db2.example.com", 
    "port": 5432,
    "status": "up",
    "role": "standby"
  }
]

# Detach a failed node
$ bctl nodes detach 2
Node 2 detached successfully
```

### Configuration Management

```bash
# Reload configuration after changes
$ bctl reload
Configuration reloaded successfully

# Invalidate cache after schema changes
$ bctl cache invalidate
Query cache invalidated successfully
```

## REST API Endpoints

bctl communicates with PgBalancer via these REST endpoints:

- `GET /api/v1/status` - Server status
- `POST /api/v1/control/stop` - Stop server
- `POST /api/v1/control/reload` - Reload configuration
- `POST /api/v1/control/logrotate` - Rotate logs
- `POST /api/v1/cache/invalidate` - Invalidate cache
- `GET /api/v1/nodes` - List nodes
- `GET /api/v1/nodes/{id}` - Get node info
- `POST /api/v1/nodes/{id}/attach` - Attach node
- `POST /api/v1/nodes/{id}/detach` - Detach node
- `POST /api/v1/nodes/{id}/recovery` - Recover node
- `POST /api/v1/nodes/{id}/promote` - Promote node

## Development

### Building from Source

```bash
# Check dependencies
make check-deps

# Build
make

# Test
make test

# Clean
make clean
```

### Testing

```bash
# Basic functionality test
make test

# Full integration test (requires running pgbalancer)
make test-full
```

## Error Handling

bctl provides clear error messages and appropriate exit codes:

- `0` - Success
- `1` - General error
- `2` - Connection error
- `3` - Authentication error
- `4` - Invalid command/option

## License

Same license as PgBalancer project.

## Support

For issues and questions:
- GitHub Issues: [PgBalancer Repository]
- Documentation: [PgBalancer Docs]
- Community: [PgBalancer Community]