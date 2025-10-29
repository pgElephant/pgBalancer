# Standalone pgBalancer Docker Image
# This image contains only pgBalancer and bctl (no PostgreSQL)

FROM ubuntu:22.04 AS builder

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

# Copy pgbalancer source
WORKDIR /build
COPY ../../ .

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

# Runtime stage - minimal image
FROM ubuntu:22.04

ARG PG_MAJOR=17
ARG DEBIAN_FRONTEND=noninteractive

LABEL maintainer="pgElephant Team <team@pgelephant.org>"
LABEL description="pgBalancer - PostgreSQL Load Balancer & Connection Pooler"
LABEL version="1.0.0"

# Install only runtime dependencies
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

# Create pgbalancer user
RUN groupadd -r pgbalancer && useradd -r -g pgbalancer -d /opt/pgbalancer pgbalancer \
    && mkdir -p /opt/pgbalancer \
    && mkdir -p /var/run/pgbalancer \
    && mkdir -p /var/log/pgbalancer \
    && mkdir -p /etc/pgbalancer \
    && mkdir -p /var/lib/pgbalancer \
    && chown -R pgbalancer:pgbalancer /opt/pgbalancer /var/run/pgbalancer /var/log/pgbalancer /etc/pgbalancer /var/lib/pgbalancer

# Copy built artifacts
COPY --from=builder /opt/pgbalancer /opt/pgbalancer

# Environment
ENV PATH="/opt/pgbalancer/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/pgbalancer/lib:${LD_LIBRARY_PATH}"

# Copy configuration and scripts
COPY config/ /etc/pgbalancer/
COPY scripts/ /usr/local/bin/
RUN chmod +x /usr/local/bin/*.sh

# Expose ports
EXPOSE 9999 9898 8080 9000 9694

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD bctl --host localhost --port 8080 status || exit 1

WORKDIR /opt/pgbalancer
USER pgbalancer

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["pgbalancer", "-n", "-f", "/etc/pgbalancer/pgbalancer.conf"]

