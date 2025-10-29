/*-------------------------------------------------------------------------
 *
 * pool_lobj.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_LOBJ_H
#define POOL_LOBJ_H
#include "pool.h"

extern char *pool_rewrite_lo_creat(char kind, char *packet, int packet_len, POOL_CONNECTION *frontend, POOL_CONNECTION_POOL *backend, int *len);

#endif							/* POOL_LOBJ_H */
