/*-------------------------------------------------------------------------
 *
 * pool_ai_load_balancer.c
 *      AI-based Load Balancing Implementation
 *
 * This module implements an AI-based load balancing algorithm that:
 * - Learns from query execution patterns
 * - Predicts backend performance
 * - Adapts routing decisions based on feedback
 * - Optimizes for response time and load distribution
 *
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 * IDENTIFICATION
 *      src/ai/pool_ai_load_balancer.c
 *
 *-------------------------------------------------------------------------
 */

#include "pool.h"
#include "pool_config.h"
#include "ai/pool_ai_load_balancer.h"
#include "utils/elog.h"
#include "utils/palloc.h"
#include "utils/memutils.h"

#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>

/* Global AI model state */
AIModelState *ai_model_state = NULL;

/* Private function prototypes */
static double calculate_node_score(AINodeMetrics *metrics, QueryPattern *pattern);
static double get_time_diff_ms(struct timeval *start, struct timeval *end);
static void update_node_averages(AINodeMetrics *metrics);
static int weighted_random_selection(int *nodes, double *weights, int count);
static void decay_metrics(AINodeMetrics *metrics);

/*
 * Initialize the AI load balancer
 */
void
pool_ai_lb_initialize(int num_nodes, AILoadBalancerMode mode)
{
	int i;
	
	elog(LOG, "AI Load Balancer: Initializing with %d nodes, mode=%d", 
	     num_nodes, mode);

	/* Allocate memory for AI model state */
	ai_model_state = (AIModelState *)palloc0(sizeof(AIModelState));
	
	if (!ai_model_state)
	{
		elog(ERROR, "AI Load Balancer: Failed to allocate memory for model state");
		return;
	}

	ai_model_state->mode = mode;
	ai_model_state->num_nodes = num_nodes;
	ai_model_state->learning_rate = 0.1;      /* 10% learning rate */
	ai_model_state->exploration_rate = 0.2;   /* 20% exploration */
	ai_model_state->total_decisions = 0;
	ai_model_state->successful_decisions = 0;

	gettimeofday(&ai_model_state->model_start_time, NULL);

	/* Allocate and initialize node metrics */
	ai_model_state->node_metrics = (AINodeMetrics *)palloc0(
		sizeof(AINodeMetrics) * num_nodes);
	
	if (!ai_model_state->node_metrics)
	{
		pfree(ai_model_state);
		ai_model_state = NULL;
		elog(ERROR, "AI Load Balancer: Failed to allocate memory for node metrics");
		return;
	}

	/* Initialize each node's metrics */
	for (i = 0; i < num_nodes; i++)
	{
		ai_model_state->node_metrics[i].node_id = i;
		ai_model_state->node_metrics[i].avg_response_time = 0.0;
		ai_model_state->node_metrics[i].current_load = 0.0;
		ai_model_state->node_metrics[i].total_queries = 0;
		ai_model_state->node_metrics[i].successful_queries = 0;
		ai_model_state->node_metrics[i].failed_queries = 0;
		ai_model_state->node_metrics[i].error_rate = 0.0;
		ai_model_state->node_metrics[i].predicted_load = 0.0;
		ai_model_state->node_metrics[i].health_score = 1.0; /* Start with perfect health */
		gettimeofday(&ai_model_state->node_metrics[i].last_update, NULL);
	}

	elog(LOG, "AI Load Balancer: Initialized successfully");
}

/*
 * Shutdown and cleanup AI load balancer
 */
void
pool_ai_lb_shutdown(void)
{
	if (ai_model_state)
	{
		elog(LOG, "AI Load Balancer: Shutting down (total decisions: %ld, success rate: %.2f%%)",
		     ai_model_state->total_decisions,
		     ai_model_state->total_decisions > 0 ? 
		     (100.0 * ai_model_state->successful_decisions / ai_model_state->total_decisions) : 0.0);

		if (ai_model_state->node_metrics)
			pfree(ai_model_state->node_metrics);
		
		pfree(ai_model_state);
		ai_model_state = NULL;
	}
}

/*
 * Select the best backend node using AI algorithm
 */
int
pool_ai_select_backend(QueryPattern *pattern, int *available_nodes, int num_available)
{
	int i, selected_node;
	double *scores;
	double max_score;
	int best_node;
	bool use_exploration;

	if (!ai_model_state || num_available <= 0)
		return available_nodes[0];  /* Fallback to first available */

	/* Exploration vs Exploitation strategy */
	use_exploration = ((double)rand() / RAND_MAX) < ai_model_state->exploration_rate;

	scores = (double *)palloc(sizeof(double) * num_available);

	/* Calculate score for each available node */
	for (i = 0; i < num_available; i++)
	{
		int node_id = available_nodes[i];
		AINodeMetrics *metrics = &ai_model_state->node_metrics[node_id];
		
		/* Decay old metrics */
		decay_metrics(metrics);
		
		/* Calculate node score based on current mode */
		scores[i] = calculate_node_score(metrics, pattern);
		
		elog(DEBUG2, "AI LB: Node %d score=%.3f (health=%.2f, load=%.2f, rt=%.2f ms)",
		     node_id, scores[i], metrics->health_score, 
		     metrics->current_load, metrics->avg_response_time);
	}

	if (use_exploration)
	{
		/* Exploration: Random weighted selection */
		selected_node = weighted_random_selection(available_nodes, scores, num_available);
		elog(DEBUG2, "AI LB: Using exploration, selected node %d", selected_node);
	}
	else
	{
		/* Exploitation: Select best scoring node */
		max_score = scores[0];
		best_node = 0;
		
		for (i = 1; i < num_available; i++)
		{
			if (scores[i] > max_score)
			{
				max_score = scores[i];
				best_node = i;
			}
		}
		
		selected_node = available_nodes[best_node];
		elog(DEBUG2, "AI LB: Using exploitation, selected node %d (score=%.3f)", 
		     selected_node, max_score);
	}

	pfree(scores);
	ai_model_state->total_decisions++;

	return selected_node;
}

/*
 * Calculate node score based on multiple factors
 */
static double
calculate_node_score(AINodeMetrics *metrics, QueryPattern *pattern)
{
	double score = 0.0;
	double health_weight = 0.4;
	double load_weight = 0.3;
	double response_time_weight = 0.3;

	/* Health score (higher is better) */
	score += metrics->health_score * health_weight;

	/* Load score (lower load is better) */
	score += (1.0 - metrics->current_load) * load_weight;

	/* Response time score (faster is better) */
	if (metrics->avg_response_time > 0)
	{
		/* Normalize response time to 0-1 scale (assuming 1000ms is very slow) */
		double rt_score = 1.0 - fmin(metrics->avg_response_time / 1000.0, 1.0);
		score += rt_score * response_time_weight;
	}
	else
	{
		/* No history, give neutral score */
		score += 0.5 * response_time_weight;
	}

	/* Adjust for AI mode */
	if (ai_model_state->mode == AI_LB_MODE_PREDICTIVE && pattern)
	{
		/* Use predicted execution time for this query pattern */
		double predicted_time = pool_ai_predict_query_time(metrics->node_id, pattern);
		if (predicted_time > 0)
		{
			double pred_score = 1.0 - fmin(predicted_time / 1000.0, 1.0);
			score = (score * 0.7) + (pred_score * 0.3); /* Blend with prediction */
		}
	}

	return score;
}

/*
 * Update metrics after query execution
 */
void
pool_ai_update_metrics(int node_id, double response_time, bool success)
{
	AINodeMetrics *metrics;
	double alpha;

	if (!ai_model_state || node_id < 0 || node_id >= ai_model_state->num_nodes)
		return;

	metrics = &ai_model_state->node_metrics[node_id];
	alpha = ai_model_state->learning_rate;

	/* Update counters */
	metrics->total_queries++;
	if (success)
	{
		metrics->successful_queries++;
		ai_model_state->successful_decisions++;
	}
	else
	{
		metrics->failed_queries++;
	}

	/* Update average response time using exponential moving average */
	if (metrics->avg_response_time == 0.0)
		metrics->avg_response_time = response_time;
	else
		metrics->avg_response_time = (alpha * response_time) + 
		                             ((1.0 - alpha) * metrics->avg_response_time);

	/* Update error rate */
	metrics->error_rate = (double)metrics->failed_queries / 
	                      (double)metrics->total_queries;

	/* Update health score (based on success rate and response time) */
	double success_rate = (double)metrics->successful_queries / 
	                      (double)metrics->total_queries;
	double rt_health = metrics->avg_response_time < 100.0 ? 1.0 : 
	                   (metrics->avg_response_time < 500.0 ? 0.8 : 0.5);
	
	metrics->health_score = (success_rate * 0.6) + (rt_health * 0.4);

	/* Update load estimate (simple moving average) */
	double current_load_sample = response_time > 100.0 ? 0.7 : 
	                             response_time > 50.0 ? 0.5 : 0.3;
	metrics->current_load = (alpha * current_load_sample) + 
	                       ((1.0 - alpha) * metrics->current_load);

	gettimeofday(&metrics->last_update, NULL);

	elog(DEBUG3, "AI LB: Updated node %d metrics - RT: %.2f ms, Health: %.2f, Load: %.2f",
	     node_id, metrics->avg_response_time, metrics->health_score, 
	     metrics->current_load);
}

/*
 * Get current node health score
 */
double
pool_ai_get_node_health(int node_id)
{
	if (!ai_model_state || node_id < 0 || node_id >= ai_model_state->num_nodes)
		return 0.5;  /* Default neutral health */

	return ai_model_state->node_metrics[node_id].health_score;
}

/*
 * Predict query execution time
 */
double
pool_ai_predict_query_time(int node_id, QueryPattern *pattern)
{
	AINodeMetrics *metrics;
	double base_time;
	double complexity_factor;

	if (!ai_model_state || !pattern || 
	    node_id < 0 || node_id >= ai_model_state->num_nodes)
		return 0.0;

	metrics = &ai_model_state->node_metrics[node_id];
	
	/* Base prediction on historical average */
	base_time = metrics->avg_response_time > 0 ? 
	            metrics->avg_response_time : 50.0;  /* Default 50ms */

	/* Adjust based on query complexity */
	complexity_factor = 1.0 + (pattern->estimated_complexity / 200.0);

	/* Adjust based on estimated rows */
	if (pattern->estimated_rows > 1000)
		complexity_factor *= 1.5;
	else if (pattern->estimated_rows > 100)
		complexity_factor *= 1.2;

	/* Read-only queries are typically faster */
	if (pattern->is_read_only)
		complexity_factor *= 0.8;

	return base_time * complexity_factor;
}

/*
 * Adaptive learning: Update model based on feedback
 */
void
pool_ai_learn_from_feedback(int node_id, QueryPattern *pattern,
                             double actual_time, bool success)
{
	AINodeMetrics *metrics;
	double predicted_time;
	double prediction_error;

	if (!ai_model_state || !pattern ||
	    node_id < 0 || node_id >= ai_model_state->num_nodes)
		return;

	metrics = &ai_model_state->node_metrics[node_id];
	
	/* Calculate prediction error */
	predicted_time = pool_ai_predict_query_time(node_id, pattern);
	prediction_error = fabs(actual_time - predicted_time);

	/* Update metrics */
	pool_ai_update_metrics(node_id, actual_time, success);

	/* Adjust learning rate based on prediction accuracy */
	if (prediction_error > 100.0)  /* Poor prediction */
		ai_model_state->learning_rate = fmin(0.2, ai_model_state->learning_rate * 1.1);
	else if (prediction_error < 10.0)  /* Good prediction */
		ai_model_state->learning_rate = fmax(0.05, ai_model_state->learning_rate * 0.95);

	elog(DEBUG3, "AI LB: Learning feedback - Node %d, Predicted: %.2f ms, Actual: %.2f ms, Error: %.2f ms",
	     node_id, predicted_time, actual_time, prediction_error);
}

/*
 * Get AI model statistics
 */
void
pool_ai_get_statistics(char *buf, int bufsize)
{
	int i, len = 0;
	double uptime_sec;
	struct timeval now;

	if (!ai_model_state || !buf || bufsize <= 0)
		return;

	gettimeofday(&now, NULL);
	uptime_sec = get_time_diff_ms(&ai_model_state->model_start_time, &now) / 1000.0;

	len += snprintf(buf + len, bufsize - len,
	                "AI Load Balancer Statistics\n");
	len += snprintf(buf + len, bufsize - len,
	                "Mode: %d, Uptime: %.1f sec\n",
	                ai_model_state->mode, uptime_sec);
	len += snprintf(buf + len, bufsize - len,
	                "Total Decisions: %ld, Success Rate: %.2f%%\n",
	                ai_model_state->total_decisions,
	                ai_model_state->total_decisions > 0 ?
	                (100.0 * ai_model_state->successful_decisions / 
	                 ai_model_state->total_decisions) : 0.0);
	len += snprintf(buf + len, bufsize - len,
	                "Learning Rate: %.3f, Exploration Rate: %.3f\n\n",
	                ai_model_state->learning_rate, 
	                ai_model_state->exploration_rate);

	/* Per-node statistics */
	for (i = 0; i < ai_model_state->num_nodes && len < bufsize - 100; i++)
	{
		AINodeMetrics *m = &ai_model_state->node_metrics[i];
		len += snprintf(buf + len, bufsize - len,
		                "Node %d: Health=%.2f, Load=%.2f, AvgRT=%.1fms, "
		                "Queries=%ld, Errors=%ld (%.1f%%)\n",
		                m->node_id, m->health_score, m->current_load,
		                m->avg_response_time, m->total_queries, 
		                m->failed_queries,
		                m->total_queries > 0 ? 
		                (100.0 * m->failed_queries / m->total_queries) : 0.0);
	}
}

/*
 * Reset AI model
 */
void
pool_ai_reset_model(void)
{
	int i;

	if (!ai_model_state)
		return;

	elog(LOG, "AI Load Balancer: Resetting model");

	ai_model_state->total_decisions = 0;
	ai_model_state->successful_decisions = 0;
	gettimeofday(&ai_model_state->model_start_time, NULL);

	for (i = 0; i < ai_model_state->num_nodes; i++)
	{
		ai_model_state->node_metrics[i].avg_response_time = 0.0;
		ai_model_state->node_metrics[i].current_load = 0.0;
		ai_model_state->node_metrics[i].total_queries = 0;
		ai_model_state->node_metrics[i].successful_queries = 0;
		ai_model_state->node_metrics[i].failed_queries = 0;
		ai_model_state->node_metrics[i].error_rate = 0.0;
		ai_model_state->node_metrics[i].health_score = 1.0;
	}
}

/*
 * Check if AI load balancing is enabled
 */
bool
pool_ai_is_enabled(void)
{
	return (ai_model_state != NULL && 
	        ai_model_state->mode != AI_LB_MODE_DISABLED);
}

/*
 * Set AI mode dynamically
 */
void
pool_ai_set_mode(AILoadBalancerMode mode)
{
	if (ai_model_state)
	{
		elog(LOG, "AI Load Balancer: Changing mode from %d to %d",
		     ai_model_state->mode, mode);
		ai_model_state->mode = mode;
	}
}

/*
 * Analyze query to extract pattern
 */
void
pool_ai_analyze_query(const char *query, QueryPattern *pattern)
{
	if (!query || !pattern)
		return;

	/* Initialize pattern */
	memset(pattern, 0, sizeof(QueryPattern));
	pattern->is_read_only = false;
	pattern->estimated_complexity = 50;  /* Default medium complexity */
	pattern->estimated_rows = 100;       /* Default estimate */

	/* Simple query type detection */
	if (strncasecmp(query, "SELECT", 6) == 0)
	{
		strcpy(pattern->query_type, "SELECT");
		pattern->is_read_only = true;
		
		/* Estimate complexity based on query keywords */
		if (strcasestr(query, "JOIN"))
			pattern->estimated_complexity += 20;
		if (strcasestr(query, "GROUP BY"))
			pattern->estimated_complexity += 15;
		if (strcasestr(query, "ORDER BY"))
			pattern->estimated_complexity += 10;
		if (strcasestr(query, "DISTINCT"))
			pattern->estimated_complexity += 10;
	}
	else if (strncasecmp(query, "INSERT", 6) == 0)
	{
		strcpy(pattern->query_type, "INSERT");
		pattern->estimated_complexity = 30;
	}
	else if (strncasecmp(query, "UPDATE", 6) == 0)
	{
		strcpy(pattern->query_type, "UPDATE");
		pattern->estimated_complexity = 40;
	}
	else if (strncasecmp(query, "DELETE", 6) == 0)
	{
		strcpy(pattern->query_type, "DELETE");
		pattern->estimated_complexity = 35;
	}
	else
	{
		strcpy(pattern->query_type, "OTHER");
		pattern->estimated_complexity = 50;
	}

	/* Predict execution time based on pattern */
	pattern->predicted_time = pattern->estimated_complexity * 
	                         (pattern->estimated_rows / 100.0);
}

/*
 * Helper: Get time difference in milliseconds
 */
static double
get_time_diff_ms(struct timeval *start, struct timeval *end)
{
	double ms = (end->tv_sec - start->tv_sec) * 1000.0;
	ms += (end->tv_usec - start->tv_usec) / 1000.0;
	return ms;
}

/*
 * Helper: Decay old metrics to give more weight to recent performance
 */
static void
decay_metrics(AINodeMetrics *metrics)
{
	struct timeval now;
	double time_since_update;
	double decay_factor;

	gettimeofday(&now, NULL);
	time_since_update = get_time_diff_ms(&metrics->last_update, &now);

	/* Decay if more than 60 seconds since last update */
	if (time_since_update > 60000.0)
	{
		decay_factor = 0.9;  /* 10% decay */
		metrics->current_load *= decay_factor;
		/* Gradually restore health if no recent failures */
		if (metrics->health_score < 1.0)
			metrics->health_score = fmin(1.0, metrics->health_score + 0.05);
	}
}

/*
 * Helper: Weighted random selection
 */
static int
weighted_random_selection(int *nodes, double *weights, int count)
{
	double total_weight = 0.0;
	double random_value;
	double cumulative = 0.0;
	int i;

	/* Calculate total weight */
	for (i = 0; i < count; i++)
		total_weight += weights[i];

	if (total_weight == 0.0)
		return nodes[rand() % count];  /* Uniform random if all weights are 0 */

	/* Select based on weight */
	random_value = ((double)rand() / RAND_MAX) * total_weight;

	for (i = 0; i < count; i++)
	{
		cumulative += weights[i];
		if (random_value <= cumulative)
			return nodes[i];
	}

	return nodes[count - 1];  /* Fallback to last node */
}

