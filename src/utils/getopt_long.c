/*-------------------------------------------------------------------------
 *
 * getopt_long.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include "utils/getopt_long.h"

#ifndef HAVE_INT_OPTRESET
int			optreset;

/* else the "extern" was provided by getopt_long.h */
#endif

#define BADCH	'?'
#define BADARG	':'
#define EMSG	""


int
getopt_long(int argc, char *const argv[],
			const char *optstring,
			const struct option *longopts, int *longindex)
{
	static char *place = EMSG;	/* option letter processing */
	char	   *oli;			/* option letter list index */

	if (optreset || !*place)
	{							/* update scanning pointer */
		optreset = 0;

		if (optind >= argc)
		{
			place = EMSG;
			return -1;
		}

		place = argv[optind];

		if (place[0] != '-')
		{
			place = EMSG;
			return -1;
		}

		place++;

		if (place[0] && place[0] == '-' && place[1] == '\0')
		{						/* found "--" */
			++optind;
			place = EMSG;
			return -1;
		}

		if (place[0] && place[0] == '-' && place[1])
		{
			/* long option */
			size_t		namelen;
			int			i;

			place++;

			namelen = strcspn(place, "=");
			for (i = 0; longopts[i].name != NULL; i++)
			{
				if (strlen(longopts[i].name) == namelen
					&& strncmp(place, longopts[i].name, namelen) == 0)
				{
					if (longopts[i].has_arg)
					{
						if (place[namelen] == '=')
							optarg = place + namelen + 1;
						else if (optind < argc - 1)
						{
							optind++;
							optarg = argv[optind];
						}
						else
						{
							if (optstring[0] == ':')
								return BADARG;
							if (opterr)
								fprintf(stderr,
										"%s: option requires an argument -- %s\n",
										argv[0], place);
							place = EMSG;
							optind++;
							return BADCH;
						}
					}
					else
					{
						optarg = NULL;
						if (place[namelen] != 0)
						{
							/* XXX error? */
						}
					}

					optind++;

					if (longindex)
						*longindex = i;

					place = EMSG;

					if (longopts[i].flag == NULL)
						return longopts[i].val;
					else
					{
						*longopts[i].flag = longopts[i].val;
						return 0;
					}
				}
			}

			if (opterr && optstring[0] != ':')
				fprintf(stderr,
						"%s: illegal option -- %s\n", argv[0], place);
			place = EMSG;
			optind++;
			return BADCH;
		}
	}

	/* short option */
	optopt = (int) *place++;

	oli = strchr(optstring, optopt);
	if (!oli)
	{
		if (!*place)
			++optind;
		if (opterr && *optstring != ':')
			fprintf(stderr,
					"%s: illegal option -- %c\n", argv[0], optopt);
		return BADCH;
	}

	if (oli[1] != ':')
	{							/* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else
	{							/* need an argument */
		if (*place)				/* no white space */
			optarg = place;
		else if (argc <= ++optind)
		{						/* no arg */
			place = EMSG;
			if (*optstring == ':')
				return BADARG;
			if (opterr)
				fprintf(stderr,
						"%s: option requires an argument -- %c\n",
						argv[0], optopt);
			return BADCH;
		}
		else
			/* white space */
			optarg = argv[optind];
		place = EMSG;
		++optind;
	}
	return optopt;
}
