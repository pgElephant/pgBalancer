/*-------------------------------------------------------------------------
 *
 * wd_commands.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef WD_COMMANDS_H
#define WD_COMMANDS_H

#include "watchdog/wd_ipc_defines.h"
#include "watchdog/wd_ipc_conn.h"

typedef struct WDNodeInfo
{
	int			state;
	int			membership_status;
	char		membership_status_string[WD_MAX_HOST_NAMELEN];
	char		nodeName[WD_MAX_HOST_NAMELEN];
	char		hostName[WD_MAX_HOST_NAMELEN];	/* host name */
	char		stateName[WD_MAX_HOST_NAMELEN]; /* watchdog state name */
	int			wd_port;		/* watchdog port */
	int			pgpool_port;	/* pgpool port */
	int			wd_priority;	/* node priority */
	char		delegate_ip[WD_MAX_HOST_NAMELEN];	/* delegate IP */
	int			id;
} WDNodeInfo;

typedef struct WDGenericData
{
	WDValueDataType valueType;
	union data
	{
		char	   *stringVal;
		int			intVal;
		bool		boolVal;
		long		longVal;
	}			data;
} WDGenericData;



extern WDGenericData *get_wd_runtime_variable_value(char *wd_authkey, char *varName);
extern WD_STATES get_watchdog_local_node_state(char *wd_authkey);
extern int	get_watchdog_quorum_state(char *wd_authkey);
extern char *wd_get_watchdog_nodes_json(char *wd_authkey, int nodeID);
extern void set_wd_command_timeout(int sec);
extern char *get_request_json(char *key, char *value, char *authKey);
extern WDNodeInfo *parse_watchdog_node_info_from_wd_node_json(json_value *source);
#endif							/* WD_COMMANDS_H */
