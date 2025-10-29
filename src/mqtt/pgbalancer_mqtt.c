/*-------------------------------------------------------------------------
 *
 * pgbalancer_mqtt.c
 *      MQTT event publisher for pgbalancer
 *
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */

#include "pgbalancer_mqtt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/* MQTT state */
static int mqtt_enabled = MQTT_ENABLED;
static char mqtt_broker[256] = "localhost";
static int mqtt_port = 1883;
static char mqtt_client_id[64] = "pgbalancer";
static int mqtt_socket = -1;

/*
 * Initialize MQTT client
 */
int pgbalancer_mqtt_init(const char *broker_address, int broker_port, const char *client_id) {
    if (!mqtt_enabled) {
        fprintf(stderr, "[MQTT] MQTT publishing disabled (MQTT_ENABLED = 0)\n");
        return 0;
    }
    
    strncpy(mqtt_broker, broker_address, sizeof(mqtt_broker) - 1);
    mqtt_port = broker_port;
    strncpy(mqtt_client_id, client_id, sizeof(mqtt_client_id) - 1);
    
    fprintf(stderr, "[MQTT] Initialized: broker=%s:%d, client_id=%s\n", 
            mqtt_broker, mqtt_port, mqtt_client_id);
    
    return 0;
}

/*
 * Simple MQTT publish (logs to stderr for now, can integrate with real MQTT library)
 */
int pgbalancer_mqtt_publish(const char *topic, const char *message) {
    if (!mqtt_enabled) return 0;
    
    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(stderr, "[MQTT] %s | Topic: %s | Message: %s\n", 
            timestamp, topic, message);
    
    /* TODO: Integrate with real MQTT library (Paho MQTT C, mosquitto, etc.) */
    /* For now, we log events. To enable real MQTT:
     * 1. Link with -lpaho-mqtt3c or -lmosquitto
     * 2. Replace fprintf with MQTTClient_publish() or mosquitto_publish()
     */
    
    return 0;
}

/*
 * Publish node status change event
 */
void pgbalancer_mqtt_publish_node_status(int node_id, const char *status) {
    char topic[128];
    char message[512];
    
    snprintf(topic, sizeof(topic), "pgbalancer/nodes/%d/status", node_id);
    snprintf(message, sizeof(message), 
        "{\"node_id\":%d,\"status\":\"%s\",\"timestamp\":%ld}",
        node_id, status, (long)time(NULL));
    
    pgbalancer_mqtt_publish(topic, message);
}

/*
 * Publish failover event
 */
void pgbalancer_mqtt_publish_failover(int old_primary, int new_primary) {
    char message[512];
    
    snprintf(message, sizeof(message),
        "{\"event\":\"failover\",\"old_primary\":%d,\"new_primary\":%d,\"timestamp\":%ld}",
        old_primary, new_primary, (long)time(NULL));
    
    pgbalancer_mqtt_publish("pgbalancer/events/failover", message);
}

/*
 * Publish health check result
 */
void pgbalancer_mqtt_publish_health(int node_id, int is_healthy) {
    char topic[128];
    char message[512];
    
    snprintf(topic, sizeof(topic), "pgbalancer/nodes/%d/health", node_id);
    snprintf(message, sizeof(message),
        "{\"node_id\":%d,\"healthy\":%s,\"timestamp\":%ld}",
        node_id, is_healthy ? "true" : "false", (long)time(NULL));
    
    pgbalancer_mqtt_publish(topic, message);
}

/*
 * Publish connection pool statistics
 */
void pgbalancer_mqtt_publish_pool_stats(int total_connections, int active_connections, int idle_connections) {
    char message[512];
    
    snprintf(message, sizeof(message),
        "{\"total\":%d,\"active\":%d,\"idle\":%d,\"timestamp\":%ld}",
        total_connections, active_connections, idle_connections, (long)time(NULL));
    
    pgbalancer_mqtt_publish("pgbalancer/stats/connections", message);
}

/*
 * Publish query statistics
 */
void pgbalancer_mqtt_publish_query_stats(int queries_per_sec, double avg_response_time) {
    char message[512];
    
    snprintf(message, sizeof(message),
        "{\"qps\":%d,\"avg_response_time_ms\":%.2f,\"timestamp\":%ld}",
        queries_per_sec, avg_response_time, (long)time(NULL));
    
    pgbalancer_mqtt_publish("pgbalancer/stats/queries", message);
}

/*
 * Publish node attach/detach event
 */
void pgbalancer_mqtt_publish_node_event(int node_id, const char *event_type) {
    char topic[128];
    char message[512];
    
    snprintf(topic, sizeof(topic), "pgbalancer/nodes/%d/events", node_id);
    snprintf(message, sizeof(message),
        "{\"node_id\":%d,\"event\":\"%s\",\"timestamp\":%ld}",
        node_id, event_type, (long)time(NULL));
    
    pgbalancer_mqtt_publish(topic, message);
}

/*
 * Shutdown MQTT client
 */
void pgbalancer_mqtt_shutdown(void) {
    if (!mqtt_enabled) return;
    
    fprintf(stderr, "[MQTT] Shutting down MQTT publisher\n");
    
    if (mqtt_socket >= 0) {
        close(mqtt_socket);
        mqtt_socket = -1;
    }
}

/*
 * Enable MQTT at runtime
 */
void pgbalancer_mqtt_enable(int enable) {
    mqtt_enabled = enable;
    fprintf(stderr, "[MQTT] MQTT publishing %s\n", enable ? "enabled" : "disabled");
}

