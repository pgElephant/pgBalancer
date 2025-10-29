/*-------------------------------------------------------------------------
 *
 * pool_params.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef pool_params_h
#define pool_params_h

typedef struct
{
	int			num;			/* number of entries */
	char	  **names;			/* parameter names */
	char	  **values;			/* values */
} ParamStatus;

extern int	pool_init_params(ParamStatus *params);
extern void pool_discard_params(ParamStatus *params);
extern char *pool_find_name(ParamStatus *params, char *name, int *pos);
extern int	pool_get_param(ParamStatus *params, int index, char **name, char **value);
extern int	pool_add_param(ParamStatus *params, char *name, char *value);
extern void pool_param_debug_print(ParamStatus *params);


#endif							/* pool_params_h */
