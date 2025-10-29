/*-------------------------------------------------------------------------
 *
 * pgbalancer_rest_api.h
 *      REST API server header for pgbalancer
 *
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PGBALANCER_REST_API_H
#define PGBALANCER_REST_API_H

/*
 * Initialize REST API server on specified port
 * Returns 0 on success, -1 on error
 */
int pgbalancer_rest_api_init(int port);

/*
 * Poll REST API server (call periodically from main loop)
 * timeout_ms: timeout in milliseconds
 */
void pgbalancer_rest_api_poll(int timeout_ms);

/*
 * Check if server received stop signal
 * Returns non-zero if server should stop
 */
int pgbalancer_rest_api_should_stop(void);

/*
 * Shutdown REST API server and free resources
 */
void pgbalancer_rest_api_shutdown(void);

#endif /* PGBALANCER_REST_API_H */

