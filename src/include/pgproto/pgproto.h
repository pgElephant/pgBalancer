/*-------------------------------------------------------------------------
 *
 * pgproto.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PGPROTO_H
#define PGPROTO_H

/*
 * The default file name for protocol data to be sent from frontend to backend
 */
#define PGPROTODATA "pgproto.data"

#define MAXENTRIES 128

/*
 * Protocol data representation read from the data file.
 * We assume that the protocol version is V3.
 */
typedef struct
{
	char		type;			/* message type */
	int			size;			/* message length excluding this (in host
								 * order) */
	char		message[1];		/* actual message (variable length) */
}			PROTO_DATA;

extern int	read_nap;

#endif							/* PGPROTO_H */
