/*-------------------------------------------------------------------------
 *
 * bctl_mqtt.c
 *      MQTT event monitoring for bctl
 *
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* MQTT configuration */
#define MQTT_BROKER "localhost"
#define MQTT_PORT 1883

/*
 * Show MQTT topics published by pgbalancer
 */
void bctl_show_mqtt_topics(void) {
    printf("pgbalancer MQTT Event Topics\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    printf("Node Events:\n");
    printf("  pgbalancer/nodes/{id}/status    - Node status changes (up/down)\n");
    printf("  pgbalancer/nodes/{id}/health    - Health check results\n");
    printf("  pgbalancer/nodes/{id}/events    - Node operations (attach/detach/promote)\n");
    printf("\n");
    
    printf("Cluster Events:\n");
    printf("  pgbalancer/events/failover      - Failover events (old_primary, new_primary)\n");
    printf("  pgbalancer/events/config_reload - Configuration reload events\n");
    printf("\n");
    
    printf("Statistics (periodic):\n");
    printf("  pgbalancer/stats/connections    - Connection pool statistics\n");
    printf("  pgbalancer/stats/queries        - Query rate and latency\n");
    printf("\n");
    
    printf("Watchdog:\n");
    printf("  pgbalancer/watchdog/status      - Watchdog status changes\n");
    printf("  pgbalancer/watchdog/quorum      - Quorum status\n");
    printf("\n");
    
    printf("Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    printf("\n");
    
    printf("Subscribe with mosquitto_sub:\n");
    printf("  mosquitto_sub -h %s -p %d -t 'pgbalancer/#' -v\n", MQTT_BROKER, MQTT_PORT);
    printf("  mosquitto_sub -h %s -p %d -t 'pgbalancer/nodes/+/status' -v\n", MQTT_BROKER, MQTT_PORT);
    printf("  mosquitto_sub -h %s -p %d -t 'pgbalancer/events/#' -v\n", MQTT_BROKER, MQTT_PORT);
}

/*
 * Show example MQTT integration
 */
void bctl_show_mqtt_examples(void) {
    printf("MQTT Integration Examples\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    printf("1. Monitor all pgbalancer events:\n");
    printf("   mosquitto_sub -h localhost -t 'pgbalancer/#' -v\n\n");
    
    printf("2. Monitor node status changes only:\n");
    printf("   mosquitto_sub -h localhost -t 'pgbalancer/nodes/+/status'\n\n");
    
    printf("3. Monitor failover events:\n");
    printf("   mosquitto_sub -h localhost -t 'pgbalancer/events/failover' \\\n");
    printf("     | jq '.new_primary'\n\n");
    
    printf("4. Grafana/Prometheus integration:\n");
    printf("   • Use MQTT exporter to convert MQTT → Prometheus metrics\n");
    printf("   • Subscribe to pgbalancer/stats/* topics\n");
    printf("   • Visualize in Grafana dashboards\n\n");
    
    printf("5. Alerting integration (PagerDuty/Slack):\n");
    printf("   • Subscribe to pgbalancer/events/failover\n");
    printf("   • Trigger alerts on critical events\n");
    printf("   • Node.js/Python MQTT clients\n\n");
    
    printf("6. Real-time dashboard:\n");
    printf("   • WebSocket → MQTT bridge\n");
    printf("   • Live updates in web UI\n");
    printf("   • No polling needed\n");
}

