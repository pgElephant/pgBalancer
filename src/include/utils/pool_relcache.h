/*-------------------------------------------------------------------------
 *
 * pool_relcache.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_RELCACHE_H
#define POOL_RELCACHE_H

/* ------------------------
 * Relation cache structure
 *-------------------------
*/
#define MAX_ITEM_LENGTH	1024

/* Relation lookup cache structure */

typedef struct
{
	char		dbname[MAX_ITEM_LENGTH];	/* database name */
	char		relname[MAX_ITEM_LENGTH];	/* table name */
	void	   *data;			/* user data */
	int			refcnt;			/* reference count */
	int			session_id;		/* LocalSessionId */
	time_t		expire;			/* cache expiration absolute time in seconds */
} PoolRelCache;

#define	MAX_QUERY_LENGTH	1500
typedef struct
{
	int			num;			/* number of cache items */
	char		sql[MAX_QUERY_LENGTH];	/* Query to relation */

	/*
	 * User defined function to be called at data register. Argument is
	 * POOL_SELECT_RESULT *. This function must return a pointer to be saved
	 * in cache->data.
	 */
	void	   *(*register_func) (POOL_SELECT_RESULT *data);

	/*
	 * User defined function to be called at data unregister. Argument
	 * cache->data.
	 */
	void	   *(*unregister_func) (void *data);
	bool		cache_is_session_local; /* True if cache life time is session
										 * local */
	bool		no_cache_if_zero;	/* if register func returns 0, do not
									 * cache the data */
	PoolRelCache *cache;		/* cache data */
} POOL_RELCACHE;

extern POOL_RELCACHE *pool_create_relcache(int cachesize, char *sql,
										   void *(*register_func) (POOL_SELECT_RESULT *), void *(*unregister_func) (void *),
										   bool issessionlocal);
extern void pool_discard_relcache(POOL_RELCACHE *relcache);
extern void *pool_search_relcache(POOL_RELCACHE *relcache, POOL_CONNECTION_POOL *backend, char *table);
extern char *remove_quotes_and_schema_from_relname(char *table);
extern void *int_register_func(POOL_SELECT_RESULT *res);
extern void *int_unregister_func(void *data);
extern void *string_register_func(POOL_SELECT_RESULT *res);
extern void *string_unregister_func(void *data);
extern bool SplitIdentifierString(char *rawstring, char separator, Node **namelist);

#endif							/* POOL_RELCACHE_H */
