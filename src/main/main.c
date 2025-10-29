/*-------------------------------------------------------------------------
 *
 * main.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "pool.h"
#include "pool_config.h"
#include "version.h"
#include "pool_config_variables.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "utils/getopt_long.h"
#endif

#include <errno.h>
#include <string.h>
#include <libgen.h>

#include "utils/elog.h"
#include "utils/palloc.h"
#include "utils/memutils.h"
#include "utils/pool_path.h"
#include "utils/pool_signal.h"
#include "utils/pool_ipc.h"
#include "utils/ps_status.h"
#include "utils/pool_ssl.h"

#include "auth/pool_passwd.h"
#include "auth/pool_hba.h"
#include "query_cache/pool_memqcache.h"
#include "watchdog/wd_utils.h"


static bool get_pool_key_filename(char *poolKeyFile);

static void daemonize(void);
static char *get_pid_file_path(void);
static int	read_pid_file(void);
static void write_pid_file(void);
static void usage(void);
static void show_version(void);
static void show_config_table(void);
static void stop_me(void);
static void FileUnlink(int code, Datum path);

char	   *pcp_conf_file = NULL;	/* absolute path of the pcp.conf */
char	   *conf_file = NULL;	/* absolute path of the pgbalancer.conf */
char	   *hba_file = NULL;	/* absolute path of the hba.conf */
char	   *base_dir = NULL;	/* The working dir from where pgpool was
								 * invoked from */

static int	not_detach = 0;		/* non 0 if non detach option (-n) is given */
int			stop_sig = SIGTERM; /* stopping signal default value */
int			myargc;
char	  **myargv;
int			assert_enabled = 0;
char	   *pool_key = NULL;

int
main(int argc, char **argv)
{
	int			opt;
	int			debug_level = 0;
	int			optindex;
	bool		discard_status = false;
	bool		clear_memcache_oidmaps = false;
	bool		show_config = false;

	char		pcp_conf_file_path[POOLMAXPATHLEN + 1];
	char		conf_file_path[POOLMAXPATHLEN + 1];
	char		hba_file_path[POOLMAXPATHLEN + 1];
	char		pool_passwd_key_file_path[POOLMAXPATHLEN + 1 + sizeof(POOLKEYFILE) + 1];

	static struct option long_options[] = {
		{"hba-file", required_argument, NULL, 'a'},
		{"debug", no_argument, NULL, 'd'},
		{"config-file", required_argument, NULL, 'f'},
		{"key-file", required_argument, NULL, 'k'},
		{"pcp-file", required_argument, NULL, 'F'},
		{"help", no_argument, NULL, 'h'},
		{"mode", required_argument, NULL, 'm'},
		{"dont-detach", no_argument, NULL, 'n'},
		{"discard-status", no_argument, NULL, 'D'},
		{"clear-oidmaps", no_argument, NULL, 'C'},
		{"debug-assertions", no_argument, NULL, 'x'},
		{"version", no_argument, NULL, 'v'},
		{"show-config", no_argument, NULL, 'S'},
		{NULL, 0, NULL, 0}
	};

	myargc = argc;
	myargv = argv;

	snprintf(conf_file_path, sizeof(conf_file_path), "%s/%s", DEFAULT_CONFIGDIR, POOL_CONF_FILE_NAME);
	snprintf(pcp_conf_file_path, sizeof(pcp_conf_file_path), "%s/%s", DEFAULT_CONFIGDIR, PCP_PASSWD_FILE_NAME);
	snprintf(hba_file_path, sizeof(hba_file_path), "%s/%s", DEFAULT_CONFIGDIR, HBA_CONF_FILE_NAME);
	pool_passwd_key_file_path[0] = 0;

	while ((opt = getopt_long(argc, argv, "a:df:k:F:hm:nDCxvS", long_options, &optindex)) != -1)
	{
		switch (opt)
		{
			case 'a':			/* specify hba configuration file */
				if (!optarg)
				{
					usage();
					exit(1);
				}
				strlcpy(hba_file_path, optarg, sizeof(hba_file_path));
				break;

			case 'x':			/* enable cassert */
				assert_enabled = 1;
				break;

			case 'd':			/* debug option */
				debug_level = 1;
				break;

			case 'f':			/* specify configuration file */
				if (!optarg)
				{
					usage();
					exit(1);
				}
				strlcpy(conf_file_path, optarg, sizeof(conf_file_path));
				break;

			case 'F':			/* specify PCP password file */
				if (!optarg)
				{
					usage();
					exit(1);
				}
				strlcpy(pcp_conf_file_path, optarg, sizeof(pcp_conf_file_path));
				break;

			case 'k':			/* specify key file for decrypt pool_password
								 * entries */
				if (!optarg)
				{
					usage();
					exit(1);
				}
				strlcpy(pool_passwd_key_file_path, optarg, sizeof(pool_passwd_key_file_path));
				break;

			case 'h':
				usage();
				exit(0);
				break;

			case 'm':			/* stop mode */
				if (!optarg)
				{
					usage();
					exit(1);
				}
				if (*optarg == 's' || !strcmp("smart", optarg))
					stop_sig = SIGTERM; /* smart shutdown */
				else if (*optarg == 'f' || !strcmp("fast", optarg))
					stop_sig = SIGINT;	/* fast shutdown */
				else if (*optarg == 'i' || !strcmp("immediate", optarg))
					stop_sig = SIGQUIT; /* immediate shutdown */
				else
				{
					usage();
					exit(1);
				}
				break;

			case 'n':			/* no detaching control ttys */
				not_detach = 1;
				break;

			case 'D':			/* discard pgbalancer_status */
				discard_status = true;
				break;

			case 'C':			/* discard caches in memcached */
				clear_memcache_oidmaps = true;
				break;

		case 'v':
			show_version();
			exit(0);

		case 'S':
			show_config = true;
			break;

		default:
			usage();
			exit(1);
		}
	}

	myargv = save_ps_display_args(myargc, myargv);
	/* create MemoryContexts */
	MemoryContextInit();

	/* load the CWD before it is changed */
	base_dir = get_current_working_dir();
	/* convert all the paths to absolute paths */
	conf_file = make_absolute_path(conf_file_path, base_dir);
	pcp_conf_file = make_absolute_path(pcp_conf_file_path, base_dir);
	hba_file = make_absolute_path(hba_file_path, base_dir);

	mypid = getpid();
	SetProcessGlobalVariables(PT_MAIN);


	pool_init_config();

	pool_get_config(conf_file, CFGCXT_INIT);

	/*
	 * Override debug level if command line -d arg is given adjust the
	 * log_min_message config variable
	 */
	if (debug_level > 0 && pool_config->log_min_messages > DEBUG1)
		set_one_config_option("log_min_messages", "DEBUG1", CFGCXT_INIT, PGC_S_ARGV, INFO);

	/*
	 * If --show-config was specified, display configuration and exit
	 */
	if (show_config)
	{
		show_config_table();
		exit(0);
	}

	/*
	 * If a non-switch argument remains, then it should be either "reload" or
	 * "stop".
	 */
	if (optind == (argc - 1))
	{
		if (!strcmp(argv[optind], "reload"))
		{
			pid_t		pid;

			pid = read_pid_file();
			if (pid < 0)
			{
				ereport(FATAL,
						(return_code(1),
						 errmsg("could not read pid file")));
			}

			if (kill(pid, SIGHUP) == -1)
			{
				ereport(FATAL,
						(return_code(1),
						 errmsg("could not reload configuration file pid: %d", pid),
						 errdetail("%m")));
			}
			exit(0);
		}
		if (!strcmp(argv[optind], "stop"))
		{
			stop_me();
			exit(0);
		}
		else
		{
			usage();
			exit(1);
		}
	}

	/*
	 * else if no non-switch argument remains, then it should be a start
	 * request
	 */
	else if (optind == argc)
	{
		int			pid = read_pid_file();

		if (pid > 0)
		{
			if (kill(pid, 0) == 0)
			{
				fprintf(stderr, "ERROR: pid file found. is another pgpool(%d) is running?\n", pid);
				exit(EXIT_FAILURE);
			}
			else
				fprintf(stderr, "NOTICE: pid file found but it seems bogus. Trying to start pgpool anyway...\n");
		}
	}

	/*
	 * otherwise an error...
	 */
	else
	{
		usage();
		exit(1);
	}

	if (pool_config->enable_pool_hba)
		load_hba(hba_file);

#ifdef USE_SSL

	/*
	 * If ssl is enabled, initialize the SSL context
	 */
	if (pool_config->ssl)
		SSL_ServerSide_init();
#endif							/* USE_SSL */

	/* check effective user id for watchdog */
	/* watchdog must be started under the privileged user */
	wd_check_network_command_configurations();

	/* set signal masks */
	poolinitmask();

	/* read the pool password key */
	if (strlen(pool_passwd_key_file_path) == 0)
	{
		get_pool_key_filename(pool_passwd_key_file_path);
	}
	pool_key = read_pool_key(pool_passwd_key_file_path);

	if (not_detach)
		write_pid_file();
	else
		daemonize();

	/*
	 * Locate pool_passwd The default file name "pool_passwd" can be changed
	 * by setting pgbalancer.conf's "pool_passwd" directive.
	 */
	if (strcmp("", pool_config->pool_passwd))
	{
		char		pool_passwd[POOLMAXPATHLEN + 1];
		char		dirnamebuf[POOLMAXPATHLEN + 1];
		char	   *dirp;

		if (pool_config->pool_passwd[0] != '/')
		{
			strlcpy(dirnamebuf, conf_file, sizeof(dirnamebuf));
			dirp = dirname(dirnamebuf);
			snprintf(pool_passwd, sizeof(pool_passwd), "%s/%s",
					 dirp, pool_config->pool_passwd);
		}
		else
			strlcpy(pool_passwd, pool_config->pool_passwd,
					sizeof(pool_passwd));

		pool_init_pool_passwd(pool_passwd, POOL_PASSWD_R);
	}

	pool_semaphore_create(MAX_NUM_SEMAPHORES);

	PgpoolMain(discard_status, clear_memcache_oidmaps); /* this is an infinite
														 * loop */

	exit(0);

}

static void
show_version(void)
{
	fprintf(stderr, "%s version %s (%s)\n", PACKAGE, VERSION, PGPOOLVERSION);
}

/*
 * show_backends_structured - Display backends in PostgreSQL table style
 */
static void
show_backends_structured(void)
{
	int i, count = 0;
	char buf[256];
	
	/* Count backends */
	for (i = 0; i < MAX_NUM_BACKENDS; i++)
	{
		if (pool_config->backend_desc &&
			pool_config->backend_desc->backend_info[i].backend_hostname &&
			pool_config->backend_desc->backend_info[i].backend_hostname[0] != '\0')
			count++;
	}
	
	printf("\nBACKEND SERVERS:\n");
	
	if (count == 0)
	{
		printf("(no backends configured)\n");
		return;
	}
	
	printf(" %-8s | %-20s | %-8s | %-10s | %s\n", "id", "hostname", "port", "weight", "data_directory");
	printf("----------+----------------------+----------+------------+--------------------------------\n");
	
	for (i = 0; i < MAX_NUM_BACKENDS; i++)
	{
		if (!pool_config->backend_desc ||
			!pool_config->backend_desc->backend_info[i].backend_hostname ||
			pool_config->backend_desc->backend_info[i].backend_hostname[0] == '\0')
			continue;
		
		printf(" %-8d | %-20s | %-8d | %-10.1f | %s\n",
			i,
			pool_config->backend_desc->backend_info[i].backend_hostname,
			pool_config->backend_desc->backend_info[i].backend_port,
			(float)pool_config->backend_desc->backend_info[i].backend_weight / RAND_MAX,
			pool_config->backend_desc->backend_info[i].backend_data_directory ?
				pool_config->backend_desc->backend_info[i].backend_data_directory : "");
	}
	printf("(%d row%s)\n", count, count == 1 ? "" : "s");
}

/*
 * show_config_table - Display all configuration values in PostgreSQL table format
 */
static void
show_config_table(void)
{
	extern struct config_generic **all_parameters;
	extern int num_all_parameters;
	int i;
	char buf[256];
	
	/* Show structured sections */
	
	/* CLUSTERING */
	printf("\nCLUSTERING:\n");
	printf(" %-30s | %s\n", "mode", "replication_mode");
	printf("--------------------------------+---------------------------------------\n");
	printf(" %-30s | %s\n", "streaming_replication", pool_config->replication_mode ? "on" : "off");
	printf("(1 row)\n");
	
	/* NETWORK */
	printf("\nNETWORK:\n");
	printf(" %-20s | %-8s | %-25s | %s\n", "listen_addresses", "port", "unix_socket_directories", "unix_socket_permissions");
	printf("----------------------+----------+---------------------------+------------------------\n");
	snprintf(buf, sizeof(buf), "%d", pool_config->port);
	printf(" %-20s | %-8s | %-25s | 0%o\n", 
		pool_config->listen_addresses && pool_config->listen_addresses[0] ? pool_config->listen_addresses[0] : "*",
		buf,
		pool_config->unix_socket_directories && pool_config->unix_socket_directories[0] ? pool_config->unix_socket_directories[0] : "/tmp",
		pool_config->unix_socket_permissions);
	printf("(1 row)\n");
	
	/* CONNECTION POOL */
	printf("\nCONNECTION POOL:\n");
	printf(" %-18s | %-10s | %-16s | %-22s | %s\n", 
		"num_init_children", "max_pool", "child_life_time", "child_max_connections", "connection_cache");
	printf("--------------------+------------+------------------+------------------------+-----------------\n");
	printf(" %-18d | %-10d | %-16d | %-22d | %s\n",
		pool_config->num_init_children,
		pool_config->max_pool,
		pool_config->child_life_time,
		pool_config->child_max_connections,
		pool_config->connection_cache ? "on" : "off");
	printf("(1 row)\n");
	
	/* LOAD BALANCING */
	printf("\nLOAD BALANCING:\n");
	printf(" %-6s | %-27s | %s\n", "mode", "ignore_leading_white_space", "statement_level_load_balance");
	printf("--------+-----------------------------+------------------------------\n");
	printf(" %-6s | %-27s | %s\n",
		pool_config->load_balance_mode ? "on" : "off",
		pool_config->ignore_leading_white_space ? "on" : "off",
		pool_config->statement_level_load_balance ? "on" : "off");
	printf("(1 row)\n");
	
	/* HEALTH CHECK */
	printf("\nHEALTH CHECK:\n");
	printf(" %-8s | %-10s | %-15s | %s\n", "period", "timeout", "user", "database");
	printf("----------+------------+-----------------+------------------\n");
	printf(" %-8d | %-10d | %-15s | %s\n",
		pool_config->health_check_period,
		pool_config->health_check_timeout,
		pool_config->health_check_user ? pool_config->health_check_user : "",
		pool_config->health_check_database ? pool_config->health_check_database : "");
	printf("(1 row)\n");
	
	/* LOGGING */
	printf("\nLOGGING:\n");
	printf(" %-12s | %-18s | %-16s | %-13s | %-35s | %s\n", 
		"destination", "line_prefix", "log_connections", "log_hostname", "pid_file_name", "logdir");
	printf("--------------+--------------------+------------------+---------------+-------------------------------------+----------\n");
	printf(" %-12s | %-18s | %-16s | %-13s | %-35s | %s\n",
		pool_config->log_destination == 0 ? "stderr" : "syslog",
		pool_config->log_line_prefix ? pool_config->log_line_prefix : "",
		pool_config->log_connections ? "on" : "off",
		pool_config->log_hostname ? "on" : "off",
		pool_config->pid_file_name ? pool_config->pid_file_name : "",
		pool_config->logdir ? pool_config->logdir : "");
	printf("(1 row)\n");
	
	/* BACKENDS */
	show_backends_structured();
	
	/* WATCHDOG if enabled */
	if (pool_config->use_watchdog)
	{
		printf("\nWATCHDOG:\n");
		printf(" %-10s | %-10s | %-18s | %s\n", "enabled", "priority", "lifecheck_method", "interval");
		printf("------------+------------+--------------------+----------\n");
		printf(" %-10s | %-10d | %-18s | %d\n",
			"on",
			pool_config->wd_priority,
			pool_config->wd_lifecheck_method == 0 ? "heartbeat" : "query",
			pool_config->wd_interval);
		printf("(1 row)\n");
	}
	
	printf("\n");
	return;  /* Skip the detailed alphabetical listing for cleaner output */
	
	/* Below is the full alphabetical dump (kept for compatibility) */
	printf("\n");
	printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
	printf("║                          ALL PARAMETERS (Alphabetical - Full Listing)                                        ║\n");
	printf("╠════════════════════════════════════════════════╤═════════════════════════════════════════════════════════════╣\n");
	printf("║ Parameter                                      │ Value                                                       ║\n");
	printf("╠════════════════════════════════════════════════╪═════════════════════════════════════════════════════════════╣\n");
	
	printf("\n");
	printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
	printf("║                                   PGBALANCER CONFIGURATION                                                   ║\n");
	printf("╠════════════════════════════════════════════════╤═════════════════════════════════════════════════════════════╣\n");
	printf("║ Parameter                                      │ Value                                                       ║\n");
	printf("╠════════════════════════════════════════════════╪═════════════════════════════════════════════════════════════╣\n");
	
	for (i = 0; i < num_all_parameters; i++)
	{
		struct config_generic *conf = all_parameters[i];
		char value_str[256];
		const char *display_value = NULL;
		
		if (!conf || !conf->name)
			continue;
			
		/* Skip hidden variables */
		if (conf->flags & VAR_HIDDEN_IN_SHOW_ALL)
			continue;
		
		/* Get the value based on type */
		switch (conf->vartype)
		{
			case CONFIG_VAR_TYPE_BOOL:
			{
				struct config_bool *bconf = (struct config_bool *) conf;
				if (bconf->variable)
					display_value = *bconf->variable ? "true" : "false";
				else
					display_value = "NULL";
				break;
			}
			
		case CONFIG_VAR_TYPE_INT:
		{
			struct config_int *iconf = (struct config_int *) conf;
			if (iconf->variable)
			{
				if (iconf->show_hook)
					display_value = iconf->show_hook();
				else
				{
					snprintf(value_str, sizeof(value_str), "%d", *iconf->variable);
					display_value = value_str;
				}
			}
			else
				display_value = "NULL";
			break;
		}
			
			case CONFIG_VAR_TYPE_DOUBLE:
			{
				struct config_double *dconf = (struct config_double *) conf;
				if (dconf->variable)
				{
					snprintf(value_str, sizeof(value_str), "%g", *dconf->variable);
					display_value = value_str;
				}
				else
					display_value = "NULL";
				break;
			}
			
			case CONFIG_VAR_TYPE_LONG:
			{
				struct config_long *lconf = (struct config_long *) conf;
				if (lconf->variable)
				{
					snprintf(value_str, sizeof(value_str), "%ld", *lconf->variable);
					display_value = value_str;
				}
				else
					display_value = "NULL";
				break;
			}
			
			case CONFIG_VAR_TYPE_STRING:
			{
				struct config_string *sconf = (struct config_string *) conf;
				if (sconf->variable && *sconf->variable)
				{
					/* Check if this is a password field */
					if (conf->flags & VAR_HIDDEN_VALUE)
						display_value = "********";
					else
						display_value = *sconf->variable;
				}
				else
					display_value = "";
				break;
			}
			
			case CONFIG_VAR_TYPE_ENUM:
			{
				struct config_enum *econf = (struct config_enum *) conf;
				if (econf->variable)
				{
					/* Find the enum value name */
					const struct config_enum_entry *entry = econf->options;
					while (entry && entry->name)
					{
						if (entry->val == *econf->variable)
						{
							display_value = entry->name;
							break;
						}
						entry++;
					}
					if (!display_value)
					{
						snprintf(value_str, sizeof(value_str), "%d", *econf->variable);
						display_value = value_str;
					}
				}
				else
					display_value = "NULL";
				break;
			}
			
			case CONFIG_VAR_TYPE_INT_ARRAY:
			case CONFIG_VAR_TYPE_DOUBLE_ARRAY:
			case CONFIG_VAR_TYPE_STRING_ARRAY:
			case CONFIG_VAR_TYPE_STRING_LIST:
				display_value = "[array]";
				break;
				
			case CONFIG_VAR_TYPE_GROUP:
				continue;  /* Skip group headers */
				
			default:
				display_value = "[unknown type]";
				break;
		}
		
		if (display_value)
		{
			/* Truncate long values */
			char truncated_value[60];
			if (strlen(display_value) > 59)
			{
				strncpy(truncated_value, display_value, 56);
				strcpy(truncated_value + 56, "...");
				display_value = truncated_value;
			}
			
			printf("║ %-46s │ %-59s ║\n", conf->name, display_value);
		}
	}
	
	printf("╚════════════════════════════════════════════════╧═════════════════════════════════════════════════════════════╝\n");
	printf("\n");
}

static void
usage(void)
{
	char		homedir[POOLMAXPATHLEN];

	if (!get_home_directory(homedir, sizeof(homedir)))
		strncpy(homedir, "USER-HOME-DIR", POOLMAXPATHLEN);

	fprintf(stderr, "%s version %s (%s),\n", PACKAGE, VERSION, PGPOOLVERSION);
	fprintf(stderr, "  A generic connection pool/replication/load balance server for PostgreSQL\n\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  pgbalancer [ -c] [ -f CONFIG_FILE ] [ -a HBA_CONFIG_FILE ]\n");
	fprintf(stderr, "         [ -n ] [ -D ] [ -d ]\n");
	fprintf(stderr, "  pgbalancer [ -f CONFIG_FILE ] [ -a HBA_CONFIG_FILE ]\n");
	fprintf(stderr, "         [ -m SHUTDOWN-MODE ] stop\n");
	fprintf(stderr, "  pgbalancer [ -f CONFIG_FILE ] [ -a HBA_CONFIG_FILE ] reload\n\n");
	fprintf(stderr, "Common options:\n");
	fprintf(stderr, "  -a, --hba-file=HBA_CONFIG_FILE\n");
	fprintf(stderr, "                      Set the path to the pool_hba.conf configuration file\n");
	fprintf(stderr, "                      (default: %s/%s)\n", DEFAULT_CONFIGDIR, HBA_CONF_FILE_NAME);
	fprintf(stderr, "  -f, --config-file=CONFIG_FILE\n");
	fprintf(stderr, "                      Set the path to the pgbalancer configuration file (.conf format)\n");
	fprintf(stderr, "                      (default: %s/%s)\n", DEFAULT_CONFIGDIR, POOL_CONF_FILE_NAME);
	fprintf(stderr, "  -k, --key-file=KEY_FILE\n");
	fprintf(stderr, "                      Set the path to the pgbalancer key file\n");
	fprintf(stderr, "                      (default: %s/%s)\n", homedir, POOLKEYFILE);
	fprintf(stderr, "                      can be over ridden by %s environment variable\n", POOLKEYFILEENV);
	fprintf(stderr, "  -h, --help          Print this help\n");
	fprintf(stderr, "  -S, --show-config   Show all configuration values in table format and exit\n\n");
	fprintf(stderr, "Start options:\n");
	fprintf(stderr, "  -C, --clear-oidmaps Clear query cache oidmaps when memqcache_method is memcached\n");
	fprintf(stderr, "                      (If shmem, discards whenever pgbalancer starts.)\n");
	fprintf(stderr, "  -n, --dont-detach   Don't run in daemon mode, does not detach control tty\n");
	fprintf(stderr, "  -x, --debug-assertions   Turns on various assertion checks, This is a debugging aid\n");
	fprintf(stderr, "  -D, --discard-status Discard pgbalancer_status file and do not restore previous status\n");
	fprintf(stderr, "  -d, --debug         Debug mode\n\n");
	fprintf(stderr, "Stop options:\n");
	fprintf(stderr, "  -m, --mode=SHUTDOWN-MODE\n");
	fprintf(stderr, "                      Can be \"smart\", \"fast\", or \"immediate\"\n\n");
	fprintf(stderr, "Shutdown modes are:\n");
	fprintf(stderr, "  smart       quit after all clients have disconnected\n");
	fprintf(stderr, "  fast        quit directly, with proper shutdown\n");
	fprintf(stderr, "  immediate   the same mode as fast\n\n");
	fprintf(stderr, "Management:\n");
	fprintf(stderr, "  Use 'bctl' utility for REST API-based management\n");
	fprintf(stderr, "  Example: bctl status, bctl reload, bctl node attach\n");
}

static bool
get_pool_key_filename(char *poolKeyFile)
{
	char	   *passfile_env;

	if ((passfile_env = getenv(POOLKEYFILEENV)) != NULL)
	{
		/* use the literal path from the environment, if set */
		strlcpy(poolKeyFile, passfile_env, POOLMAXPATHLEN);
	}
	else
	{
		char		homedir[POOLMAXPATHLEN];

		if (!get_home_directory(homedir, sizeof(homedir)))
			return false;
		snprintf(poolKeyFile, POOLMAXPATHLEN + sizeof(POOLKEYFILE) + 1, "%s/%s", homedir, POOLKEYFILE);
	}
	return true;
}


char *
get_pool_key(void)
{
	return pool_key;
}

/*
* detach control ttys
*/
static void
daemonize(void)
{
	int			i;
	pid_t		pid;
	int			fdlimit;

	pid = fork();
	if (pid == (pid_t) -1)
	{
		ereport(FATAL,
				(errmsg("could not daemonize the pgbalancer"),
				 errdetail("fork() system call failed with reason: \"%m\"")));
	}
	else if (pid > 0)
	{							/* parent */
		exit(0);
	}

#ifdef HAVE_SETSID
	if (setsid() < 0)
	{
		ereport(FATAL,
				(errmsg("could not daemonize the pgbalancer"),
				 errdetail("setsid() system call failed with reason: \"%m\"")));
	}
#endif

	mypid = getpid();
	SetProcessGlobalVariables(PT_MAIN);
	write_pid_file();
	if (chdir("/"))
		ereport(WARNING,
				(errmsg("change directory failed"),
				 errdetail("chdir() system call failed with reason: \"%m\"")));

	/* redirect stdin, stdout and stderr to /dev/null */
	i = open("/dev/null", O_RDWR);
	if (i < 0)
	{
		ereport(WARNING,
				(errmsg("failed to open \"/dev/null\""),
				 errdetail("%m")));
	}
	else
	{
		dup2(i, 0);
		dup2(i, 1);
		dup2(i, 2);
		close(i);
	}
	/* close syslog connection for daemonizing */
	if (pool_config->log_destination & LOG_DESTINATION_SYSLOG)
	{
		closelog();
	}

	/* close other file descriptors */
	fdlimit = sysconf(_SC_OPEN_MAX);
	for (i = 3; i < fdlimit; i++)
		close(i);
}

/*
* stop myself
*/
static void
stop_me(void)
{
	pid_t		pid;
	char	   *pid_file;

	pid = read_pid_file();
	if (pid < 0)
	{
		ereport(FATAL,
				(errmsg("could not read pid file")));
	}

	for (;;)
	{
		int			cnt = 5;	/* sending signal retry interval */

		if (kill(pid, stop_sig) == -1)
		{
			ereport(FATAL,
					(errmsg("could not stop process with pid: %d", pid),
					 errdetail("%m")));
		}
		ereport(LOG,
				(errmsg("stop request sent to pgpool (pid: %d). waiting for termination...", pid)));

		while (kill(pid, 0) == 0)
		{
			fprintf(stderr, ".");
			sleep(1);
			cnt--;

			/*
			 * If pgpool did not stop within 5 seconds, break the loop and try
			 * to send the signal again
			 */
			if (cnt <= 0)
				break;
		}
		if (cnt > 0)
			break;
	}
	fprintf(stderr, "done.\n");
	pid_file = get_pid_file_path();
	unlink(pid_file);
	pfree(pid_file);
}

/*
 * The function returns the palloc'd copy of pid_file_path,
 * caller must free it after use
 */
static char *
get_pid_file_path(void)
{
	char	   *new = NULL;

	if (!is_absolute_path(pool_config->pid_file_name))
	{
		/*
		 * some implementations of dirname() may modify the string argument
		 * passed to it, so do not use the original conf_file as an argument
		 */
		char	   *conf_file_copy = pstrdup(conf_file);
		char	   *conf_dir = dirname(conf_file_copy);
		size_t		path_size;

		if (conf_dir == NULL)
		{
			ereport(LOG,
					(errmsg("failed to get the dirname of pid file:\"%s\"",
							pool_config->pid_file_name),
					 errdetail("%m")));
			return NULL;
		}
		path_size = strlen(conf_dir) + strlen(pool_config->pid_file_name) + 1 + 1;
		new = palloc(path_size);
		snprintf(new, path_size, "%s/%s", conf_dir, pool_config->pid_file_name);

		ereport(DEBUG1,
				(errmsg("pid file location is \"%s\"",
						new)));

		pfree(conf_file_copy);
	}
	else
	{
		new = pstrdup(pool_config->pid_file_name);
	}

	return new;
}

/*
* read the pid file
*/
static int
read_pid_file(void)
{
	int			fd;
	int			readlen;
	char		pidbuf[128];
	char	   *pid_file = get_pid_file_path();

	if (pid_file == NULL)
	{
		ereport(FATAL,
				(errmsg("failed to read pid file"),
				 errdetail("failed to get pid file path from \"%s\"",
						   pool_config->pid_file_name)));
	}
	fd = open(pid_file, O_RDONLY);
	if (fd == -1)
	{
		pfree(pid_file);
		return -1;
	}
	if ((readlen = read(fd, pidbuf, sizeof(pidbuf))) == -1)
	{
		close(fd);
		pfree(pid_file);
		ereport(FATAL,
				(errmsg("could not read pid file \"%s\"",
						pool_config->pid_file_name),
				 errdetail("%m")));
	}
	else if (readlen == 0)
	{
		close(fd);
		pfree(pid_file);
		ereport(FATAL,
				(errmsg("EOF detected while reading pid file \"%s\"",
						pool_config->pid_file_name),
				 errdetail("%m")));
	}
	pfree(pid_file);
	close(fd);
	return (atoi(pidbuf));
}

/*
* write the pid file
*/
static void
write_pid_file(void)
{
	int			fd;
	char		pidbuf[128];
	char	   *pid_file = get_pid_file_path();

	if (pid_file == NULL)
	{
		ereport(FATAL,
				(errmsg("failed to write pid file"),
				 errdetail("failed to get pid file path from \"%s\"",
						   pool_config->pid_file_name)));
	}

	fd = open(pid_file, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		ereport(FATAL,
				(errmsg("could not open pid file \"%s\"",
						pool_config->pid_file_name),
				 errdetail("%m")));
	}
	snprintf(pidbuf, sizeof(pidbuf), "%d", (int) getpid());
	if (write(fd, pidbuf, strlen(pidbuf) + 1) == -1)
	{
		close(fd);
		pfree(pid_file);
		ereport(FATAL,
				(errmsg("could not write pid file \"%s\"",
						pool_config->pid_file_name),
				 errdetail("%m")));
	}
	if (fsync(fd) == -1)
	{
		close(fd);
		pfree(pid_file);
		ereport(FATAL,
				(errmsg("could not fsync pid file \"%s\"",
						pool_config->pid_file_name),
				 errdetail("%m")));
	}
	if (close(fd) == -1)
	{
		pfree(pid_file);
		ereport(FATAL,
				(errmsg("could not close pid file \"%s\"",
						pool_config->pid_file_name),
				 errdetail("%m")));
	}
	/* register the call back to delete the pid file at system exit */
	on_proc_exit(FileUnlink, (Datum) pid_file);
}

/*
 * get_config_file_name: return full path of pgbalancer.conf.
 */
char *
get_config_file_name(void)
{
	return conf_file;
}

/*
 * get_hba_file_name: return full path of pool_hba.conf.
 */
char *
get_hba_file_name(void)
{
	return hba_file;
}

/*
 * Call back function to unlink the file
 */
static void
FileUnlink(int code, Datum path)
{
	char	   *filePath = (char *) path;

	if (unlink(filePath) == 0)
		return;

	/*
	 * We are already exiting the system just produce a log entry to report an
	 * error
	 */
	ereport(LOG,
			(errmsg("unlink failed for file at path \"%s\"", filePath),
			 errdetail("%m")));
}
