/*-------------------------------------------------------------------------
 *
 * scansup.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef SCANSUP_H
#define SCANSUP_H

extern char *downcase_truncate_identifier(const char *ident, int len,
										  bool warn);

extern char *downcase_identifier(const char *ident, int len,
								 bool warn, bool truncate);

extern void truncate_identifier(char *ident, int len, bool warn);

extern bool scanner_isspace(char ch);

#endif							/* SCANSUP_H */
