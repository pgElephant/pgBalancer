/*-------------------------------------------------------------------------
 *
 * pool_connection_pool.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef pool_connection_pool_h
#define pool_connection_pool_h

extern POOL_CONNECTION_POOL *pool_connection_pool;	/* connection pool */

extern int	pool_init_cp(void);
extern POOL_CONNECTION_POOL *pool_create_cp(void);
extern POOL_CONNECTION_POOL *pool_get_cp(char *user, char *database, int protoMajor, int check_socket);
extern void pool_discard_cp(char *user, char *database, int protoMajor);
extern void pool_backend_timer(void);
extern void pool_connection_pool_timer(POOL_CONNECTION_POOL *backend);
extern RETSIGTYPE pool_backend_timer_handler(int sig);
extern int	connect_inet_domain_socket(int slot, bool retry);
extern int	connect_unix_domain_socket(int slot, bool retry);
extern int	connect_inet_domain_socket_by_port(char *host, int port, bool retry);
extern int	connect_unix_domain_socket_by_port(int port, char *socket_dir, bool retry);
extern int	pool_pool_index(void);
extern void close_all_backend_connections(void);
extern void update_pooled_connection_count(void);
extern int	in_use_backend_id(POOL_CONNECTION_POOL *pool);

#endif							/* pool_connection_pool_h */
