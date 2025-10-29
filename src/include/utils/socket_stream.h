/*-------------------------------------------------------------------------
 *
 * socket_stream.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef SOCKET_STREAM_H
#define SOCKET_STREAM_H

extern void socket_set_nonblock(int fd);
extern void socket_unset_nonblock(int fd);

extern int	socket_read(int sock, void *buf, size_t len, int timeout);
extern int	socket_write(int fd, void *buf, size_t len);


#endif							/* SOCKET_STREAM_H */
