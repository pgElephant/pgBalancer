/*-------------------------------------------------------------------------
 *
 * wd_internal_commands.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef WD_INTERNAL_COMMANDS_H
#define WD_INTERNAL_COMMANDS_H

#include "watchdog/wd_ipc_defines.h"
#include "watchdog/wd_json_data.h"
#include "watchdog/wd_ipc_conn.h"
#include "watchdog/wd_commands.h"
#include "parser/pg_list.h"

/*
 * These lock can only be acquired by
 * coordinator watchdog node on standby
 * watchdog node.
 */
typedef enum WD_LOCK_STANDBY_TYPE
{
	WD_INVALID_LOCK,
	/* currently we have only one lock */
	WD_FOLLOW_PRIMARY_LOCK
} WD_LOCK_STANDBY_TYPE;


extern WdCommandResult wd_start_recovery(void);
extern WdCommandResult wd_end_recovery(void);
extern WDFailoverCMDResults wd_send_failback_request(int node_id, unsigned char flags);
extern WDFailoverCMDResults wd_degenerate_backend_set(int *node_id_set, int count, unsigned char flags);
extern WDFailoverCMDResults wd_promote_backend(int node_id, unsigned char flags);

extern WdCommandResult wd_execute_cluster_command(char *clusterCommand, List *argsList);

extern WDPGBackendStatus *get_pg_backend_status_from_leader_wd_node(void);

extern WD_STATES wd_internal_get_watchdog_local_node_state(void);
extern int	wd_internal_get_watchdog_quorum_state(void);
extern char *wd_internal_get_watchdog_nodes_json(int nodeID);

extern void wd_ipc_initialize_data(void);

/* functions for failover commands interlocking */
extern WDFailoverCMDResults wd_failover_end(void);
extern WDFailoverCMDResults wd_failover_start(void);

extern unsigned int *get_ipc_shared_key(void);
extern void set_watchdog_process_needs_cleanup(void);
extern void reset_watchdog_process_needs_cleanup(void);
extern bool get_watchdog_process_needs_cleanup(void);
extern void set_watchdog_node_escalated(void);
extern void reset_watchdog_node_escalated(void);
extern bool get_watchdog_node_escalation_state(void);
extern size_t wd_ipc_get_shared_mem_size(void);

extern WdCommandResult wd_lock_standby(WD_LOCK_STANDBY_TYPE lock_type);
extern WdCommandResult wd_unlock_standby(WD_LOCK_STANDBY_TYPE lock_type);

#endif							/* WD_INTERNAL_COMMANDS_H */
