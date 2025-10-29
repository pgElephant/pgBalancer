# pgbalancer Prometheus Metrics Integration

## Overview

pgbalancer provides comprehensive Prometheus metrics for monitoring connection pooling,
backend health, load balancing, and system performance.

## Metrics Categories

### 1. Server Status Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `pgbalancer_up` | Gauge | Server status (1=up, 0=down) |
| `pgbalancer_version_info` | Gauge | Version information |
| `pgbalancer_start_time_seconds` | Gauge | Server start timestamp |
| `pgbalancer_uptime_seconds` | Gauge | Server uptime in seconds |

### 2. Backend Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `pgbalancer_backend_total` | Gauge | - | Total configured backends |
| `pgbalancer_backend_up` | Gauge | node_id, hostname | Backend status (1=up, 0=down) |
| `pgbalancer_backend_role` | Gauge | node_id | Role (1=primary, 0=standby) |
| `pgbalancer_backend_queries_total` | Counter | node_id | Total queries sent to backend |
| `pgbalancer_backend_connections` | Gauge | node_id | Active connections to backend |
| `pgbalancer_backend_weight` | Gauge | node_id | Load balancing weight |
| `pgbalancer_backend_replication_lag_seconds` | Gauge | node_id | Replication lag in seconds |

### 3. Connection Pool Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `pgbalancer_pool_connections_total` | Gauge | Total pool capacity |
| `pgbalancer_pool_connections_active` | Gauge | Active connections in use |
| `pgbalancer_pool_connections_idle` | Gauge | Idle connections in pool |
| `pgbalancer_pool_utilization_percent` | Gauge | Pool utilization (0-100) |
| `pgbalancer_pool_hits_total` | Counter | Pool cache hits |
| `pgbalancer_pool_misses_total` | Counter | Pool cache misses |

### 4. Process Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `pgbalancer_process_total` | Gauge | Total child processes |
| `pgbalancer_process_active` | Gauge | Active child processes |
| `pgbalancer_process_idle` | Gauge | Idle child processes |

### 5. Query Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `pgbalancer_queries_total` | Counter | type | Total queries by type (SELECT/INSERT/UPDATE/DELETE) |
| `pgbalancer_transactions_total` | Counter | status | Transactions (committed/aborted) |
| `pgbalancer_query_duration_seconds` | Histogram | - | Query duration distribution |

### 6. Health Check Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `pgbalancer_health_check_total` | Counter | node_id, result | Health checks performed |
| `pgbalancer_health_check_failures_total` | Counter | node_id | Failed health checks |
| `pgbalancer_health_check_duration_seconds` | Histogram | node_id | Health check duration |

### 7. Watchdog Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `pgbalancer_watchdog_state` | Gauge | Watchdog state (0-5) |
| `pgbalancer_watchdog_quorum` | Gauge | Quorum status (1=quorum, 0=no) |
| `pgbalancer_watchdog_alive_nodes` | Gauge | Number of alive watchdog nodes |
| `pgbalancer_watchdog_total_nodes` | Gauge | Total watchdog nodes |

### 8. Failover Metrics

| Metric | Type | Labels | Description |
|--------|------|--------|-------------|
| `pgbalancer_failover_total` | Counter | reason | Total failover events |
| `pgbalancer_failback_total` | Counter | - | Total failback events |
| `pgbalancer_backend_detach_total` | Counter | node_id | Backend detach events |
| `pgbalancer_backend_attach_total` | Counter | node_id | Backend attach events |

## Setup Methods

### Method 1: Prometheus Textfile Collector (Bare Metal)

```bash
# Run exporter script periodically
*/1 * * * * /usr/local/bin/pgbalancer-exporter.sh > /var/lib/node_exporter/textfile_collector/pgbalancer.prom

# Configure node_exporter
node_exporter --collector.textfile.directory=/var/lib/node_exporter/textfile_collector
```

### Method 2: ServiceMonitor (Kubernetes + Prometheus Operator)

```bash
# Apply ServiceMonitor
kubectl apply -f monitoring/prometheus/servicemonitor.yaml

# Prometheus will auto-discover pgbalancer pods
```

### Method 3: Direct Scrape Configuration

```yaml
# Add to prometheus.yml
scrape_configs:
  - job_name: 'pgbalancer'
    static_configs:
      - targets:
          - 'pgbalancer-host:8080'
    metrics_path: '/metrics'
```

## Installation

### 1. Deploy Exporter Script

```bash
# Copy exporter
sudo cp monitoring/prometheus/pgbalancer-exporter.sh /usr/local/bin/
sudo chmod +x /usr/local/bin/pgbalancer-exporter.sh

# Test
/usr/local/bin/pgbalancer-exporter.sh
```

### 2. Configure Prometheus

```bash
# Copy configuration
sudo cp monitoring/prometheus/prometheus.yml /etc/prometheus/

# Copy alert rules
sudo cp monitoring/prometheus/pgbalancer-alerts.yml /etc/prometheus/rules/

# Reload Prometheus
curl -X POST http://localhost:9090/-/reload
```

### 3. Import Grafana Dashboard

```bash
# Upload dashboard
curl -X POST http://admin:admin@localhost:3000/api/dashboards/db \
  -H "Content-Type: application/json" \
  -d @monitoring/grafana/pgbalancer-dashboard.json
```

## Kubernetes Deployment

### With Prometheus Operator

```bash
# Deploy ServiceMonitor
kubectl apply -f monitoring/prometheus/servicemonitor.yaml

# Prometheus will automatically discover and scrape pgbalancer pods
```

### Manual Configuration

```yaml
# Add to Prometheus ConfigMap
scrape_configs:
  - job_name: 'pgbalancer-k8s'
    kubernetes_sd_configs:
      - role: pod
    relabel_configs:
      - source_labels: [__meta_kubernetes_pod_label_app]
        action: keep
        regex: pgbalancer
      - source_labels: [__meta_kubernetes_pod_container_port_number]
        action: keep
        regex: '8080'
```

## Example Queries

### Backend Health

```promql
# Backends currently up
sum(pgbalancer_backend_up)

# Backend uptime percentage (24h)
avg_over_time(pgbalancer_backend_up[24h]) * 100

# Backends down
pgbalancer_backend_up == 0
```

### Performance

```promql
# Queries per second
rate(pgbalancer_backend_queries_total[5m])

# Query distribution by backend
sum(rate(pgbalancer_backend_queries_total[5m])) by (node_id)

# Pool utilization
(pgbalancer_pool_connections_active / pgbalancer_pool_connections_total) * 100
```

### Health Checks

```promql
# Health check failure rate
rate(pgbalancer_health_check_failures_total[5m])

# Health check success rate
rate(pgbalancer_health_check_total{result="success"}[5m])

# Average health check duration
avg(pgbalancer_health_check_duration_seconds)
```

### Replication

```promql
# Replication lag
pgbalancer_backend_replication_lag_seconds

# Max replication lag
max(pgbalancer_backend_replication_lag_seconds)

# Lag alert
pgbalancer_backend_replication_lag_seconds > 60
```

### Failover

```promql
# Failover events (24h)
increase(pgbalancer_failover_total[24h])

# Recent failovers (1h)
increase(pgbalancer_failover_total[1h])
```

## Alerting

Pre-configured alerts in `pgbalancer-alerts.yml`:

- ✅ **PgbalancerDown** - Server unavailable
- ✅ **PgbalancerBackendDown** - Backend server down
- ✅ **PgbalancerPrimaryDown** - Primary backend down (critical)
- ✅ **PgbalancerAllBackendsDown** - Complete outage
- ✅ **PgbalancerHighConnectionPoolUtilization** - Pool >90%
- ✅ **PgbalancerConnectionPoolExhaustion** - Pool at 100%
- ✅ **PgbalancerUnbalancedLoad** - Skewed load distribution
- ✅ **PgbalancerHealthCheckFailures** - Frequent health check failures
- ✅ **PgbalancerHighReplicationLag** - Lag >60s
- ✅ **PgbalancerCriticalReplicationLag** - Lag >5 minutes
- ✅ **PgbalancerWatchdogQuorumLost** - Lost quorum
- ✅ **PgbalancerFailoverEvent** - Failover occurred
- ✅ **PgbalancerFrequentFailovers** - >3 failovers in 1 hour

## Grafana Dashboard

Import `monitoring/grafana/pgbalancer-dashboard.json` for:

- ✅ Server status overview
- ✅ Backend node status
- ✅ Connection pool utilization
- ✅ Query rate (QPS)
- ✅ Load distribution graphs
- ✅ Replication lag timeline
- ✅ Backend status timeline
- ✅ Failover event tracking
- ✅ Watchdog status
- ✅ Process statistics

## Metric Collection Flow

```
pgbalancer → bctl REST API → pgbalancer-exporter.sh → Prometheus → Grafana
             (port 8080)      (textfile or direct)
```

## Best Practices

1. **Scrape Interval**: 15-30 seconds (balance freshness vs load)
2. **Retention**: 15-30 days minimum for trend analysis
3. **Alerts**: Configure alertmanager for notifications
4. **Dashboards**: Use pre-built Grafana dashboard
5. **Labels**: Use pod, namespace, node labels in Kubernetes
6. **High Cardinality**: Avoid per-connection metrics
7. **Aggregation**: Use recording rules for expensive queries

## Testing

```bash
# Test exporter
./monitoring/prometheus/pgbalancer-exporter.sh

# Verify metrics format
./monitoring/prometheus/pgbalancer-exporter.sh | promtool check metrics

# Query Prometheus
curl http://localhost:9090/api/v1/query?query=pgbalancer_up

# View in Prometheus UI
http://localhost:9090/graph
```

## Troubleshooting

### No Metrics Appearing

```bash
# Check exporter
./monitoring/prometheus/pgbalancer-exporter.sh

# Check bctl connectivity
bctl -H localhost -p 8080 status

# Check Prometheus targets
curl http://localhost:9090/api/v1/targets
```

### Stale Metrics

```bash
# Check scrape timestamp
pgbalancer_scrape_duration_seconds

# Verify exporter is running
ps aux | grep pgbalancer-exporter

# Check cron job
crontab -l | grep pgbalancer
```

## Integration Examples

### With Alertmanager

```yaml
# alertmanager.yml
route:
  receiver: 'pgbalancer-alerts'
  group_by: ['alertname', 'instance']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h

receivers:
- name: 'pgbalancer-alerts'
  slack_configs:
  - channel: '#database-alerts'
    title: 'pgbalancer Alert'
    text: '{{ .Annotations.description }}'
```

### With Grafana Alerting

```json
{
  "alert": {
    "name": "pgbalancer Backend Down",
    "conditions": [
      {
        "query": "pgbalancer_backend_up == 0",
        "threshold": 0
      }
    ],
    "frequency": "1m",
    "handler": 1
  }
}
```

## Summary

✅ **Complete Prometheus Integration**  
✅ **40+ Metrics Available**  
✅ **Pre-built Grafana Dashboard**  
✅ **Alert Rules Included**  
✅ **Kubernetes ServiceMonitor**  
✅ **Textfile Exporter**  
✅ **Documentation Complete**  

**pgbalancer is fully instrumented for Prometheus monitoring!** 📊

