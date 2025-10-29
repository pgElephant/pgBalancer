# Standalone PostgreSQL Primary Node
# Configured for streaming replication

FROM postgres:17

LABEL maintainer="pgElephant Team <team@pgelephant.org>"
LABEL description="PostgreSQL 17 Primary Node for pgBalancer"

# Copy initialization scripts
COPY init-primary.sh /docker-entrypoint-initdb.d/
COPY postgresql-primary.conf /etc/postgresql/
COPY pg_hba.conf /etc/postgresql/

# Make scripts executable
RUN chmod +x /docker-entrypoint-initdb.d/*.sh

# Environment defaults
ENV POSTGRES_USER=postgres
ENV POSTGRES_PASSWORD=postgres
ENV POSTGRES_DB=testdb
ENV PGDATA=/var/lib/postgresql/data/pgdata

# Expose PostgreSQL port
EXPOSE 5432

# Health check
HEALTHCHECK --interval=10s --timeout=5s --retries=5 --start-period=30s \
    CMD pg_isready -U postgres || exit 1

