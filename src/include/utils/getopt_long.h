/*-------------------------------------------------------------------------
 *
 * getopt_long.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef GETOPT_LONG_H
#define GETOPT_LONG_H

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

/* These are picked up from the system's getopt() facility. */
extern int	opterr;
extern int	optind;
extern int	optopt;
extern char *optarg;
extern int	optreset;

#ifndef HAVE_STRUCT_OPTION

struct option
{
	const char *name;
	int			has_arg;
	int		   *flag;
	int			val;
};

#define no_argument 0
#define required_argument 1
#endif

#ifndef HAVE_GETOPT_LONG

extern int	getopt_long(int argc, char *const argv[],
						const char *optstring,
						const struct option *longopts, int *longindex);
#endif

#endif							/* GETOPT_LONG_H */
