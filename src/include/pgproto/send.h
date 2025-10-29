/*-------------------------------------------------------------------------
 *
 * send.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef SEND_H
#define SEND_H

extern void send_char(char c, PGconn *conn);
extern void send_int(int intval, PGconn *conn);
extern void send_int16(short shortval, PGconn *conn);
extern void send_string(char *buf, PGconn *conn);
extern void send_byte(char *buf, int len, PGconn *conn);

#endif
