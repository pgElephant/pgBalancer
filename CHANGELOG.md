# Changelog

All notable changes to pgbalancer will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-10-27

### Added
- Initial release of pgbalancer (modern fork of pgpool-II)
- Support for PostgreSQL 13, 14, 15, 16, 17, 18
- **REST API Management**
  - Production HTTP/JSON REST API integrated as child process (port 8080)
  - 17 endpoints for complete pgbalancer control
  - JWT authentication with HMAC-SHA256
  - <10ms response time
  - Real-time backend data from shared memory
- **Unified CLI Tool (bctl)**
  - Single command replacing multiple pcp_* tools
  - Box-drawing table format
  - JSON output mode
  - Remote connection support
  - Verbose debugging mode
- **YAML Configuration**
  - Modern YAML format with libyaml integration
  - Validation and error checking
  - Backward compatible with .conf format
- **pgraft Consensus Integration**
  - Optional Raft-based leader election
  - Consensus mode: heuristic (default) or pgraft
  - Automatic leader detection and failover
  - Integration with pgraft extension
- **AI-Based Load Balancing**
  - Smart query distribution algorithm
  - Adaptive backend selection
  - Performance-based routing
- **Enhanced Watchdog**
  - Improved failover detection
  - Better split-brain prevention
  - Configurable health checks
- **Connection Pooling**
  - Session pooling
  - Transaction pooling
  - Statement pooling
  - Automatic connection cleanup
- **Monitoring & Observability**
  - Prometheus metrics export
  - Grafana dashboard templates
  - Health check endpoints
  - Performance statistics

### Changed
- Modernized configuration system with YAML support
- Streamlined API from binary PCP to HTTP/JSON REST
- Unified CLI interface replacing fragmented pcp_* commands
- Enhanced watchdog with pgraft consensus option

### Technical Details
- Based on pgpool-II with extensive enhancements
- C codebase following PostgreSQL coding conventions
- Autotools build system (./configure && make)
- Comprehensive test suite
- Production-ready stability and performance

## [Unreleased]

### Planned
- Additional load balancing algorithms
- Enhanced monitoring dashboards
- Kubernetes operator
- Docker Compose examples
- Additional consensus backends

---

For detailed version history and development updates, see the [commit log](https://github.com/pgelephant/pgbalancer/commits/main).

