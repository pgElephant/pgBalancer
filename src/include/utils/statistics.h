/*-------------------------------------------------------------------------
 *
 * statistics.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef statistics_h
#define statistics_h

extern size_t stat_shared_memory_size(void);
extern void stat_set_stat_area(void *address);
extern void stat_init_stat_area(void);
extern void stat_count_up(int backend_node_id, Node *parsetree);
extern void error_stat_count_up(int backend_node_id, char *str);
extern uint64 stat_get_select_count(int backend_node_id);
extern uint64 stat_get_insert_count(int backend_node_id);
extern uint64 stat_get_update_count(int backend_node_id);
extern uint64 stat_get_delete_count(int backend_node_id);
extern uint64 stat_get_ddl_count(int backend_node_id);
extern uint64 stat_get_other_count(int backend_node_id);
extern uint64 stat_get_panic_count(int backend_node_id);
extern uint64 stat_get_fatal_count(int backend_node_id);
extern uint64 stat_get_error_count(int backend_node_id);

#endif							/* statistics_h */
