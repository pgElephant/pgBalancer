/*-------------------------------------------------------------------------
 *
 * read.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef READ_H
#define READ_H

extern void read_until_ready_for_query(PGconn *conn, int check_input, int wait_for_ready_for_query);
#endif
