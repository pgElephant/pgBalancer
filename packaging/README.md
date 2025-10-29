# pgbalancer Packaging

This directory contains packaging files for building RPM and DEB packages of pgbalancer for various PostgreSQL versions.

## Supported Versions

- PostgreSQL 13, 14, 15, 16, 17, 18
- Rocky Linux / AlmaLinux / RHEL 9 (RPM)
- Ubuntu 22.04+ / Debian (DEB)

## Package Structure

```
packaging/
├── rpm/
│   └── pgbalancer.spec           # RPM spec file
└── debian/
    ├── control                    # Package metadata
    ├── rules                      # Build rules
    ├── changelog                  # Changelog
    └── compat                     # Debhelper compatibility level
```

## GitHub Actions Workflow

The packages are built automatically using GitHub Actions workflows:

- `.github/workflows/build-packages.yml` - Main workflow (manual trigger)
- `.github/workflows/reusable-build-rpm.yml` - RPM build workflow
- `.github/workflows/reusable-build-deb.yml` - DEB build workflow

### Manual Trigger

1. Go to: `https://github.com/pgelephant/pgbalancer/actions`
2. Select "Build Packages" workflow
3. Click "Run workflow"
4. Configure options:
   - **pg_versions**: Comma-separated list (e.g., `13,14,15,16,17,18`)
   - **create_release**: Check to create GitHub release
   - **release_tag**: Release tag name (e.g., `REL_1_0`, `v1.0.0`)
5. Click "Run workflow"

### Workflow Features

- ✅ Matrix build across all PostgreSQL versions
- ✅ Builds both RPM and DEB packages
- ✅ Automatic package testing
- ✅ Optional GitHub release creation
- ✅ SHA256SUMS generation
- ✅ Artifact retention (30 days)

## Package Naming

### RPM Packages
```
pgbalancer_<pg_version>-<version>-<release>.el9.<arch>.rpm
```
Example: `pgbalancer_17-1.0.0-1.el9.x86_64.rpm`

### DEB Packages
```
postgresql-<pg_version>-pgbalancer_<version>-<release>_<arch>.deb
```
Example: `postgresql-17-pgbalancer_1.0.0-1_amd64.deb`

## Building Locally

### Build RPM

```bash
# Install dependencies
sudo dnf install rpm-build rpmdevtools postgresql17-devel libyaml-devel

# Create source tarball
VERSION=1.0.0
tar czf ~/rpmbuild/SOURCES/pgbalancer-${VERSION}.tar.gz \
  --transform "s,^,pgbalancer-${VERSION}/," \
  src/ doc/ bctl/ monitoring/ configure Makefile.am \
  AUTHORS COPYING README.md CHANGELOG.md

# Build RPM
rpmbuild -ba packaging/rpm/pgbalancer.spec \
  --define "pg_version 17" \
  --define "package_version ${VERSION}"
```

### Build DEB

```bash
# Install dependencies
sudo apt install build-essential debhelper autoconf automake libtool \
  postgresql-server-dev-17 libyaml-dev libssl-dev

# Prepare build directory
VERSION=1.0.0
BUILD_DIR=pgbalancer-postgresql-17-${VERSION}
mkdir -p ${BUILD_DIR}
cp -r src/ doc/ bctl/ configure Makefile.am AUTHORS COPYING README.md ${BUILD_DIR}/
cp -r packaging/debian ${BUILD_DIR}/
cd ${BUILD_DIR}

# Update PostgreSQL version
sed -i "s/@PG_VERSION@/17/g" debian/control

# Build package
dpkg-buildpackage -us -uc -b
```

## Installation

### RPM Installation

```bash
# Install package
sudo dnf install pgbalancer_17-1.0.0-1.el9.x86_64.rpm

# Verify installation
rpm -ql pgbalancer_17
pgbalancer --version
bctl --version
```

### DEB Installation

```bash
# Install package
sudo apt install ./postgresql-17-pgbalancer_1.0.0-1_amd64.deb

# Verify installation
dpkg -L postgresql-17-pgbalancer
pgbalancer --version
bctl --version
```

## Post-Installation

### Basic Configuration

1. Create configuration file:
   ```bash
   sudo cp /etc/pgbalancer/pgbalancer.conf.sample /etc/pgbalancer/pgbalancer.yml
   sudo vi /etc/pgbalancer/pgbalancer.yml
   ```

2. Configure backends:
   ```yaml
   backend_hostname0: 'localhost'
   backend_port0: 5432
   backend_weight0: 1
   ```

3. Start pgbalancer:
   ```bash
   pgbalancer -f /etc/pgbalancer/pgbalancer.yml
   ```

4. Check status:
   ```bash
   bctl status
   ```

### REST API Setup

Enable REST API in configuration:

```yaml
rest_api:
  enabled: true
  port: 8080
  bind_address: '127.0.0.1'
  jwt_enabled: true
  jwt_secret: 'your-256-bit-secret-change-this'
```

Test API:
```bash
curl http://localhost:8080/api/v1/status
```

## Package Contents

### Files Installed

**RPM:**
- Binaries: `/usr/pgsql-<ver>/bin/pgbalancer`, `/usr/pgsql-<ver>/bin/bctl`
- Config: `/etc/pgbalancer/`
- Logs: `/var/log/pgbalancer/`
- Runtime: `/var/run/pgbalancer/`

**DEB:**
- Binaries: `/usr/bin/pgbalancer`, `/usr/bin/bctl`
- Config: `/etc/pgbalancer/`
- Logs: `/var/log/pgbalancer/`
- Runtime: `/var/run/pgbalancer/`

## Dependencies

### Build Dependencies
- PostgreSQL development headers
- libyaml-devel (RPM) / libyaml-dev (DEB)
- openssl-devel (RPM) / libssl-dev (DEB)
- pam-devel (RPM) / libpam0g-dev (DEB)
- autoconf, automake, libtool
- gcc, make

### Runtime Dependencies
- PostgreSQL server (matching version)
- libyaml
- OpenSSL libraries
- PAM libraries

## Troubleshooting

### Build Failures

1. **PostgreSQL version not found**:
   - Ensure PostgreSQL repository is configured
   - Check: `dnf list postgresql*-devel` or `apt-cache search postgresql-server-dev`

2. **libyaml not found**:
   - RPM: `sudo dnf install libyaml-devel`
   - DEB: `sudo apt install libyaml-dev`

3. **Configure fails**:
   - Check autoconf/automake versions
   - Run `autoreconf -i` to regenerate configure script

### Installation Failures

1. **Dependency errors**:
   - Install PostgreSQL server first
   - Ensure libyaml is available

2. **Permission errors**:
   - Run installation as root
   - Check /var/log/pgbalancer and /var/run/pgbalancer permissions

3. **Binary not found**:
   - Check PATH includes PostgreSQL bin directory
   - RPM: `/usr/pgsql-<ver>/bin`
   - DEB: `/usr/bin`

## Testing Packages

### Basic Functionality Test

```bash
# Start pgbalancer
pgbalancer -f /etc/pgbalancer/pgbalancer.yml -d

# Check status
bctl status

# Test connection
psql -h localhost -p 9999 -U postgres -c "SELECT version();"

# Check REST API
curl http://localhost:8080/api/v1/backends

# Stop
bctl stop
```

## Support

For issues, questions, or contributions:
- GitHub Issues: https://github.com/pgelephant/pgbalancer/issues
- Documentation: https://pgelephant.github.io/pgbalancer/
- Email: team@pgelephant.org

