/*-------------------------------------------------------------------------
 *
 * bctl.c
 *      PgBalancer Control Tool - REST API client for pgbalancer management
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <getopt.h>
#include <errno.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <MQTTClient.h>

/* Command definitions */
typedef struct Command {
    char *name;
    char *description;
    char *usage;
    int (*handler)(int argc, char **argv);
} Command;

/* REST API Response structure */
typedef struct {
    char *data;
    size_t size;
    long http_code;
} RestResponse;

/* Global variables */
static char *program_name = "bctl";
static char *host = "localhost";
static int port = 8080;
static char *username = NULL;
static char *password = NULL;
static char *jwt_token = NULL;
static bool use_jwt = false;
static bool verbose = false;
static bool quiet = false;
static bool json_output = false;
static bool table_output = false;
static CURL *curl = NULL;

/* Forward declarations */
static int cmd_stop(int argc, char **argv);
static int cmd_status(int argc, char **argv);
static int cmd_reload(int argc, char **argv);
static int cmd_logrotate(int argc, char **argv);
static int cmd_nodes_count(int argc, char **argv);
static int cmd_nodes_info(int argc, char **argv);
static int cmd_nodes_attach(int argc, char **argv);
static int cmd_nodes_detach(int argc, char **argv);
static int cmd_nodes_recovery(int argc, char **argv);
static int cmd_nodes_promote(int argc, char **argv);
static int cmd_processes_count(int argc, char **argv);
static int cmd_processes_info(int argc, char **argv);
static int cmd_health_stats(int argc, char **argv);
static int cmd_cache_invalidate(int argc, char **argv);
static int cmd_watchdog_info(int argc, char **argv);
static int cmd_password_hash(int argc, char **argv);
static int cmd_password_encrypt(int argc, char **argv);
static int cmd_watchdog_status(int argc, char **argv);
static int cmd_watchdog_start(int argc, char **argv);
static int cmd_watchdog_stop(int argc, char **argv);
static int cmd_mqtt_info(int argc, char **argv);
static int cmd_mqtt_subscribe(int argc, char **argv);
static int cmd_mqtt_monitor(int argc, char **argv);
static int cmd_mqtt_publish(int argc, char **argv);
static int cmd_help(int argc, char **argv);

/* Helper function declarations */
static bool generate_md5_hash(const char *username, const char *password, char *hash);
static bool encrypt_password(const char *username, const char *password, char *encrypted);
static void parse_watchdog_status(const char *json_data);
static void parse_nodes_info(const char *json_data);
static void parse_nodes_info_table(const char *json_data);
static void parse_server_status(const char *json_data);
static int rest_get(const char *url, RestResponse *response);
static int rest_post(const char *url, const char *data, RestResponse *response);
static void print_verbose_request(const char *method, const char *url, const char *data);
static void print_verbose_response(RestResponse *response);

/* Available commands */
static Command commands[] = {
    {"stop", "Stop pgbalancer server", "bctl stop [options]", cmd_stop},
    {"status", "Show server status", "bctl status [options]", cmd_status},
    {"reload", "Reload configuration", "bctl reload [options]", cmd_reload},
    {"logrotate", "Rotate log files", "bctl logrotate [options]", cmd_logrotate},
    {"nodes", "Node management", "bctl nodes <subcommand> [options]", cmd_nodes_info},
    {"nodes-count", "Show node count", "bctl nodes-count [options]", cmd_nodes_count},
    {"nodes-attach", "Attach node", "bctl nodes-attach <node_id> [options]", cmd_nodes_attach},
    {"nodes-detach", "Detach node", "bctl nodes-detach <node_id> [options]", cmd_nodes_detach},
    {"nodes-recovery", "Recover node", "bctl nodes-recovery <node_id> [options]", cmd_nodes_recovery},
    {"nodes-promote", "Promote node", "bctl nodes-promote <node_id> [options]", cmd_nodes_promote},
    {"processes", "Process management", "bctl processes <subcommand> [options]", cmd_processes_info},
    {"processes-count", "Show process count", "bctl processes-count [options]", cmd_processes_count},
    {"health", "Health monitoring", "bctl health stats [options]", cmd_health_stats},
    {"cache", "Cache management", "bctl cache invalidate [options]", cmd_cache_invalidate},
    {"watchdog", "Watchdog management", "bctl watchdog info [options]", cmd_watchdog_info},
    {"watchdog-status", "Show watchdog status", "bctl watchdog-status [options]", cmd_watchdog_status},
    {"watchdog-start", "Start watchdog", "bctl watchdog-start [options]", cmd_watchdog_start},
    {"watchdog-stop", "Stop watchdog", "bctl watchdog-stop [options]", cmd_watchdog_stop},
    {"password", "Password management", "bctl password <subcommand> [options]", cmd_password_hash},
    {"password-hash", "Hash password with MD5", "bctl password-hash <username> [password]", cmd_password_hash},
    {"password-encrypt", "Encrypt password", "bctl password-encrypt <username> [password]", cmd_password_encrypt},
    {"mqtt", "Show MQTT event topics", "bctl mqtt [options]", cmd_mqtt_info},
    {"mqtt-subscribe", "Subscribe to MQTT topic", "bctl mqtt-subscribe <topic>", cmd_mqtt_subscribe},
    {"mqtt-monitor", "Monitor all pgbalancer events", "bctl mqtt-monitor", cmd_mqtt_monitor},
    {"mqtt-publish", "Publish MQTT message", "bctl mqtt-publish <topic> <message>", cmd_mqtt_publish},
    {NULL, NULL, NULL, NULL}
};

/*
 * REST API Helper Functions
 */

/*
 * Print verbose request information in pretty format
 */
static void
print_verbose_request(const char *method, const char *url, const char *data)
{
	if (!verbose)
		return;

	printf("\n");
	printf("=== REST REQUEST ===\n");
	printf("Method: %s\n", method);
	printf("URL: %s\n", url);

	if (data && strlen(data) > 0)
	{
		json_object *json_obj;

		printf("Request Body:\n");
		json_obj = json_tokener_parse(data);
		if (json_obj)
		{
			printf("%s\n", json_object_to_json_string_ext(json_obj,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED));
			json_object_put(json_obj);
		}
		else
		{
			printf("%s\n", data);
		}
	}
	else
	{
		printf("Request Body: (empty)\n");
	}
	printf("===================\n");
}

/*
 * Print verbose response information in pretty format
 */
static void
print_verbose_response(RestResponse *response)
{
	if (!verbose)
		return;

	printf("\n");
	printf("=== REST RESPONSE ===\n");
	printf("HTTP Status: %ld\n", response->http_code);

	if (response->data && response->size > 0)
	{
		json_object *json_obj;

		printf("Response Body:\n");
		json_obj = json_tokener_parse(response->data);
		if (json_obj)
		{
			printf("%s\n", json_object_to_json_string_ext(json_obj,
				JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED));
			json_object_put(json_obj);
		}
		else
		{
			printf("%s\n", response->data);
		}
	}
	else
	{
		printf("Response Body: (empty)\n");
	}
	printf("====================\n");
}

/* Callback function for curl to write response data */
static size_t
write_callback(void *contents, size_t size, size_t nmemb, RestResponse *response)
{
    size_t total_size = size * nmemb;
    response->data = realloc(response->data, response->size + total_size + 1);
    if (response->data == NULL) {
        return 0;
    }
    memcpy(&(response->data[response->size]), contents, total_size);
    response->size += total_size;
    response->data[response->size] = 0;
    return total_size;
}

/* Make REST API request */
static RestResponse*
make_rest_request(const char *method, const char *endpoint, const char *data)
{
    RestResponse *response = malloc(sizeof(RestResponse));
    char url[1024];
    CURLcode res;
    
    response->data = malloc(1);
    response->size = 0;
    response->http_code = 0;
    
    snprintf(url, sizeof(url), "http://%s:%d/api/v1%s", host, port, endpoint);
    
    /* Print verbose request information */
    print_verbose_request(method, url, data);
    
    if (curl) {
        curl_easy_reset(curl);
    } else {
        curl = curl_easy_init();
    }
    
    if (!curl) {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to initialize curl\n", program_name);
        }
        free(response->data);
        free(response);
        return NULL;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data ? data : "");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data ? strlen(data) : 0);
    }
    
    /* Set authentication: JWT or Basic Auth */
    struct curl_slist *headers = NULL;
    if (jwt_token && use_jwt) {
        char auth_header[2048];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", jwt_token);
        headers = curl_slist_append(headers, auth_header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    } else if (username && password) {
        char auth[256];
        snprintf(auth, sizeof(auth), "%s:%s", username, password);
        curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
    }
    
    if (verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
    
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        if (!quiet) {
            fprintf(stderr, "%s: %s\n", program_name, curl_easy_strerror(res));
        }
        free(response->data);
        free(response);
        return NULL;
    }
    
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_code);
    
    /* Print verbose response information */
    print_verbose_response(response);
    
    return response;
}

/* Free REST API response */
static void
free_rest_response(RestResponse *response)
{
    if (response) {
        free(response->data);
        free(response);
    }
}

/* Initialize REST client */
static bool
init_rest_client(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return true;
}

/* Cleanup REST client */
static void
cleanup_rest_client(void)
{
    if (curl) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    curl_global_cleanup();
}

/* Print JSON response */
static void
print_json_response(RestResponse *response)
{
    if (response && response->data) {
        struct json_object *json_obj = json_tokener_parse(response->data);
        if (json_obj) {
            printf("%s\n", json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(json_obj);
        } else {
            printf("%s\n", response->data);
        }
    }
}

/* Print plain response */
static void
print_plain_response(RestResponse *response)
{
    if (response && response->data) {
        printf("%s\n", response->data);
    }
}

/*
 * Command Handlers
 */

static int
cmd_stop(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("POST", "/control/stop", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Server stopped\n");
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to stop server (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_status(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("GET", "/status", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (json_output) {
            print_json_response(response);
        } else {
            print_plain_response(response);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to get server status (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_reload(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("POST", "/control/reload", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Configuration reloaded\n");
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to reload configuration (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_logrotate(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("POST", "/control/logrotate", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Logs rotated\n");
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to rotate logs (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_nodes_count(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("GET", "/nodes", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (json_output) {
            printf("%s\n", response->data);
        } else {
            /* Parse JSON to count nodes */
            struct json_object *json_obj = json_tokener_parse(response->data);
            if (json_obj) {
                if (json_object_is_type(json_obj, json_type_array)) {
                    printf("%zu\n", json_object_array_length(json_obj));
                } else {
                    printf("0\n");
                }
                json_object_put(json_obj);
            } else {
                printf("0\n");
            }
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to get node count (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_nodes_info(int argc, char **argv)
{
    RestResponse *response;
    char endpoint[256];
    
    /* Check if a specific node ID was provided after the command */
    /* Skip past any options like -t, -j, -v to find actual command arguments */
    int cmd_arg_start = optind + 1;  /* optind points to command, +1 is first arg after command */
    
    if (cmd_arg_start < argc && argv[cmd_arg_start] != NULL && 
        argv[cmd_arg_start][0] != '-') {
        /* Specific node ID provided */
        snprintf(endpoint, sizeof(endpoint), "/nodes/%s", argv[cmd_arg_start]);
    } else {
        /* List all nodes */
        strcpy(endpoint, "/nodes");
    }
    
    response = make_rest_request("GET", endpoint, NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (json_output) {
            printf("%s\n", response->data);
        } else if (table_output) {
            parse_nodes_info_table(response->data);
        } else {
            parse_nodes_info(response->data);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to get node info (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_nodes_attach(int argc, char **argv)
{
    RestResponse *response;
    char endpoint[256];
    
    if (argc < 3) {
        if (!quiet) {
            fprintf(stderr, "%s: Usage: %s nodes attach <node_id>\n", program_name, program_name);
        }
        return 1;
    }
    
    snprintf(endpoint, sizeof(endpoint), "/nodes/%s/attach", argv[2]);
    response = make_rest_request("POST", endpoint, NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Node %s attached\n", argv[2]);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to attach node %s (HTTP %ld)\n", program_name, argv[2], response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_nodes_detach(int argc, char **argv)
{
    RestResponse *response;
    char endpoint[256];
    
    if (argc < 3) {
        if (!quiet) {
            fprintf(stderr, "%s: Usage: %s nodes detach <node_id>\n", program_name, program_name);
        }
        return 1;
    }
    
    snprintf(endpoint, sizeof(endpoint), "/nodes/%s/detach", argv[2]);
    response = make_rest_request("POST", endpoint, NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Node %s detached\n", argv[2]);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to detach node %s (HTTP %ld)\n", program_name, argv[2], response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_nodes_recovery(int argc, char **argv)
{
    RestResponse *response;
    char endpoint[256];
    
    if (argc < 3) {
        if (!quiet) {
            fprintf(stderr, "%s: Usage: %s nodes recovery <node_id>\n", program_name, program_name);
        }
        return 1;
    }
    
    snprintf(endpoint, sizeof(endpoint), "/nodes/%s/recovery", argv[2]);
    response = make_rest_request("POST", endpoint, NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Node %s recovery initiated\n", argv[2]);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to initiate recovery for node %s (HTTP %ld)\n", program_name, argv[2], response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_nodes_promote(int argc, char **argv)
{
    RestResponse *response;
    char endpoint[256];
    
    if (argc < 3) {
        if (!quiet) {
            fprintf(stderr, "%s: Usage: %s nodes promote <node_id>\n", program_name, program_name);
        }
        return 1;
    }
    
    snprintf(endpoint, sizeof(endpoint), "/nodes/%s/promote", argv[2]);
    response = make_rest_request("POST", endpoint, NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Node %s promoted\n", argv[2]);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to promote node %s (HTTP %ld)\n", program_name, argv[2], response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_processes_count(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("GET", "/processes", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (json_output) {
            print_json_response(response);
        } else {
            /* Parse JSON to count processes */
            struct json_object *json_obj = json_tokener_parse(response->data);
            if (json_obj) {
                if (json_object_is_type(json_obj, json_type_array)) {
                    printf("%zu\n", json_object_array_length(json_obj));
                } else {
                    printf("0\n");
                }
                json_object_put(json_obj);
            } else {
                printf("0\n");
            }
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to get process count (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_processes_info(int argc, char **argv)
{
    RestResponse *response;
    char endpoint[256];
    
    if (argc > 2) {
        snprintf(endpoint, sizeof(endpoint), "/processes/%s", argv[2]);
    } else {
        strcpy(endpoint, "/processes");
    }
    
    response = make_rest_request("GET", endpoint, NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (json_output) {
            print_json_response(response);
        } else {
            print_plain_response(response);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to get process info (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_health_stats(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("GET", "/health/stats", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (json_output) {
            print_json_response(response);
        } else {
            print_plain_response(response);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to get health stats (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_cache_invalidate(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("POST", "/cache/invalidate", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (!quiet) {
            printf("Query cache invalidated\n");
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to invalidate cache (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_watchdog_info(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse *response;
    
    response = make_rest_request("GET", "/watchdog/info", NULL);
    if (!response) {
        return 1;
    }
    
    if (response->http_code == 200) {
        if (json_output) {
            print_json_response(response);
        } else {
            print_plain_response(response);
        }
        free_rest_response(response);
        return 0;
    } else {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to get watchdog info (HTTP %ld)\n", program_name, response->http_code);
        }
        free_rest_response(response);
        return 1;
    }
}

static int
cmd_help(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    int i;
    
    printf("bctl - pgBalancer Control Utility\n");
    printf("\n");
    printf("SYNOPSIS\n");
    printf("       bctl [OPTIONS] COMMAND [ARGUMENTS]\n");
    printf("\n");
    printf("DESCRIPTION\n");
    printf("       bctl is the control utility for pgbalancer, a PostgreSQL connection pooler\n");
    printf("       and load balancer. It provides administrative control over pgbalancer\n");
    printf("       through a REST API interface.\n");
    printf("\n");
    printf("COMMANDS\n");
    
    for (i = 0; commands[i].name != NULL; i++) {
        printf("       %-16s %s\n", commands[i].name, commands[i].description);
    }
    
    printf("\n");
    printf("OPTIONS\n");
    printf("       -H, --host HOSTNAME\n");
    printf("              Connect to pgbalancer on HOSTNAME (default: localhost)\n");
    printf("\n");
    printf("       -p, --port PORT\n");
    printf("              Connect to pgbalancer REST API on PORT (default: 8080)\n");
    printf("\n");
    printf("       -U, --username USER\n");
    printf("              Connect as USER for authentication\n");
    printf("\n");
    printf("       -v, --verbose\n");
    printf("              Enable verbose output\n");
    printf("\n");
    printf("       -q, --quiet\n");
    printf("              Quiet mode (minimal output)\n");
    printf("\n");
    printf("       -j, --json\n");
    printf("              Output results in JSON format\n");
    printf("\n");
    printf("       -t, --table\n");
    printf("              Output results in table format (for nodes command)\n");
    printf("\n");
    printf("       --help\n");
    printf("              Display this help and exit\n");
    printf("\n");
    printf("EXAMPLES\n");
    printf("       bctl status\n");
    printf("       bctl nodes\n");
    printf("       bctl -t nodes                      # Table format\n");
    printf("       bctl -j nodes                      # JSON format\n");
    printf("       bctl nodes attach 1\n");
    printf("       bctl -H remote-host -p 8080 status\n");
    printf("\n");
    printf("SEE ALSO\n");
    printf("       pgbalancer(1), pgbalancer.yaml(5)\n");
    
    return 0;
}

/*
 * Main function
 */
int
main(int argc, char **argv)
{
    int i, result, opt;
    char *command = NULL;
    
    static struct option long_options[] = {
        {"host", required_argument, 0, 'H'},
        {"port", required_argument, 0, 'p'},
        {"username", required_argument, 0, 'U'},
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"json", no_argument, 0, 'j'},
        {"table", no_argument, 0, 't'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    program_name = argv[0];
    
    /* Check for help option first */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            cmd_help(0, NULL);
            exit(0);
        }
    }
    
    /* Parse global options first to handle -v, -q, -j, -t, etc. */
    optind = 1;
    while ((opt = getopt_long(argc, argv, "H:p:U:vqjth", long_options, NULL)) != -1) {
        switch (opt) {
            case 'H':
                host = optarg;
                break;
            case 'h':
                cmd_help(0, NULL);
                exit(0);
            case 'p':
                port = atoi(optarg);
                break;
            case 'U':
                username = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'q':
                quiet = true;
                break;
            case 'j':
                json_output = true;
                break;
            case 't':
                table_output = true;
                break;
            default:
                cmd_help(0, NULL);
                exit(1);
        }
    }
    
    /* Find the command after options */
    if (optind >= argc) {
        cmd_help(0, NULL);
        exit(1);
    }
    
    command = argv[optind];
    
    /* Initialize REST client */
    if (!init_rest_client()) {
        if (!quiet) {
            fprintf(stderr, "%s: Failed to initialize REST client\n", program_name);
        }
        exit(1);
    }
    
    /* Find and execute command */
    for (i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, command) == 0) {
            result = commands[i].handler(argc, argv);
            cleanup_rest_client();
            exit(result);
        }
    }
    
    if (!quiet) {
        fprintf(stderr, "%s: Unknown command: %s\n", program_name, command);
        fprintf(stderr, "Try '%s help' for more information.\n", program_name);
    }
    cleanup_rest_client();
    exit(1);
}

/*
 * MQTT Information Command
 */

static int
cmd_mqtt_info(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  pgbalancer MQTT Event Topics\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    printf("Node Events:\n");
    printf("  Topic: pgbalancer/nodes/{id}/status\n");
    printf("  Event: Node status changes (up/down)\n");
    printf("  Example: {\"node_id\":0,\"status\":\"up\",\"timestamp\":1234567890}\n\n");
    
    printf("  Topic: pgbalancer/nodes/{id}/health\n");
    printf("  Event: Health check results\n");
    printf("  Example: {\"node_id\":0,\"healthy\":true,\"timestamp\":1234567890}\n\n");
    
    printf("  Topic: pgbalancer/nodes/{id}/events\n");
    printf("  Event: Node operations (attach/detach/recovery/promote)\n");
    printf("  Example: {\"node_id\":1,\"event\":\"attach\",\"timestamp\":1234567890}\n\n");
    
    printf("Cluster Events:\n");
    printf("  Topic: pgbalancer/events/failover\n");
    printf("  Event: Failover events\n");
    printf("  Example: {\"event\":\"failover\",\"old_primary\":0,\"new_primary\":1}\n\n");
    
    printf("Statistics:\n");
    printf("  Topic: pgbalancer/stats/connections\n");
    printf("  Event: Connection pool statistics (periodic)\n");
    printf("  Example: {\"total\":10,\"active\":5,\"idle\":5}\n\n");
    
    printf("  Topic: pgbalancer/stats/queries\n");
    printf("  Event: Query statistics (periodic)\n");
    printf("  Example: {\"qps\":150,\"avg_response_time_ms\":5.2}\n\n");
    
    printf("Broker Configuration:\n");
    printf("  Host: localhost\n");
    printf("  Port: 1883\n");
    printf("  Client ID: pgbalancer\n\n");
    
    printf("Subscribe to events:\n");
    printf("  mosquitto_sub -h localhost -t 'pgbalancer/#' -v\n");
    printf("  mosquitto_sub -h localhost -t 'pgbalancer/nodes/+/status'\n");
    printf("  mosquitto_sub -h localhost -t 'pgbalancer/events/failover'\n\n");
    
    printf("Integration examples:\n");
    printf("  • Grafana: Use MQTT data source plugin\n");
    printf("  • Prometheus: Use MQTT exporter\n");
    printf("  • Node-RED: Visual MQTT flow processing\n");
    printf("  • Home Assistant: Device/sensor integration\n\n");
    
    return 0;
}

/*
 * MQTT Subscribe Command
 */

static volatile int mqtt_running = 1;

static void mqtt_signal_handler(int sig __attribute__((unused)))
{
    mqtt_running = 0;
}

static int message_arrived(void *context __attribute__((unused)), 
                          char *topicName, 
                          int topicLen __attribute__((unused)), 
                          MQTTClient_message *message)
{
    printf("%s %.*s\n", topicName, (int)message->payloadlen, (char*)message->payload);
    fflush(stdout);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

static int
cmd_mqtt_subscribe(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Error: Topic required\n");
        fprintf(stderr, "Usage: bctl mqtt-subscribe <topic>\n");
        fprintf(stderr, "Example: bctl mqtt-subscribe 'pgbalancer/nodes/+/status'\n");
        return 1;
    }

    const char *topic = argv[1];
    const char *broker = "tcp://localhost:1883";
    const char *client_id = "bctl";
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    printf("Subscribing to MQTT topic: %s\n", topic);
    printf("Press Ctrl+C to stop...\n\n");

    MQTTClient_create(&client, broker, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, NULL, message_arrived, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to connect to MQTT broker: %d\n", rc);
        return 1;
    }

    MQTTClient_subscribe(client, topic, 1);

    signal(SIGINT, mqtt_signal_handler);
    signal(SIGTERM, mqtt_signal_handler);

    while (mqtt_running) {
        usleep(100000);
    }

    printf("\nDisconnecting...\n");
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    
    return 0;
}

/*
 * MQTT Monitor Command (subscribe to all pgbalancer topics)
 */

static int
cmd_mqtt_monitor(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    const char *topic = "pgbalancer/#";
    const char *broker = "tcp://localhost:1883";
    const char *client_id = "bctl-monitor";
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Monitoring all pgbalancer MQTT events\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Topic: %s\n", topic);
    printf("Broker: localhost:1883\n");
    printf("Press Ctrl+C to stop...\n\n");

    MQTTClient_create(&client, broker, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, NULL, message_arrived, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to connect to MQTT broker: %d\n", rc);
        fprintf(stderr, "Make sure mosquitto is running: brew services start mosquitto\n");
        return 1;
    }

    printf("✅ Connected to MQTT broker\n");
    printf("Listening for events...\n\n");

    MQTTClient_subscribe(client, topic, 1);

    signal(SIGINT, mqtt_signal_handler);
    signal(SIGTERM, mqtt_signal_handler);

    while (mqtt_running) {
        usleep(100000);
    }

    printf("\nDisconnecting...\n");
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    
    return 0;
}

/*
 * MQTT Publish Command
 */

static int
cmd_mqtt_publish(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Error: Topic and message required\n");
        fprintf(stderr, "Usage: bctl mqtt-publish <topic> <message>\n");
        fprintf(stderr, "Example: bctl mqtt-publish 'pgbalancer/test' '{\"status\":\"ok\"}'\n");
        return 1;
    }

    const char *topic = argv[1];
    const char *message = argv[2];
    const char *broker = "tcp://localhost:1883";
    const char *client_id = "bctl-publish";
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, broker, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to connect to MQTT broker: %d\n", rc);
        return 1;
    }

    pubmsg.payload = (void*)message;
    pubmsg.payloadlen = (int)strlen(message);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000);
    
    printf("✅ Published to topic: %s\n", topic);
    printf("Message: %s\n", message);

    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    
    return 0;
}

/*
 * Password Management Commands
 */

static int
cmd_password_hash(int argc, char **argv)
{
    char *username = NULL;
    char *password = NULL;
    char hash[33];
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s password-hash <username> [password]\n", program_name);
        return 1;
    }
    
    username = argv[1];
    
    /* Get password from argument or prompt */
    if (argc > 2) {
        password = argv[2];
    } else {
        password = getpass("Enter password: ");
        if (password == NULL) {
            fprintf(stderr, "Failed to read password\n");
            return 1;
        }
    }
    
    /* Generate MD5 hash (simplified version) */
    if (generate_md5_hash(username, password, hash)) {
        printf("MD5 Hash: %s\n", hash);
        return 0;
    } else {
        fprintf(stderr, "Failed to generate MD5 hash\n");
        return 1;
    }
}

static int
cmd_password_encrypt(int argc, char **argv)
{
    char *username = NULL;
    char *password = NULL;
    char encrypted[256];
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s password-encrypt <username> [password]\n", program_name);
        return 1;
    }
    
    username = argv[1];
    
    /* Get password from argument or prompt */
    if (argc > 2) {
        password = argv[2];
    } else {
        password = getpass("Enter password: ");
        if (password == NULL) {
            fprintf(stderr, "Failed to read password\n");
            return 1;
        }
    }
    
    /* Encrypt password (simplified version) */
    if (encrypt_password(username, password, encrypted)) {
        printf("Encrypted Password: %s\n", encrypted);
        return 0;
    } else {
        fprintf(stderr, "Failed to encrypt password\n");
        return 1;
    }
}

/*
 * Enhanced Watchdog Commands
 */

static int
cmd_watchdog_status(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse response = {0};
    char url[256];
    
    snprintf(url, sizeof(url), "http://%s:%d/api/v1/watchdog/status", host, port);
    
    if (verbose) {
        printf("Requesting watchdog status from %s\n", url);
    }
    
    if (rest_get(url, &response) != 0) {
        if (!quiet) {
            fprintf(stderr, "Failed to get watchdog status\n");
        }
        return 1;
    }
    
    if (response.http_code != 200) {
        if (!quiet) {
            fprintf(stderr, "HTTP Error %ld: %s\n", response.http_code, response.data);
        }
        return 1;
    }
    
    if (json_output) {
        printf("%s\n", response.data);
    } else {
        /* Parse and format JSON response */
        parse_watchdog_status(response.data);
    }
    
    free(response.data);
    return 0;
}

static int
cmd_watchdog_start(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse response = {0};
    char url[256];
    
    snprintf(url, sizeof(url), "http://%s:%d/api/v1/watchdog/start", host, port);
    
    if (verbose) {
        printf("Starting watchdog via %s\n", url);
    }
    
    if (rest_post(url, NULL, &response) != 0) {
        if (!quiet) {
            fprintf(stderr, "Failed to start watchdog\n");
        }
        return 1;
    }
    
    if (response.http_code != 200) {
        if (!quiet) {
            fprintf(stderr, "HTTP Error %ld: %s\n", response.http_code, response.data);
        }
        return 1;
    }
    
    if (!quiet) {
        printf("Watchdog started successfully\n");
    }
    
    free(response.data);
    return 0;
}

static int
cmd_watchdog_stop(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    RestResponse response = {0};
    char url[256];
    
    snprintf(url, sizeof(url), "http://%s:%d/api/v1/watchdog/stop", host, port);
    
    if (verbose) {
        printf("Stopping watchdog via %s\n", url);
    }
    
    if (rest_post(url, NULL, &response) != 0) {
        if (!quiet) {
            fprintf(stderr, "Failed to stop watchdog\n");
        }
        return 1;
    }
    
    if (response.http_code != 200) {
        if (!quiet) {
            fprintf(stderr, "HTTP Error %ld: %s\n", response.http_code, response.data);
        }
        return 1;
    }
    
    if (!quiet) {
        printf("Watchdog stopped successfully\n");
    }
    
    free(response.data);
    return 0;
}

/*
 * Helper Functions for New Commands
 */

static bool
generate_md5_hash(const char *username, const char *password, char *hash)
{
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    char combined[256];
    char hex_string[33];
    int i;
    
    /* Create combined string: password + username */
    snprintf(combined, sizeof(combined), "%s%s", password, username);
    
    /* Use EVP interface for MD5 */
    md = EVP_md5();
    mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return false;
    }
    
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        return false;
    }
    
    if (EVP_DigestUpdate(mdctx, combined, strlen(combined)) != 1) {
        EVP_MD_CTX_free(mdctx);
        return false;
    }
    
    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return false;
    }
    
    EVP_MD_CTX_free(mdctx);
    
    /* Convert to hex string */
    for (i = 0; i < (int)digest_len; i++) {
        snprintf(hex_string + (i * 2), 3, "%02x", digest[i]);
    }
    
    /* Add md5 prefix */
    snprintf(hash, 33, "md5%s", hex_string);
    
    return true;
}

static bool
encrypt_password(const char *username __attribute__((unused)), const char *password, char *encrypted)
{
    EVP_CIPHER_CTX *ctx;
    unsigned char key[32];
    unsigned char iv[16];
    unsigned char *plaintext = (unsigned char*)password;
    unsigned char ciphertext[256];
    int len;
    int ciphertext_len;
    int i;
    
    /* Generate random key and IV */
    if (RAND_bytes(key, 32) != 1 || RAND_bytes(iv, 16) != 1) {
        return false;
    }
    
    /* Create and initialize context */
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }
    
    /* Initialize encryption operation */
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    /* Encrypt the plaintext */
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, strlen(password)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len = len;
    
    /* Finalize encryption */
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;
    
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    
    /* Convert to base64-like format */
    snprintf(encrypted, 256, "enc_");
    char *ptr = encrypted + 4;
    
    /* Encode key (simplified base64) */
    for (i = 0; i < 32; i++) {
        snprintf(ptr, 3, "%02x", key[i]);
        ptr += 2;
    }
    *ptr++ = ':';
    
    /* Encode IV */
    for (i = 0; i < 16; i++) {
        snprintf(ptr, 3, "%02x", iv[i]);
        ptr += 2;
    }
    *ptr++ = ':';
    
    /* Encode ciphertext */
    for (i = 0; i < ciphertext_len; i++) {
        snprintf(ptr, 3, "%02x", ciphertext[i]);
        ptr += 2;
    }
    *ptr = '\0';
    
    return true;
}

static void
parse_watchdog_status(const char *json_data)
{
    json_object *root, *status, *nodes, *node, *value;
    const char *status_str;
    int i, node_count;
    
    /* Parse JSON */
    root = json_tokener_parse(json_data);
    if (!root) {
        printf("Watchdog Status:\n");
        printf("  Failed to parse JSON response\n");
        return;
    }
    
    printf("Watchdog Status:\n");
    
    /* Parse status */
    if (json_object_object_get_ex(root, "status", &status)) {
        status_str = json_object_get_string(status);
        printf("  Status: %s\n", status_str ? status_str : "unknown");
    }
    
    /* Parse nodes */
    if (json_object_object_get_ex(root, "nodes", &nodes)) {
        node_count = json_object_array_length(nodes);
        printf("  Nodes: %d\n", node_count);
        
        for (i = 0; i < node_count; i++) {
            node = json_object_array_get_idx(nodes, i);
            if (node) {
                printf("    Node %d:\n", i + 1);
                
                if (json_object_object_get_ex(node, "host", &value)) {
                    printf("      Host: %s\n", json_object_get_string(value));
                }
                if (json_object_object_get_ex(node, "port", &value)) {
                    printf("      Port: %d\n", json_object_get_int(value));
                }
                if (json_object_object_get_ex(node, "status", &value)) {
                    printf("      Status: %s\n", json_object_get_string(value));
                }
                if (json_object_object_get_ex(node, "last_heartbeat", &value)) {
                    printf("      Last Heartbeat: %s\n", json_object_get_string(value));
                }
            }
        }
    }
    
    /* Parse other fields */
    if (json_object_object_get_ex(root, "uptime", &value)) {
        printf("  Uptime: %d seconds\n", json_object_get_int(value));
    }
    
    if (json_object_object_get_ex(root, "active_nodes", &value)) {
        printf("  Active Nodes: %d\n", json_object_get_int(value));
    }
    
    json_object_put(root);
}

static void
parse_nodes_info_table(const char *json_data)
{
    json_object *root, *nodes, *node, *value;
    int i, node_count;
    
    root = json_tokener_parse(json_data);
    if (!root) {
        printf("Failed to parse JSON response\n");
        return;
    }
    
    if (!json_object_object_get_ex(root, "nodes", &nodes)) {
        printf("No nodes found\n");
        json_object_put(root);
        return;
    }
    
    node_count = json_object_array_length(nodes);
    
    /* Print table header */
    printf("┌────┬─────────────────┬───────┬──────────┬────────┬─────────┬──────────┐\n");
    printf("│ ID │ Host            │ Port  │ Status   │ Weight │ Role    │ Rep Lag  │\n");
    printf("├────┼─────────────────┼───────┼──────────┼────────┼─────────┼──────────┤\n");
    
    /* Print each node */
    for (i = 0; i < node_count; i++) {
        node = json_object_array_get_idx(nodes, i);
        if (node) {
            int id = 0, port = 0, weight = 0, lag = 0;
            const char *host = "", *status = "", *role = "";
            
            if (json_object_object_get_ex(node, "id", &value))
                id = json_object_get_int(value);
            if (json_object_object_get_ex(node, "host", &value))
                host = json_object_get_string(value);
            if (json_object_object_get_ex(node, "port", &value))
                port = json_object_get_int(value);
            if (json_object_object_get_ex(node, "status", &value))
                status = json_object_get_string(value);
            if (json_object_object_get_ex(node, "weight", &value))
                weight = json_object_get_int(value);
            if (json_object_object_get_ex(node, "role", &value))
                role = json_object_get_string(value);
            if (json_object_object_get_ex(node, "replication_lag", &value))
                lag = json_object_get_int(value);
            
            printf("│ %-2d │ %-15s │ %-5d │ %-8s │ %-6d │ %-7s │ %-8d │\n",
                   id, host, port, status, weight, role, lag);
        }
    }
    
    printf("└────┴─────────────────┴───────┴──────────┴────────┴─────────┴──────────┘\n");
    printf("Total nodes: %d\n", node_count);
    
    json_object_put(root);
}

static void
parse_nodes_info(const char *json_data)
{
    json_object *root, *nodes, *node, *value;
    int i, node_count;
    
    root = json_tokener_parse(json_data);
    if (!root) {
        printf("Nodes Information:\n");
        printf("  Failed to parse JSON response\n");
        return;
    }
    
    printf("Nodes Information:\n");
    
    if (json_object_object_get_ex(root, "nodes", &nodes)) {
        node_count = json_object_array_length(nodes);
        printf("  Total Nodes: %d\n", node_count);
        
        for (i = 0; i < node_count; i++) {
            node = json_object_array_get_idx(nodes, i);
            if (node) {
                printf("  Node %d:\n", i + 1);
                
                if (json_object_object_get_ex(node, "id", &value)) {
                    printf("    ID: %d\n", json_object_get_int(value));
                }
                if (json_object_object_get_ex(node, "host", &value)) {
                    printf("    Host: %s\n", json_object_get_string(value));
                }
                if (json_object_object_get_ex(node, "port", &value)) {
                    printf("    Port: %d\n", json_object_get_int(value));
                }
                if (json_object_object_get_ex(node, "status", &value)) {
                    printf("    Status: %s\n", json_object_get_string(value));
                }
                if (json_object_object_get_ex(node, "weight", &value)) {
                    printf("    Weight: %d\n", json_object_get_int(value));
                }
                if (json_object_object_get_ex(node, "role", &value)) {
                    printf("    Role: %s\n", json_object_get_string(value));
                }
                if (json_object_object_get_ex(node, "replication_lag", &value)) {
                    printf("    Replication Lag: %d\n", json_object_get_int(value));
                }
                printf("\n");
            }
        }
    }
    
    json_object_put(root);
}

static void __attribute__((unused))
parse_server_status(const char *json_data)
{
    json_object *root, *value;
    
    root = json_tokener_parse(json_data);
    if (!root) {
        printf("Server Status:\n");
        printf("  Failed to parse JSON response\n");
        return;
    }
    
    printf("Server Status:\n");
    
    if (json_object_object_get_ex(root, "status", &value)) {
        printf("  Status: %s\n", json_object_get_string(value));
    }
    
    if (json_object_object_get_ex(root, "uptime", &value)) {
        printf("  Uptime: %d seconds\n", json_object_get_int(value));
    }
    
    if (json_object_object_get_ex(root, "version", &value)) {
        printf("  Version: %s\n", json_object_get_string(value));
    }
    
    if (json_object_object_get_ex(root, "connections", &value)) {
        printf("  Active Connections: %d\n", json_object_get_int(value));
    }
    
    if (json_object_object_get_ex(root, "nodes", &value)) {
        printf("  Total Nodes: %d\n", json_object_get_int(value));
    }
    
    if (json_object_object_get_ex(root, "healthy_nodes", &value)) {
        printf("  Healthy Nodes: %d\n", json_object_get_int(value));
    }
    
    json_object_put(root);
}

/*
 * REST API Helper Functions
 */

static int
rest_get(const char *url, RestResponse *response)
{
    CURL *curl_handle;
    CURLcode res;
    
    /* Print verbose request information */
    print_verbose_request("GET", url, NULL);
    
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        return -1;
    }
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    res = curl_easy_perform(curl_handle);
    
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl_handle);
        return -1;
    }
    
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response->http_code);
    curl_easy_cleanup(curl_handle);
    
    /* Print verbose response information */
    print_verbose_response(response);
    
    return 0;
}

static int
rest_post(const char *url, const char *data, RestResponse *response)
{
    CURL *curl_handle;
    CURLcode res;
    struct curl_slist *headers = NULL;
    
    /* Print verbose request information */
    print_verbose_request("POST", url, data);
    
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        return -1;
    }
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    /* Set POST data if provided */
    if (data) {
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    } else {
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    }
    
    res = curl_easy_perform(curl_handle);
    
    if (res != CURLE_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl_handle);
        return -1;
    }
    
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response->http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl_handle);
    
    /* Print verbose response information */
    print_verbose_response(response);
    
    return 0;
}