/*-------------------------------------------------------------------------
 *
 * buffer.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef BUFFER_H

#define BUFFER_H

#define SKIP_TABS(p) while (*p == '\t') {p++;}

extern int	buffer_read_int(char *buf, char **bufp);
extern char *buffer_read_string(char *buf, char **bufp);
extern char buffer_read_char(char *buf, char **bufp);

#endif
