/*-------------------------------------------------------------------------
 *
 * recovery.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef recovery_h
#define recovery_h

extern void start_recovery(int recovery_node);
extern void finish_recovery(void);
extern int	wait_connection_closed(void);
extern int	ensure_conn_counter_validity(void);

#endif /* recovery_h */

