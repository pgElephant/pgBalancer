/*-------------------------------------------------------------------------
 *
 * keywords.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "keywords.h"

/* ScanKeywordList lookup data for SQL keywords */

#include "kwlist_d.h"

/* Keyword categories for SQL keywords */

#define PG_KEYWORD(kwname, value, category, collabel) category,

const uint8 ScanKeywordCategories[SCANKEYWORDS_NUM_KEYWORDS] = {
#include "kwlist.h"
};

#undef PG_KEYWORD

/* Keyword can-be-bare-label flags for SQL keywords */

#define PG_KEYWORD(kwname, value, category, collabel) collabel,

#define BARE_LABEL true
#define AS_LABEL false

const bool	ScanKeywordBareLabel[SCANKEYWORDS_NUM_KEYWORDS] = {
#include "parser/kwlist.h"
};

#undef PG_KEYWORD
#undef BARE_LABEL
#undef AS_LABEL
