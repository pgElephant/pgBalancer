/*-------------------------------------------------------------------------
 *
 * pool_internal_comms.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef pool_internal_comms_h
#define pool_internal_comms_h

extern bool terminate_pgpool(char mode, bool error);
extern void notice_backend_error(int node_id, unsigned char flags);
extern bool degenerate_backend_set(int *node_id_set, int count,
								   unsigned char flags);
extern bool degenerate_backend_set_ex(int *node_id_set, int count,
									  unsigned char flags, bool error, bool test_only);
extern bool promote_backend(int node_id, unsigned char flags);
extern bool send_failback_request(int node_id, bool throw_error,
								  unsigned char flags);

extern void degenerate_all_quarantine_nodes(void);
extern bool close_idle_connections(void);

/* defined in pgbalancer_main.c */
extern void register_watchdog_quorum_change_interrupt(void);
extern void register_watchdog_state_change_interrupt(void);
extern void register_backend_state_sync_req_interrupt(void);
extern void register_inform_quarantine_nodes_req(void);
extern bool register_node_operation_request(POOL_REQUEST_KIND kind,
											int *node_id_set, int count, unsigned char flags);
#endif							/* pool_internal_comms_h */
