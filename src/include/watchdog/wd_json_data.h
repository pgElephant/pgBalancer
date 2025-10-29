/*-------------------------------------------------------------------------
 *
 * wd_json_data.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "utils/json.h"
#include "pool_config.h"
#include "parser/pg_list.h"
#include "watchdog/watchdog.h"

#ifndef WD_JSON_DATA_H
#define WD_JSON_DATA_H

/*
 * The structure to hold the parsed PG backend node status data fetched
 * from the leader watchdog node
 */
typedef struct WDPGBackendStatus
{
	int			primary_node_id;
	int			node_count;
	BACKEND_STATUS backend_status[MAX_NUM_BACKENDS];
	char		nodeName[WD_MAX_HOST_NAMELEN];	/* name of the watchdog node
												 * that sent the data */
} WDPGBackendStatus;

extern WatchdogNode *get_watchdog_node_from_json(char *json_data, int data_len, char **authkey);
extern char *get_watchdog_node_info_json(WatchdogNode *wdNode, char *authkey);
extern POOL_CONFIG *get_pool_config_from_json(char *json_data, int data_len);
extern char *get_pool_config_json(void);
extern char *get_lifecheck_node_status_change_json(int nodeID, int nodeStatus, char *message, char *authKey);
extern bool parse_node_status_json(char *json_data, int data_len, int *nodeID, int *nodeStatus, char **message);


extern bool parse_beacon_message_json(char *json_data, int data_len, int *state,
									  long *seconds_since_node_startup,
									  long *seconds_since_current_state,
									  int *quorumStatus,
									  int *standbyNodesCount,
									  bool *escalated);
extern char *get_beacon_message_json(WatchdogNode *wdNode);

extern char *get_wd_node_function_json(char *func_name, int *node_id_set, int count, unsigned char flags, unsigned int sharedKey, char *authKey);
extern bool parse_wd_node_function_json(char *json_data, int data_len, char **func_name, int **node_id_set, int *count, unsigned char *flags);
extern char *get_wd_simple_message_json(char *message);

extern WDPGBackendStatus *get_pg_backend_node_status_from_json(char *json_data, int data_len);
extern char *get_backend_node_status_json(WatchdogNode *wdNode);

extern char *get_simple_request_json(char *key, char *value, unsigned int sharedKey, char *authKey);

extern bool parse_data_request_json(char *json_data, int data_len, char **request_type);
extern char *get_data_request_json(char *request_type, unsigned int sharedKey, char *authKey);

extern bool
			parse_wd_exec_cluster_command_json(char *json_data, int data_len,
											   char **clusterCommand, List **args_list);

extern char *get_wd_exec_cluster_command_json(char *clusterCommand, List *args_list,
											  unsigned int sharedKey, char *authKey);

#endif
