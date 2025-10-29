%global pg_version %{?pg_version}%{!?pg_version:17}
%global package_version %{?package_version}%{!?package_version:1.0.0}
%global pginstdir /usr/pgsql-%{pg_version}

Name:           pgbalancer_%{pg_version}
Version:        %{package_version}
Release:        1%{?dist}
Summary:        Modern PostgreSQL connection pooler with REST API and YAML configuration
License:        PostgreSQL
URL:            https://github.com/pgelephant/pgbalancer
Source0:        pgbalancer-%{version}.tar.gz

BuildRequires:  postgresql%{pg_version}-server
BuildRequires:  postgresql%{pg_version}-devel
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  libyaml-devel
BuildRequires:  openssl-devel
BuildRequires:  pam-devel

Requires:       postgresql%{pg_version}-server
Requires:       libyaml
Requires:       openssl-libs

%description
pgbalancer is a modern, production-ready PostgreSQL connection pooler and load
balancer built as a fork of pgpool-II. It provides a comprehensive REST API,
professional CLI tool (bctl), and YAML configuration support.

Features:
- Modern REST API with 17 endpoints for complete cluster control
- Unified CLI tool (bctl) replacing fragmented pcp_* commands
- YAML configuration with validation
- Optional pgraft consensus integration for leader election
- AI-based load balancing algorithms
- JWT authentication for API security
- Connection pooling and load balancing
- Automatic failover and recovery
- Health monitoring and performance statistics
- Compatible with PostgreSQL 13, 14, 15, 16, 17, 18

%prep
%setup -q -n pgbalancer-%{version}

%build
export PG_CONFIG=%{pginstdir}/bin/pg_config
./configure \
    --prefix=%{pginstdir} \
    --with-pgsql=%{pginstdir} \
    --with-openssl \
    --with-pam \
    --enable-rest-api

make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

# Create directories
mkdir -p %{buildroot}/etc/pgbalancer
mkdir -p %{buildroot}/var/log/pgbalancer
mkdir -p %{buildroot}/var/run/pgbalancer

# Install sample config
install -m 644 src/sample/pgpool.conf.sample %{buildroot}/etc/pgbalancer/pgbalancer.conf.sample

%files
%license COPYING
%doc README.md CHANGELOG.md AUTHORS NEWS
%{pginstdir}/bin/pgbalancer
%{pginstdir}/bin/bctl
%{pginstdir}/bin/pcp_*
%{pginstdir}/bin/pg_enc
%{pginstdir}/bin/pg_md5
%{pginstdir}/bin/pgpool_setup
%{pginstdir}/etc/pgbalancer.conf.sample*
%{pginstdir}/share/pgbalancer/
%dir /etc/pgbalancer
%config(noreplace) /etc/pgbalancer/pgbalancer.conf.sample
%dir /var/log/pgbalancer
%dir /var/run/pgbalancer

%post
# Create pgbalancer user if doesn't exist
if ! id -u pgbalancer >/dev/null 2>&1; then
    useradd -r -d /var/lib/pgbalancer -s /bin/bash pgbalancer
fi
# Set permissions
chown -R pgbalancer:pgbalancer /var/log/pgbalancer /var/run/pgbalancer

%changelog
* Sun Oct 27 2024 pgElephant Team <team@pgelephant.org> - 1.0.0-1
- Initial pgbalancer release (fork of pgpool-II)
- PostgreSQL 13, 14, 15, 16, 17, 18 support
- REST API with JWT authentication
- Unified bctl CLI tool
- YAML configuration support
- pgraft consensus integration
- AI-based load balancing
- Enhanced monitoring and observability

