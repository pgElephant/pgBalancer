/*-------------------------------------------------------------------------
 *
 * pool_pg_utils.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef pool_pg_utils_h
#define pool_pg_utils_h

#include "pool.h"

#define MAX_PG_VERSION_STRING	512

/*
 * PostgreSQL version descriptor
 */
typedef struct
{
	short		major;			/* major version number in up to 3 digits
								 * decimal. Examples: 120, 110, 100, 96. */
	short		minor;			/* minor version number in up to 2 digits
								 * decimal. Examples: 0, 1, 2, 10, 23. */
	char		version_string[MAX_PG_VERSION_STRING + 1];	/* original version
															 * string */
} PGVersion;


extern void send_startup_packet(POOL_CONNECTION_POOL_SLOT *cp);
extern void pool_free_startup_packet(StartupPacket *sp);

extern POOL_CONNECTION_POOL_SLOT *make_persistent_db_connection(
																int db_node_id, char *hostname, int port, char *dbname, char *user, char *password, bool retry);
extern POOL_CONNECTION_POOL_SLOT *make_persistent_db_connection_noerror(
																		int db_node_id, char *hostname, int port, char *dbname, char *user, char *password, bool retry);
extern void discard_persistent_db_connection(POOL_CONNECTION_POOL_SLOT *cp);
extern int	select_load_balancing_node(void);

extern PGVersion *Pgversion(POOL_CONNECTION_POOL *backend);

/* pool_pg_utils.c */
extern bool si_snapshot_prepared(void);
extern bool si_snapshot_acquire_command(Node *node);
extern void si_acquire_snapshot(void);
extern void si_snapshot_acquired(void);
extern void si_commit_request(void);
extern void si_commit_done(void);
extern int	check_replication_delay(int node_id);

#endif							/* pool_pg_utils_h */
