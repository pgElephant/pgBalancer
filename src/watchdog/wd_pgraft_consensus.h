/*-------------------------------------------------------------------------
 *
 * wd_pgraft_consensus.h
 *      pgraft Raft Consensus Integration for pgbalancer
 *
 * Copyright (c) 2025, pgElephant, Inc.
 *
 * This module provides integration between pgbalancer's watchdog and
 * pgraft's Raft consensus algorithm for leader election.
 *
 *-------------------------------------------------------------------------
 */
#ifndef WD_PGRAFT_CONSENSUS_H
#define WD_PGRAFT_CONSENSUS_H

#include <stdbool.h>
#include <libpq-fe.h>

/*
 * pgraft node information structure
 */
typedef struct
{
	int			node_id;
	char		hostname[256];
	int			port;			/* PostgreSQL port */
	int			pgraft_port;	/* Raft consensus port */
	bool		is_leader;
	char		state[32];		/* leader, follower, candidate */
	int			term;			/* Raft term number */
} PgRaftNodeInfo;

/*
 * pgraft cluster status
 */
typedef struct
{
	int			num_nodes;
	int			leader_id;
	int			current_term;
	bool		has_quorum;		/* N/2 + 1 nodes available */
	int			quorum_size;	/* Required quorum size */
	PgRaftNodeInfo *nodes;
} PgRaftClusterStatus;

/*
 * Consensus mode enumeration
 */
typedef enum
{
	CONSENSUS_MODE_HEURISTIC = 0,	/* Heuristic-based (existing: watchdog + metrics) */
	CONSENSUS_MODE_PGRAFT			/* Raft consensus via pgraft extension */
} ConsensusMode;

/*
 * Main initialization and lifecycle functions
 */
extern bool wd_pgraft_init(const char *cluster_id, int node_id, 
							const char *address, int port);
extern void wd_pgraft_shutdown(void);
extern bool wd_pgraft_is_enabled(void);

/*
 * Leader election functions
 */
extern bool wd_pgraft_is_leader(void);
extern int wd_pgraft_get_leader_id(void);
extern bool wd_pgraft_election_in_progress(void);

/*
 * Cluster membership and status
 */
extern PgRaftClusterStatus *wd_pgraft_get_cluster_status(void);
extern bool wd_pgraft_add_node(int node_id, const char *address, int port);
extern bool wd_pgraft_remove_node(int node_id);
extern void wd_pgraft_free_cluster_status(PgRaftClusterStatus *status);

/*
 * Quorum and consensus
 */
extern bool wd_pgraft_has_quorum(void);
extern int wd_pgraft_get_quorum_size(void);
extern bool wd_pgraft_can_make_decisions(void);

/*
 * State replication (for future use)
 */
extern bool wd_pgraft_replicate_config(const char *key, const char *value);
extern char *wd_pgraft_get_replicated_config(const char *key);

/*
 * Health and monitoring
 */
extern bool wd_pgraft_is_healthy(void);
extern int wd_pgraft_get_term(void);
extern const char *wd_pgraft_get_state(void);

/*
 * Utility functions
 */
extern ConsensusMode wd_get_consensus_mode(void);
extern const char *wd_consensus_mode_to_string(ConsensusMode mode);
extern bool wd_should_use_pgraft(void);

/*
 * Leader detection and failover functions
 */
extern int wd_pgraft_find_leader_backend(void);

#endif							/* WD_PGRAFT_CONSENSUS_H */

