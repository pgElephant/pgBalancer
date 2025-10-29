/*-------------------------------------------------------------------------
 *
 * pcp_stubs.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "pool.h"
#include "utils/elog.h"

/*
 * Stub for pcp_main - PCP worker main function
 * PCP functionality replaced by REST API via bctl
 */
void pcp_main(int *fds)
{
	ereport(WARNING,
			(errmsg("PCP protocol has been replaced with REST API"),
			 errdetail("Use bctl utility to manage pgbalancer"),
			 errhint("See documentation for bctl usage")));
	/* Do nothing - PCP is disabled */
}

/*
 * Stub for send_to_pcp_frontend
 * No-op since PCP is replaced by REST API
 */
int send_to_pcp_frontend(char *data, int len, bool flush)
{
	/* No-op: PCP communication disabled */
	return 0;
}

/*
 * Stub for pcp_frontend_exists
 * Always returns false since PCP is disabled
 */
int pcp_frontend_exists(void)
{
	/* PCP frontends don't exist anymore */
	return 0;
}

/*
 * Stub for pcp_worker_wakeup_request
 * No-op since PCP worker doesn't exist
 */
void pcp_worker_wakeup_request(void)
{
	/* No-op: PCP worker disabled */
}

