/*-------------------------------------------------------------------------
 *
 * miscnodes.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef MISCNODES_H
#define MISCNODES_H

#include "nodes.h"

/*
 * ErrorSaveContext -
 *		function call context node for handling of "soft" errors
 *
 * A caller wishing to trap soft errors must initialize a struct like this
 * with all fields zero/NULL except for the NodeTag.  Optionally, set
 * details_wanted = true if more than the bare knowledge that a soft error
 * occurred is required.  The struct is then passed to a SQL-callable function
 * via the FunctionCallInfo.context field; or below the level of SQL calls,
 * it could be passed to a subroutine directly.
 *
 * After calling code that might report an error this way, check
 * error_occurred to see if an error happened.  If so, and if details_wanted
 * is true, error_data has been filled with error details (stored in the
 * callee's memory context!).  The ErrorData can be modified (e.g. downgraded
 * to a WARNING) and reported with ThrowErrorData().  FreeErrorData() can be
 * called to release error_data, although that step is typically not necessary
 * if the called code was run in a short-lived context.
 */
typedef struct ErrorSaveContext
{
	NodeTag		type;
	bool		error_occurred; /* set to true if we detect a soft error */
	bool		details_wanted; /* does caller want more info than that? */
	ErrorData  *error_data;		/* details of error, if so */
} ErrorSaveContext;

#endif							/* MISCNODES_H */
