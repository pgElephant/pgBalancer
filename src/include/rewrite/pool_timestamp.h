/*-------------------------------------------------------------------------
 *
 * pool_timestamp.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_TIMESTAMP_H
#define POOL_TIMESTAMP_H
#include "pool.h"
#include "protocol/pool_proto_modules.h"
#include "parser/nodes.h"
#include "context/pool_session_context.h"

extern char *rewrite_timestamp(POOL_CONNECTION_POOL *backend, Node *node, bool rewrite_to_params, POOL_SENT_MESSAGE *message);
extern char *bind_rewrite_timestamp(POOL_CONNECTION_POOL *backend, POOL_SENT_MESSAGE *message, const char *orig_msg, int *len);
extern bool isSystemType(Node *node, const char *name);

#endif							/* POOL_TIMESTAMP_H */
