/*-------------------------------------------------------------------------
 *
 * wd_pgraft_consensus.c
 *      pgraft Raft Consensus Integration for pgbalancer
 *
 * Copyright (c) 2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pool.h"
#include "pool_config.h"
#include "utils/elog.h"
#include "wd_pgraft_consensus.h"

/* Global state */
static bool pgraft_initialized = false;
static PGconn *pgraft_conn = NULL;
static char pgraft_cluster_id[256] = "";
static int pgraft_node_id = -1;

/*
 * Helper function to execute SQL and get single value
 */
static char *
execute_pgraft_query(const char *query)
{
	PGresult   *res;
	char	   *result = NULL;

	if (!pgraft_conn || PQstatus(pgraft_conn) != CONNECTION_OK)
	{
		ereport(WARNING,
				(errmsg("pgraft consensus: not connected to database")));
		return NULL;
	}

	res = PQexec(pgraft_conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		ereport(WARNING,
				(errmsg("pgraft consensus: query failed: %s", 
						PQerrorMessage(pgraft_conn))));
		PQclear(res);
		return NULL;
	}

	if (PQntuples(res) > 0 && PQnfields(res) > 0)
	{
		char	   *val = PQgetvalue(res, 0, 0);

		if (val)
			result = pstrdup(val);
	}

	PQclear(res);
	return result;
}

/*
 * Initialize pgraft consensus
 */
bool
wd_pgraft_init(const char *cluster_id, int node_id, 
			   const char *address, int port)
{
	char		conninfo[1024];
	PGresult   *res;

	if (pgraft_initialized)
	{
		ereport(LOG,
				(errmsg("pgraft consensus already initialized")));
		return true;
	}

	/* Build connection string */
	snprintf(conninfo, sizeof(conninfo),
			 "host=%s port=%d dbname=postgres user=%s",
			 address, port, pool_config->sr_check_user);

	/* Connect to PostgreSQL */
	pgraft_conn = PQconnectdb(conninfo);

	if (PQstatus(pgraft_conn) != CONNECTION_OK)
	{
		ereport(ERROR,
				(errmsg("pgraft consensus: failed to connect to database: %s",
						PQerrorMessage(pgraft_conn))));
		PQfinish(pgraft_conn);
		pgraft_conn = NULL;
		return false;
	}

	ereport(LOG,
			(errmsg("pgraft consensus: connected to PostgreSQL at %s:%d",
					address, port)));

	/* Check if pgraft extension exists */
	res = PQexec(pgraft_conn, 
				 "SELECT 1 FROM pg_extension WHERE extname = 'pgraft'");

	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
	{
		ereport(ERROR,
				(errmsg("pgraft consensus: pgraft extension not found"),
				 errhint("CREATE EXTENSION pgraft;")));
		PQclear(res);
		PQfinish(pgraft_conn);
		pgraft_conn = NULL;
		return false;
	}
	PQclear(res);

	/* Initialize pgraft if not already done */
	res = PQexec(pgraft_conn, "SELECT pgraft_init()");

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		ereport(WARNING,
				(errmsg("pgraft consensus: pgraft_init() returned: %s",
						PQerrorMessage(pgraft_conn))));
	}
	PQclear(res);

	/* Store configuration */
	snprintf(pgraft_cluster_id, sizeof(pgraft_cluster_id), "%s", cluster_id);
	pgraft_node_id = node_id;
	pgraft_initialized = true;

	ereport(LOG,
			(errmsg("pgraft consensus: initialized successfully"),
			 errdetail("cluster_id=%s, node_id=%d", cluster_id, node_id)));

	return true;
}

/*
 * Shutdown pgraft consensus
 */
void
wd_pgraft_shutdown(void)
{
	if (pgraft_conn)
	{
		PQfinish(pgraft_conn);
		pgraft_conn = NULL;
	}

	pgraft_initialized = false;

	ereport(LOG,
			(errmsg("pgraft consensus: shut down")));
}

/*
 * Check if pgraft is enabled
 */
bool
wd_pgraft_is_enabled(void)
{
	return pgraft_initialized && 
		   pgraft_conn && 
		   PQstatus(pgraft_conn) == CONNECTION_OK;
}

/*
 * Check if current node is leader
 */
bool
wd_pgraft_is_leader(void)
{
	char	   *result;
	bool		is_leader = false;

	if (!wd_pgraft_is_enabled())
		return false;

	result = execute_pgraft_query("SELECT pgraft_is_leader()");

	if (result)
	{
		is_leader = (strcmp(result, "t") == 0);
		pfree(result);
	}

	return is_leader;
}

/*
 * Get current leader ID
 */
int
wd_pgraft_get_leader_id(void)
{
	PGresult   *res;
	int			leader_id = -1;

	if (!wd_pgraft_is_enabled())
		return -1;

	res = PQexec(pgraft_conn, 
				 "SELECT leader_id FROM pgraft_get_cluster_status()");

	if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
	{
		char	   *val = PQgetvalue(res, 0, 0);

		if (val)
			leader_id = atoi(val);
	}

	PQclear(res);
	return leader_id;
}

/*
 * Check if election is in progress
 */
bool
wd_pgraft_election_in_progress(void)
{
	char	   *state;
	bool		in_election = false;

	if (!wd_pgraft_is_enabled())
		return false;

	state = execute_pgraft_query(
		"SELECT state FROM pgraft_get_cluster_status()");

	if (state)
	{
		in_election = (strcmp(state, "candidate") == 0);
		pfree(state);
	}

	return in_election;
}

/*
 * Get cluster status
 */
PgRaftClusterStatus *
wd_pgraft_get_cluster_status(void)
{
	PGresult   *res;
	PgRaftClusterStatus *status;
	int			i;

	if (!wd_pgraft_is_enabled())
		return NULL;

	status = (PgRaftClusterStatus *) palloc0(sizeof(PgRaftClusterStatus));

	/* Get cluster info */
	res = PQexec(pgraft_conn, "SELECT * FROM pgraft_get_cluster_status()");

	if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
	{
		status->current_term = atoi(PQgetvalue(res, 0, 
			PQfnumber(res, "current_term")));
		status->leader_id = atoi(PQgetvalue(res, 0, 
			PQfnumber(res, "leader_id")));
		status->num_nodes = atoi(PQgetvalue(res, 0, 
			PQfnumber(res, "num_nodes")));
	}
	PQclear(res);

	/* Get node details */
	res = PQexec(pgraft_conn, "SELECT * FROM pgraft_get_nodes()");

	if (PQresultStatus(res) == PGRES_TUPLES_OK)
	{
		int			ntuples = PQntuples(res);

		status->num_nodes = ntuples;
		status->nodes = (PgRaftNodeInfo *) palloc0(
			ntuples * sizeof(PgRaftNodeInfo));

		for (i = 0; i < ntuples; i++)
		{
			status->nodes[i].node_id = atoi(PQgetvalue(res, i, 0));
			strncpy(status->nodes[i].hostname, PQgetvalue(res, i, 1), 255);
			status->nodes[i].port = atoi(PQgetvalue(res, i, 2));
			status->nodes[i].is_leader = 
				(strcmp(PQgetvalue(res, i, 3), "t") == 0);
		}

		/* Calculate quorum: N/2 + 1 */
		status->quorum_size = (ntuples / 2) + 1;
		status->has_quorum = (ntuples >= status->quorum_size);
	}

	PQclear(res);
	return status;
}

/*
 * Free cluster status
 */
void
wd_pgraft_free_cluster_status(PgRaftClusterStatus *status)
{
	if (status)
	{
		if (status->nodes)
			pfree(status->nodes);
		pfree(status);
	}
}

/*
 * Add node to cluster (must be leader)
 */
bool
wd_pgraft_add_node(int node_id, const char *address, int port)
{
	char		query[512];
	PGresult   *res;
	bool		success = false;

	if (!wd_pgraft_is_enabled())
		return false;

	if (!wd_pgraft_is_leader())
	{
		ereport(WARNING,
				(errmsg("pgraft consensus: only leader can add nodes")));
		return false;
	}

	snprintf(query, sizeof(query),
			 "SELECT pgraft_add_node(%d, '%s', %d)",
			 node_id, address, port);

	res = PQexec(pgraft_conn, query);

	if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
	{
		char	   *val = PQgetvalue(res, 0, 0);

		success = (strcmp(val, "t") == 0);
	}

	PQclear(res);

	if (success)
	{
		ereport(LOG,
				(errmsg("pgraft consensus: added node %d (%s:%d)",
						node_id, address, port)));
	}

	return success;
}

/*
 * Remove node from cluster (must be leader)
 */
bool
wd_pgraft_remove_node(int node_id)
{
	char		query[256];
	PGresult   *res;
	bool		success = false;

	if (!wd_pgraft_is_enabled())
		return false;

	if (!wd_pgraft_is_leader())
	{
		ereport(WARNING,
				(errmsg("pgraft consensus: only leader can remove nodes")));
		return false;
	}

	snprintf(query, sizeof(query),
			 "SELECT pgraft_remove_node(%d)", node_id);

	res = PQexec(pgraft_conn, query);

	if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
	{
		char	   *val = PQgetvalue(res, 0, 0);

		success = (strcmp(val, "t") == 0);
	}

	PQclear(res);

	if (success)
	{
		ereport(LOG,
				(errmsg("pgraft consensus: removed node %d", node_id)));
	}

	return success;
}

/*
 * Check if cluster has quorum
 */
bool
wd_pgraft_has_quorum(void)
{
	PgRaftClusterStatus *status = wd_pgraft_get_cluster_status();
	bool		has_quorum = false;

	if (status)
	{
		has_quorum = status->has_quorum;
		wd_pgraft_free_cluster_status(status);
	}

	return has_quorum;
}

/*
 * Get quorum size (N/2 + 1)
 */
int
wd_pgraft_get_quorum_size(void)
{
	PgRaftClusterStatus *status = wd_pgraft_get_cluster_status();
	int			quorum_size = 0;

	if (status)
	{
		quorum_size = status->quorum_size;
		wd_pgraft_free_cluster_status(status);
	}

	return quorum_size;
}

/*
 * Check if cluster can make decisions (has quorum + has leader)
 */
bool
wd_pgraft_can_make_decisions(void)
{
	return wd_pgraft_has_quorum() && (wd_pgraft_get_leader_id() > 0);
}

/*
 * Replicate configuration via Raft
 */
bool
wd_pgraft_replicate_config(const char *key, const char *value)
{
	char		query[1024];
	PGresult   *res;
	bool		success = false;

	if (!wd_pgraft_is_enabled())
		return false;

	snprintf(query, sizeof(query),
			 "SELECT pgraft_kv_put('%s', '%s')", key, value);

	res = PQexec(pgraft_conn, query);

	if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
	{
		char	   *val = PQgetvalue(res, 0, 0);

		success = (strcmp(val, "t") == 0);
	}

	PQclear(res);
	return success;
}

/*
 * Get replicated configuration
 */
char *
wd_pgraft_get_replicated_config(const char *key)
{
	char		query[512];

	if (!wd_pgraft_is_enabled())
		return NULL;

	snprintf(query, sizeof(query),
			 "SELECT pgraft_kv_get('%s')", key);

	return execute_pgraft_query(query);
}

/*
 * Check if pgraft is healthy
 */
bool
wd_pgraft_is_healthy(void)
{
	char	   *state;
	bool		healthy = false;

	if (!wd_pgraft_is_enabled())
		return false;

	state = execute_pgraft_query("SELECT pgraft_get_worker_state()");

	if (state)
	{
		healthy = (strcmp(state, "RUNNING") == 0);
		pfree(state);
	}

	return healthy;
}

/*
 * Get current term
 */
int
wd_pgraft_get_term(void)
{
	char	   *term_str;
	int			term = 0;

	if (!wd_pgraft_is_enabled())
		return 0;

	term_str = execute_pgraft_query(
		"SELECT current_term FROM pgraft_get_cluster_status()");

	if (term_str)
	{
		term = atoi(term_str);
		pfree(term_str);
	}

	return term;
}

/*
 * Get current state (leader, follower, candidate)
 */
const char *
wd_pgraft_get_state(void)
{
	static char state[32] = "unknown";
	char	   *result;

	if (!wd_pgraft_is_enabled())
		return "disabled";

	result = execute_pgraft_query(
		"SELECT state FROM pgraft_get_cluster_status()");

	if (result)
	{
		strncpy(state, result, sizeof(state) - 1);
		pfree(result);
	}

	return state;
}

/*
 * Get consensus mode from configuration
 * consensus_mode: false (0) = heuristic, true (1) = pgraft
 */
ConsensusMode
wd_get_consensus_mode(void)
{
	if (!pool_config)
		return CONSENSUS_MODE_HEURISTIC;
	
	/* consensus_mode is now a boolean: false = heuristic, true = pgraft */
	return pool_config->consensus_mode ? CONSENSUS_MODE_PGRAFT : CONSENSUS_MODE_HEURISTIC;
}

/*
 * Convert consensus mode to string
 */
const char *
wd_consensus_mode_to_string(ConsensusMode mode)
{
	switch (mode)
	{
		case CONSENSUS_MODE_PGRAFT:
			return "pgraft";
		case CONSENSUS_MODE_HEURISTIC:
		default:
			return "heuristic";
	}
}

/*
 * Check if pgraft should be used
 */
bool
wd_should_use_pgraft(void)
{
	return (pool_config->use_watchdog && 
			wd_get_consensus_mode() == CONSENSUS_MODE_PGRAFT);
}

/*
 * Find which backend corresponds to the pgraft leader
 * Returns backend index (0-based), or -1 if not found
 *
 * This function queries each backend to determine which one has
 * the pgraft node_id matching the current leader_id
 */
int
wd_pgraft_find_leader_backend(void)
{
	int leader_id;
	int backend_id;
	char conninfo[1024];
	PGconn *temp_conn;
	PGresult *res;
	int result_backend = -1;

	leader_id = wd_pgraft_get_leader_id();
	if (leader_id <= 0)
	{
		ereport(DEBUG1,
				(errmsg("pgraft consensus: no valid leader_id (%d)", leader_id)));
		return -1;
	}

	ereport(DEBUG1,
			(errmsg("pgraft consensus: searching for backend with leader_id=%d", leader_id)));

	/* Query each backend to find which one has node_id matching leader_id */
	for (backend_id = 0; backend_id < NUM_BACKENDS; backend_id++)
	{
		if (!VALID_BACKEND(backend_id))
			continue;

		/* Connect to this backend */
		snprintf(conninfo, sizeof(conninfo),
				 "host=%s port=%d dbname=postgres user=%s connect_timeout=5",
				 BACKEND_INFO(backend_id).backend_hostname,
				 BACKEND_INFO(backend_id).backend_port,
				 pool_config->sr_check_user);

		temp_conn = PQconnectdb(conninfo);
		if (PQstatus(temp_conn) != CONNECTION_OK)
		{
			ereport(DEBUG1,
					(errmsg("pgraft consensus: could not connect to backend %d", backend_id)));
			PQfinish(temp_conn);
			continue;
		}

		/* Check this backend's pgraft node_id */
		res = PQexec(temp_conn, "SELECT node_id FROM pgraft_get_cluster_status()");

		if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
		{
			int node_id = atoi(PQgetvalue(res, 0, 0));

			ereport(DEBUG1,
					(errmsg("pgraft consensus: backend %d has node_id=%d", backend_id, node_id)));

			if (node_id == leader_id)
			{
				ereport(LOG,
						(errmsg("pgraft consensus: found leader at backend %d (pgraft node_id=%d)",
								backend_id, leader_id)));
				result_backend = backend_id;
				PQclear(res);
				PQfinish(temp_conn);
				break;
			}
		}

		PQclear(res);
		PQfinish(temp_conn);
	}

	if (result_backend < 0)
	{
		ereport(WARNING,
				(errmsg("pgraft consensus: could not find backend for leader_id=%d", leader_id)));
	}

	return result_backend;
}

