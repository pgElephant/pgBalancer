/*-------------------------------------------------------------------------
 *
 * pool_ai_load_balancer.h
 *      AI-based Load Balancing for PostgreSQL Connection Pooler
 *
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 * IDENTIFICATION
 *      src/include/ai/pool_ai_load_balancer.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef POOL_AI_LOAD_BALANCER_H
#define POOL_AI_LOAD_BALANCER_H

#include "pool.h"
#include <sys/time.h>

/* AI Load Balancer Configuration */
typedef enum
{
	AI_LB_MODE_DISABLED = 0,
	AI_LB_MODE_ADAPTIVE = 1,      /* Adaptive based on response time */
	AI_LB_MODE_PREDICTIVE = 2,    /* Predictive using historical patterns */
	AI_LB_MODE_HYBRID = 3         /* Hybrid: Traditional + AI */
} AILoadBalancerMode;

/* Node Performance Metrics */
typedef struct
{
	int			node_id;
	double		avg_response_time;     /* Average response time in ms */
	double		current_load;          /* Current load (0.0 to 1.0) */
	long		total_queries;         /* Total queries processed */
	long		successful_queries;    /* Successfully completed queries */
	long		failed_queries;        /* Failed queries */
	double		error_rate;            /* Error rate (0.0 to 1.0) */
	struct timeval last_update;    /* Last metrics update timestamp */
	double		predicted_load;        /* Predicted load (AI prediction) */
	double		health_score;          /* Overall health score (0.0 to 1.0) */
} AINodeMetrics;

/* Query Pattern */
typedef struct
{
	char		query_type[32];       /* SELECT, INSERT, UPDATE, DELETE */
	int			estimated_complexity; /* 0-100 scale */
	int			estimated_rows;       /* Estimated rows to process */
	bool		is_read_only;         /* Read-only query flag */
	double		predicted_time;       /* Predicted execution time (ms) */
} QueryPattern;

/* AI Model State */
typedef struct
{
	AILoadBalancerMode mode;
	AINodeMetrics *node_metrics;   /* Array of node metrics */
	int			num_nodes;
	double		learning_rate;         /* Learning rate for adjustments */
	double		exploration_rate;      /* Exploration vs exploitation */
	long		total_decisions;       /* Total routing decisions made */
	long		successful_decisions;  /* Successful routing decisions */
	struct timeval model_start_time;
} AIModelState;

/* Global AI state */
extern AIModelState *ai_model_state;

/* AI Load Balancer Functions */

/*
 * Initialize the AI load balancer
 */
extern void pool_ai_lb_initialize(int num_nodes, AILoadBalancerMode mode);

/*
 * Shutdown and cleanup AI load balancer
 */
extern void pool_ai_lb_shutdown(void);

/*
 * Select the best backend node using AI algorithm
 * Returns: backend node ID
 */
extern int pool_ai_select_backend(QueryPattern *pattern, int *available_nodes, 
                                   int num_available);

/*
 * Update metrics after query execution
 */
extern void pool_ai_update_metrics(int node_id, double response_time, 
                                    bool success);

/*
 * Get current node health score
 */
extern double pool_ai_get_node_health(int node_id);

/*
 * Predict query execution time
 */
extern double pool_ai_predict_query_time(int node_id, QueryPattern *pattern);

/*
 * Adaptive learning: Update model based on feedback
 */
extern void pool_ai_learn_from_feedback(int node_id, QueryPattern *pattern,
                                         double actual_time, bool success);

/*
 * Get AI model statistics
 */
extern void pool_ai_get_statistics(char *buf, int bufsize);

/*
 * Reset AI model (for debugging/testing)
 */
extern void pool_ai_reset_model(void);

/*
 * Check if AI load balancing is enabled
 */
extern bool pool_ai_is_enabled(void);

/*
 * Set AI mode dynamically
 */
extern void pool_ai_set_mode(AILoadBalancerMode mode);

/*
 * Analyze query to extract pattern
 */
extern void pool_ai_analyze_query(const char *query, QueryPattern *pattern);

#endif   /* POOL_AI_LOAD_BALANCER_H */

