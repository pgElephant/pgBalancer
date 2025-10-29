/*-------------------------------------------------------------------------
 *
 * health_check.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef health_check_h
#define health_check_h

/*
 * Health check statistics per node
*/
typedef struct
{
	uint64		total_count;	/* total count of health check */
	uint64		success_count;	/* total count of successful health check */
	uint64		fail_count;		/* total count of failed health check */
	uint64		skip_count;		/* total count of skipped health check */
	uint64		retry_count;	/* total count of health check retries */
	uint32		max_retry_count;	/* max retry count in a health check
									 * session */
	uint64		total_health_check_duration;	/* sum of health check
												 * duration */
	int32		max_health_check_duration;	/* maximum duration spent for a
											 * health check session in milli
											 * seconds */
	int32		min_health_check_duration;	/* minimum duration spent for a
											 * health check session in milli
											 * seconds */
	time_t		last_health_check;	/* last health check timestamp */
	time_t		last_successful_health_check;	/* last successful health
												 * check timestamp */
	time_t		last_skip_health_check; /* last skipped health check timestamp */
	time_t		last_failed_health_check;	/* last failed health check
											 * timestamp */
} POOL_HEALTH_CHECK_STATISTICS;

extern volatile POOL_HEALTH_CHECK_STATISTICS *health_check_stats;	/* health check stats
																	 * area in shared memory */

extern void do_health_check_child(void *params);
extern size_t health_check_stats_shared_memory_size(void);
extern void health_check_stats_init(POOL_HEALTH_CHECK_STATISTICS *addr);

#endif							/* health_check_h */
