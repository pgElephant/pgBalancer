/*-------------------------------------------------------------------------
 *
 * pgpool_adm.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PGPOOL_ADM_H
#define PGPOOL_ADM_H

PG_MODULE_MAGIC;


Datum		_pcp_node_info(PG_FUNCTION_ARGS);
Datum		_pcp_health_check_stats(PG_FUNCTION_ARGS);
Datum		_pcp_pool_status(PG_FUNCTION_ARGS);
Datum		_pcp_node_count(PG_FUNCTION_ARGS);
Datum		_pcp_attach_node(PG_FUNCTION_ARGS);
Datum		_pcp_detach_node(PG_FUNCTION_ARGS);
Datum		_pcp_proc_info(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_pcp_node_info);
PG_FUNCTION_INFO_V1(_pcp_health_check_stats);
PG_FUNCTION_INFO_V1(_pcp_pool_status);
PG_FUNCTION_INFO_V1(_pcp_node_count);
PG_FUNCTION_INFO_V1(_pcp_attach_node);
PG_FUNCTION_INFO_V1(_pcp_detach_node);
PG_FUNCTION_INFO_V1(_pcp_proc_info);

#endif
