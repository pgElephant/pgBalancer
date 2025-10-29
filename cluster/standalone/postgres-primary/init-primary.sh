#!/bin/bash
# PostgreSQL Primary Node Initialization

set -e

echo "Configuring PostgreSQL Primary for Replication..."

# Apply custom configuration
if [ -f /etc/postgresql/postgresql-primary.conf ]; then
    cat /etc/postgresql/postgresql-primary.conf >> "$PGDATA/postgresql.conf"
fi

# Apply custom pg_hba
if [ -f /etc/postgresql/pg_hba.conf ]; then
    cat /etc/postgresql/pg_hba.conf >> "$PGDATA/pg_hba.conf"
fi

# Create replication user
psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" <<-EOSQL
    -- Create replication user
    CREATE ROLE replicator WITH REPLICATION LOGIN PASSWORD 'replicator';
    
    -- Create sample schema
    CREATE TABLE IF NOT EXISTS users (
        id SERIAL PRIMARY KEY,
        username VARCHAR(100) NOT NULL UNIQUE,
        email VARCHAR(255) NOT NULL,
        created_at TIMESTAMP DEFAULT NOW()
    );
    
    CREATE TABLE IF NOT EXISTS orders (
        id SERIAL PRIMARY KEY,
        user_id INTEGER REFERENCES users(id),
        product_name VARCHAR(255) NOT NULL,
        quantity INTEGER NOT NULL,
        total_amount DECIMAL(10,2) NOT NULL,
        created_at TIMESTAMP DEFAULT NOW()
    );
    
    -- Insert sample data
    INSERT INTO users (username, email) VALUES
        ('alice', 'alice@example.com'),
        ('bob', 'bob@example.com'),
        ('charlie', 'charlie@example.com')
    ON CONFLICT DO NOTHING;
    
    INSERT INTO orders (user_id, product_name, quantity, total_amount) VALUES
        (1, 'Laptop', 1, 999.99),
        (2, 'Mouse', 2, 29.98),
        (3, 'Keyboard', 1, 79.99)
    ON CONFLICT DO NOTHING;
    
    -- Create indexes
    CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
    CREATE INDEX IF NOT EXISTS idx_orders_user_id ON orders(user_id);
    
    -- Grant permissions
    GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO postgres;
    GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO postgres;
EOSQL

echo "✅ Primary node configured successfully"

