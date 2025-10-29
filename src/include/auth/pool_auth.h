/*-------------------------------------------------------------------------
 *
 * pool_auth.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef pool_auth_h
#define pool_auth_h

extern void connection_do_auth(POOL_CONNECTION_POOL_SLOT *cp, char *password);
extern int	pool_do_auth(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend);
extern int	pool_do_reauth(POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *cp);
extern void authenticate_frontend(POOL_CONNECTION *frontend);

extern void pool_random_salt(char *md5Salt);
extern void pool_random(void *buf, size_t len);

#endif							/* pool_auth_h */
