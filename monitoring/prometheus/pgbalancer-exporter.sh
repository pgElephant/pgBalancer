#!/bin/bash
#
# pgbalancer Prometheus Exporter
# Exports pgbalancer metrics in Prometheus text format
#
# Usage: ./pgbalancer-exporter.sh > /var/lib/node_exporter/textfile_collector/pgbalancer.prom
#

set -euo pipefail

BCTL="${BCTL:-bctl}"
HOST="${PGBALANCER_HOST:-localhost}"
PORT="${PGBALANCER_REST_PORT:-8080}"

METRIC_PREFIX="pgbalancer"

echo "# HELP ${METRIC_PREFIX}_up pgbalancer server status (1=up, 0=down)"
echo "# TYPE ${METRIC_PREFIX}_up gauge"
if $BCTL -H "$HOST" -p "$PORT" status &>/dev/null; then
    echo "${METRIC_PREFIX}_up 1"
else
    echo "${METRIC_PREFIX}_up 0"
    exit 0
fi

echo ""
echo "# HELP ${METRIC_PREFIX}_version_info pgbalancer version information"
echo "# TYPE ${METRIC_PREFIX}_version_info gauge"
VERSION=$($BCTL -H "$HOST" -p "$PORT" status -j 2>/dev/null | jq -r '.version // "unknown"')
echo "${METRIC_PREFIX}_version_info{version=\"$VERSION\"} 1"

echo ""
echo "# HELP ${METRIC_PREFIX}_backend_total Total number of backend servers"
echo "# TYPE ${METRIC_PREFIX}_backend_total gauge"
BACKEND_COUNT=$($BCTL -H "$HOST" -p "$PORT" nodes-count 2>/dev/null || echo "0")
echo "${METRIC_PREFIX}_backend_total $BACKEND_COUNT"

echo ""
echo "# HELP ${METRIC_PREFIX}_backend_up Backend server up status (1=up, 0=down)"
echo "# TYPE ${METRIC_PREFIX}_backend_up gauge"
$BCTL -H "$HOST" -p "$PORT" nodes 2>/dev/null | while IFS= read -r line; do
    if [[ $line =~ Node\ ([0-9]+): ]]; then
        NODE_ID="${BASH_REMATCH[1]}"
    elif [[ $line =~ status:\ (up|down) ]]; then
        STATUS="${BASH_REMATCH[1]}"
        if [[ "$STATUS" == "up" ]]; then
            echo "${METRIC_PREFIX}_backend_up{node_id=\"$NODE_ID\"} 1"
        else
            echo "${METRIC_PREFIX}_backend_up{node_id=\"$NODE_ID\"} 0"
        fi
    fi
done

echo ""
echo "# HELP ${METRIC_PREFIX}_backend_role Backend server role (1=primary, 0=standby)"
echo "# TYPE ${METRIC_PREFIX}_backend_role gauge"
$BCTL -H "$HOST" -p "$PORT" nodes 2>/dev/null | while IFS= read -r line; do
    if [[ $line =~ Node\ ([0-9]+): ]]; then
        NODE_ID="${BASH_REMATCH[1]}"
    elif [[ $line =~ role:\ (primary|standby) ]]; then
        ROLE="${BASH_REMATCH[1]}"
        if [[ "$ROLE" == "primary" ]]; then
            echo "${METRIC_PREFIX}_backend_role{node_id=\"$NODE_ID\"} 1"
        else
            echo "${METRIC_PREFIX}_backend_role{node_id=\"$NODE_ID\"} 0"
        fi
    fi
done

echo ""
echo "# HELP ${METRIC_PREFIX}_backend_queries_total Total queries sent to backend"
echo "# TYPE ${METRIC_PREFIX}_backend_queries_total counter"
$BCTL -H "$HOST" -p "$PORT" nodes 2>/dev/null | while IFS= read -r line; do
    if [[ $line =~ Node\ ([0-9]+): ]]; then
        NODE_ID="${BASH_REMATCH[1]}"
    elif [[ $line =~ select_cnt:\ ([0-9]+) ]]; then
        COUNT="${BASH_REMATCH[1]}"
        echo "${METRIC_PREFIX}_backend_queries_total{node_id=\"$NODE_ID\"} $COUNT"
    fi
done

echo ""
echo "# HELP ${METRIC_PREFIX}_process_total Total number of child processes"
echo "# TYPE ${METRIC_PREFIX}_process_total gauge"
PROC_COUNT=$($BCTL -H "$HOST" -p "$PORT" processes-count 2>/dev/null || echo "0")
echo "${METRIC_PREFIX}_process_total $PROC_COUNT"

echo ""
echo "# HELP ${METRIC_PREFIX}_process_active Active child processes"
echo "# TYPE ${METRIC_PREFIX}_process_active gauge"
ACTIVE_PROCS=$($BCTL -H "$HOST" -p "$PORT" processes 2>/dev/null | grep -c "active" || echo "0")
echo "${METRIC_PREFIX}_process_active $ACTIVE_PROCS"

echo ""
echo "# HELP ${METRIC_PREFIX}_pool_connections_total Total pooled connections"
echo "# TYPE ${METRIC_PREFIX}_pool_connections_total gauge"
$BCTL -H "$HOST" -p "$PORT" processes 2>/dev/null | while IFS= read -r line; do
    if [[ $line =~ Pool\ Counter:\ ([0-9]+) ]]; then
        POOL_COUNT="${BASH_REMATCH[1]}"
        echo "${METRIC_PREFIX}_pool_connections_total $POOL_COUNT"
    fi
done

echo ""
echo "# HELP ${METRIC_PREFIX}_watchdog_state Watchdog state (0=down, 1=loading, 2=joining, 3=initializing, 4=leader, 5=standby)"
echo "# TYPE ${METRIC_PREFIX}_watchdog_state gauge"
WD_STATE=$($BCTL -H "$HOST" -p "$PORT" watchdog-status 2>/dev/null | jq -r '.state // "down"')
case "$WD_STATE" in
    "LEADER") echo "${METRIC_PREFIX}_watchdog_state 4" ;;
    "STANDBY") echo "${METRIC_PREFIX}_watchdog_state 5" ;;
    "INITIALIZING") echo "${METRIC_PREFIX}_watchdog_state 3" ;;
    "JOINING") echo "${METRIC_PREFIX}_watchdog_state 2" ;;
    "LOADING") echo "${METRIC_PREFIX}_watchdog_state 1" ;;
    *) echo "${METRIC_PREFIX}_watchdog_state 0" ;;
esac

echo ""
echo "# HELP ${METRIC_PREFIX}_watchdog_quorum Watchdog quorum status (1=quorum, 0=no quorum)"
echo "# TYPE ${METRIC_PREFIX}_watchdog_quorum gauge"
WD_QUORUM=$($BCTL -H "$HOST" -p "$PORT" watchdog-status 2>/dev/null | jq -r '.quorum // false')
if [[ "$WD_QUORUM" == "true" ]]; then
    echo "${METRIC_PREFIX}_watchdog_quorum 1"
else
    echo "${METRIC_PREFIX}_watchdog_quorum 0"
fi

echo ""
echo "# HELP ${METRIC_PREFIX}_scrape_duration_seconds Time taken to scrape metrics"
echo "# TYPE ${METRIC_PREFIX}_scrape_duration_seconds gauge"
echo "${METRIC_PREFIX}_scrape_duration_seconds $SECONDS"

