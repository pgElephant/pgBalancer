/*-------------------------------------------------------------------------
 *
 * ps_status.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef ps_status_h
#define ps_status_h

#include "pool.h"
#include <netdb.h>

extern char remote_ps_data[NI_MAXHOST + NI_MAXSERV + 2];	/* used for
															 * set_ps_display */

extern char **save_ps_display_args(int argc, char **argv);
extern void init_ps_display(const char *username, const char *dbname,
							const char *host_info, const char *initial_str);
extern void set_ps_display(const char *activity, bool force);
extern const char *get_ps_display(int *displen);
extern void pool_ps_idle_display(POOL_CONNECTION_POOL *backend);


#endif							/* ps_status_h */
