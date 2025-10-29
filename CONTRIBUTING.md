# Contributing to pgbalancer

Thank you for your interest in contributing to pgbalancer! We welcome contributions from the community.

## How to Contribute

### Reporting Issues

If you find a bug or have a feature request:

1. Check if the issue already exists in [GitHub Issues](https://github.com/pgelephant/pgbalancer/issues)
2. If not, create a new issue with:
   - Clear description of the problem or feature
   - PostgreSQL version and OS information
   - pgbalancer version and configuration
   - Steps to reproduce (for bugs)
   - Expected vs actual behavior
   - Relevant log excerpts or error messages

### Submitting Pull Requests

1. **Fork** the repository
2. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes** following our coding standards
4. **Test thoroughly**:
   ```bash
   ./configure --with-pgsql=/usr/local/pgsql
   make
   sudo make install
   # Run tests
   cd src/test && ./run-tests.sh
   ```
5. **Commit** with clear messages:
   ```bash
   git commit -m "Add feature: description of feature"
   ```
6. **Push** to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```
7. **Open a Pull Request** with:
   - Clear description of changes
   - Why the change is needed
   - How it was tested
   - Any breaking changes or migration notes

## Coding Standards

### C Code

Follow PostgreSQL C coding conventions:

- **Variables**: Declare at the start of functions
- **Comments**: Use C-style `/* */` comments (not `//`)
- **Braces**: Allman style (opening brace on its own line for functions)
- **Indentation**: Tabs (width 4)
- **Naming**: Snake_case for functions and variables
- **Headers**: Include guards in all `.h` files
- **Warnings**: Code must compile with zero warnings
- **Memory**: Use pgbalancer memory contexts properly

Example:
```c
void
pool_my_function(int param1, char *param2)
{
    int result;
    char *buffer;
    
    /* Initialize variables */
    result = 0;
    buffer = palloc(256);
    
    /* Function logic here */
    
    pfree(buffer);
}
```

### Configuration Changes

- Update both YAML and .conf format parsers
- Add documentation in `doc/src/sgml/`
- Update sample configurations in `src/sample/`
- Test backward compatibility

### REST API Changes

- Document new endpoints in REST API reference
- Follow existing JSON response formats
- Add authentication if accessing sensitive data
- Update `bctl` CLI tool if needed

## Project Structure

```
pgbalancer/
├── src/
│   ├── main/           # Core pgbalancer logic
│   ├── protocol/       # Protocol handlers
│   ├── watchdog/       # Watchdog and failover
│   ├── rest_api/       # REST API server
│   ├── ai/             # AI load balancing
│   ├── config/         # Configuration parser
│   ├── utils/          # Utility functions
│   └── test/           # Test suite
├── bctl/               # CLI client tool
├── doc/                # SGML documentation
├── monitoring/         # Prometheus/Grafana
└── sample/             # Sample configurations
```

## Development Setup

### Prerequisites

- PostgreSQL 13+ with development headers
- GCC or Clang compiler
- Autotools (autoconf, automake, libtool)
- libyaml
- libpq
- Git

### Build from Source

```bash
# Clone repository
git clone https://github.com/pgelephant/pgbalancer.git
cd pgbalancer

# Configure
./configure \
    --with-pgsql=/usr/local/pgsql \
    --enable-rest-api \
    --with-openssl

# Build
make

# Install
sudo make install

# Verify
pgbalancer --version
bctl --version
```

### Testing

```bash
# Run integration tests
cd src/test
./run-tests.sh

# Test specific feature
./test-failover.sh
./test-load-balance.sh

# Test REST API
curl http://localhost:8080/api/v1/status
```

## Documentation

### Update Documentation

If your changes affect user-facing functionality:

1. Update `README.md`
2. Update SGML docs in `doc/src/sgml/`
3. Update REST API documentation
4. Update `bctl` help text
5. Update `CHANGELOG.md`

### Building Documentation

```bash
cd doc/src
make html
# Output in doc/src/sgml/html/
```

## Code Review Process

1. All PRs require maintainer review
2. CI/CD checks must pass
3. At least one approval required
4. No merge conflicts
5. Documentation updated as needed
6. Tests passing

## Adding Features

### New Configuration Parameters

1. Add to `src/config/pool_config_variables.c`
2. Add to `src/include/pool_config.h`
3. Update config parser in `src/config/pool_config.c`
4. Add documentation in `doc/src/sgml/config.sgml`
5. Add to sample configs in `src/sample/`
6. Write tests

### New REST API Endpoints

1. Add handler in `src/rest_api/`
2. Register endpoint in router
3. Update `bctl` if needed
4. Document in REST API reference
5. Add authentication if sensitive
6. Write tests

### New Watchdog Features

1. Implement in `src/watchdog/`
2. Add configuration parameters
3. Update watchdog documentation
4. Test failover scenarios
5. Add integration tests

## Getting Help

- **Questions**: Open a [GitHub Discussion](https://github.com/pgelephant/pgbalancer/discussions)
- **Bugs**: Create an [Issue](https://github.com/pgelephant/pgbalancer/issues)
- **Security**: Email security@pgelephant.org
- **General**: Email team@pgelephant.org

## Code of Conduct

Be respectful, professional, and collaborative. We follow the [PostgreSQL Community Code of Conduct](https://www.postgresql.org/about/policies/coc/).

## License

By contributing, you agree that your contributions will be licensed under the same terms as pgbalancer (PostgreSQL License). See [COPYING](COPYING) for details.

---

Thank you for contributing to pgbalancer! 🙏

