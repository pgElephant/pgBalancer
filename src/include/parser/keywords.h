/*-------------------------------------------------------------------------
 *
 * keywords.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef KEYWORDS_H
#define KEYWORDS_H

#include "kwlookup.h"

/* Keyword categories --- should match lists in gram.y */
#define UNRESERVED_KEYWORD		0
#define COL_NAME_KEYWORD		1
#define TYPE_FUNC_NAME_KEYWORD	2
#define RESERVED_KEYWORD		3

extern PGDLLIMPORT const ScanKeywordList ScanKeywords;
extern PGDLLIMPORT const uint8 ScanKeywordCategories[];
extern PGDLLIMPORT const bool ScanKeywordBareLabel[];

#endif							/* KEYWORDS_H */
