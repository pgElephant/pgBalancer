/*-------------------------------------------------------------------------
 *
 * extended_query.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef EXTENDED_QUERY_H
#define EXTENDED_QUERY_H

extern void process_parse(char *buf, PGconn *conn);
extern void process_bind(char *buf, PGconn *conn);
extern void process_execute(char *buf, PGconn *conn);
extern void process_describe(char *buf, PGconn *conn);
extern void process_close(char *buf, PGconn *conn);

#endif
