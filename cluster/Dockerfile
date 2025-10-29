# pgBalancer - PostgreSQL Load Balancer & Connection Pooler
# Multi-stage build for optimal image size

FROM ubuntu:22.04 AS builder

# Build arguments
ARG PG_MAJOR=17
ARG DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    autoconf \
    automake \
    libtool \
    pkg-config \
    postgresql-server-dev-${PG_MAJOR} \
    libpq-dev \
    libssl-dev \
    libyaml-dev \
    libcurl4-openssl-dev \
    libjson-c-dev \
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
WORKDIR /build
COPY ../ .

# Build pgbalancer
RUN ./configure \
    --prefix=/opt/pgbalancer \
    --with-openssl \
    --enable-rest-api \
    && make -j$(nproc) \
    && make install

# Build bctl
WORKDIR /build/bctl
RUN make clean && make && make install DESTDIR=/opt/pgbalancer

# Runtime stage
FROM ubuntu:22.04

# Runtime arguments
ARG PG_MAJOR=17
ARG DEBIAN_FRONTEND=noninteractive

LABEL maintainer="pgElephant Team <team@pgelephant.org>"
LABEL description="pgBalancer - PostgreSQL Load Balancer & Connection Pooler with bctl management"
LABEL version="1.0.0"

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libssl3 \
    libyaml-0-2 \
    libcurl4 \
    libjson-c5 \
    libpq5 \
    postgresql-client-${PG_MAJOR} \
    ca-certificates \
    procps \
    curl \
    iputils-ping \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Create pgbalancer user and directories
RUN groupadd -r pgbalancer && useradd -r -g pgbalancer -d /opt/pgbalancer pgbalancer \
    && mkdir -p /opt/pgbalancer \
    && mkdir -p /var/run/pgbalancer \
    && mkdir -p /var/log/pgbalancer \
    && mkdir -p /etc/pgbalancer \
    && mkdir -p /var/lib/pgbalancer \
    && chown -R pgbalancer:pgbalancer /opt/pgbalancer /var/run/pgbalancer /var/log/pgbalancer /etc/pgbalancer /var/lib/pgbalancer

# Copy built artifacts from builder
COPY --from=builder /opt/pgbalancer /opt/pgbalancer

# Add to PATH
ENV PATH="/opt/pgbalancer/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/pgbalancer/lib:${LD_LIBRARY_PATH}"

# Copy entrypoint and helper scripts
COPY scripts/docker-entrypoint.sh /usr/local/bin/
COPY scripts/healthcheck.sh /usr/local/bin/
COPY scripts/bctl-wrapper.sh /usr/local/bin/

# Make scripts executable
RUN chmod +x /usr/local/bin/*.sh

# Expose ports
# 9999: pgbalancer main port
# 9898: PCP (PgPool Control Protocol) port
# 8080: REST API port
# 9000: Watchdog port (primary)
# 9001: Watchdog port (standby1)
# 9002: Watchdog port (standby2)
# 9694: Watchdog heartbeat port
EXPOSE 9999 9898 8080 9000 9001 9002 9694

# Health check using bctl
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD /usr/local/bin/healthcheck.sh || exit 1

# Set working directory
WORKDIR /opt/pgbalancer

# Switch to pgbalancer user
USER pgbalancer

# Entrypoint and command
ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["pgbalancer", "-n", "-f", "/etc/pgbalancer/pgbalancer.conf"]
