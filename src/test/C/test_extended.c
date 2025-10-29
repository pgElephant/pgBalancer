/*-------------------------------------------------------------------------
 *
 * test_extended.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libpq-fe.h"
#include "libpq/libpq-fs.h"

int
main(int argc, char **argv)
{
	char	   *connect_string = "user=t-ishii dbname=test port=5432";
	int			doTrace = 1;	/* set 1 to start libpq trace */
	int			doTransaction = 1;	/* set 1 to start explicit transaction */
	int			doSleep = 0;	/* set non 0 seconds to sleep after connection
								 * and before starting command */

	/* SQL commands to be executed */
	static char *commands[] = {
		"SAVEPOINT S1",
		"UPDATE t1 SET k = 1",
		"ROLLBACK TO S1",
		"SELECT 1",
		"RELEASE SAVEPOINT S1"
	};

	PGconn	   *conn;
	PGresult   *res;
	int			i;

	conn = PQconnectdb(connect_string);
	if (PQstatus(conn) == CONNECTION_BAD)
	{
		printf("Unable to connect to db\n");
		PQfinish(conn);
		return 1;
	}

	if (doSleep)
		sleep(doSleep);

	if (doTrace == 1)
		PQtrace(conn, stdout);

	if (doTransaction)
		PQexec(conn, "BEGIN;");

	for (i = 0; i < sizeof(commands) / sizeof(char *); i++)
	{
		char	   *command = commands[i];

		res = PQexecParams(conn, command, 0, NULL, NULL, NULL, NULL, 0);
		switch (PQresultStatus(res))
		{
			case PGRES_COMMAND_OK:
			case PGRES_TUPLES_OK:
				fprintf(stderr, "\"%s\" : succeeded\n", command);
				break;
			default:
				fprintf(stderr, "\"%s\" failed: %s\n", command, PQresultErrorMessage(res));
				break;
		}
		PQclear(res);

	}

	if (doTransaction == 1)
	{
		PQexec(conn, "COMMIT;");
	}

	PQfinish(conn);
	return 0;
}
