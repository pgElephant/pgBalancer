/*-------------------------------------------------------------------------
 *
 * pool_process_reporting.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_PROCESS_REPORTING_H
#define POOL_PROCESS_REPORTING_H

extern void send_row_description(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend,
								 short num_fields, char **field_names);
extern void send_complete_and_ready(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend, const char *message, const int num_rows);
extern POOL_REPORT_CONFIG *get_config(int *nrows);
extern POOL_REPORT_POOLS *get_pools(int *nrows);
extern POOL_REPORT_PROCESSES *get_processes(int *nrows);
extern POOL_REPORT_NODES *get_nodes(int *nrows, int node_id);
extern POOL_REPORT_VERSION *get_version(void);
extern POOL_HEALTH_CHECK_STATS *get_health_check_stats(int *nrows);
extern POOL_BACKEND_STATS *get_backend_stats(int *nrows);

extern void config_reporting(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern void pools_reporting(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern void processes_reporting(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern void nodes_reporting(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern void version_reporting(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern void cache_reporting(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern void show_health_check_stats(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern void show_backend_stats(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);


extern void send_config_var_detail_row(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend, const char *name, const char *value, const char *description);
extern void send_config_var_value_only_row(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend, const char *value);
extern char *get_backend_status_string(BACKEND_STATUS status);

extern int *pool_report_pools_offsets(int *n);
#endif
