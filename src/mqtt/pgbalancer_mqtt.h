/*-------------------------------------------------------------------------
 *
 * pgbalancer_mqtt.h
 *      MQTT event publisher for pgbalancer
 *
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PGBALANCER_MQTT_H
#define PGBALANCER_MQTT_H

/* MQTT Configuration */
#define MQTT_ENABLED 0  /* Set to 1 to enable MQTT publishing */

/*
 * Initialize MQTT client
 * Returns 0 on success, -1 on error
 */
int pgbalancer_mqtt_init(const char *broker_address, int broker_port, const char *client_id);

/*
 * Publish event to MQTT broker
 */
int pgbalancer_mqtt_publish(const char *topic, const char *message);

/*
 * Publish node status change
 */
void pgbalancer_mqtt_publish_node_status(int node_id, const char *status);

/*
 * Publish failover event
 */
void pgbalancer_mqtt_publish_failover(int old_primary, int new_primary);

/*
 * Publish health check result
 */
void pgbalancer_mqtt_publish_health(int node_id, int is_healthy);

/*
 * Shutdown MQTT client
 */
void pgbalancer_mqtt_shutdown(void);

#endif /* PGBALANCER_MQTT_H */

