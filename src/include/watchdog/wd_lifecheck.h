/*-------------------------------------------------------------------------
 *
 * wd_lifecheck.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "watchdog/watchdog.h"
#include "pool_config.h"

#ifndef WD_LIFECHECK_H
#define WD_LIFECHECK_H


typedef enum NodeState
{
	NODE_EMPTY,
	NODE_DEAD,
	NODE_ALIVE
} NodeStates;

typedef struct LifeCheckNode
{
	NodeStates	nodeState;
	int			ID;
	WD_STATES	wdState;
	char		stateName[128];
	char		hostName[WD_MAX_HOST_NAMELEN];
	char		nodeName[WD_MAX_NODE_NAMELEN];
	int			wdPort;
	int			pgpoolPort;
	int			retry_lives;
	struct timeval hb_send_time;	/* send time */
	struct timeval hb_last_recv_time;	/* recv time */
} LifeCheckNode;

typedef struct lifeCheckCluster
{
	int			nodeCount;
	struct LifeCheckNode *lifeCheckNodes;
} LifeCheckCluster;

extern LifeCheckCluster *gslifeCheckCluster;	/* lives in shared memory */


/* wd_lifecheck.c */
extern pid_t initialize_watchdog_lifecheck(void);


/* wd_heartbeat.c */
extern pid_t wd_hb_receiver(int fork_wait_time, WdHbIf *hb_if);
extern pid_t wd_hb_sender(int fork_wait_time, WdHbIf *hb_if);


#endif
