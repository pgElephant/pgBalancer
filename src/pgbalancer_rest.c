// Minimal REST API for pgbalancer using mongoose
#include "mongoose.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Node struct
typedef struct {
    int id;
    char name[32];
    char ip[16];
    int rale_port;
    int dstore_port;
    char status[16];
} Node;

#define MAX_NODES 16
Node cluster_nodes[MAX_NODES] = {
    {1, "cluster-node-01", "127.0.0.1", 7400, 7401, "leader"},
    {2, "cluster-node-02", "127.0.0.1", 7410, 7411, "follower"},
    {3, "cluster-node-03", "127.0.0.1", 7420, 7421, "follower"}
};
int node_count = 3;

void json_nodes(struct mg_connection *c) {
    mg_printf(c,
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
        "{ \"nodes\": [");
    for (int i = 0; i < node_count; i++) {
        mg_printf(c,
            "%s{\"id\":%d,\"name\":\"%s\",\"ip\":\"%s\",\"rale_port\":%d,\"dstore_port\":%d,\"status\":\"%s\"}",
            i ? "," : "", cluster_nodes[i].id, cluster_nodes[i].name, cluster_nodes[i].ip,
            cluster_nodes[i].rale_port, cluster_nodes[i].dstore_port, cluster_nodes[i].status);
    }
    mg_printf(c, "], \"cluster_size\":%d, \"leader_id\":1 }\n", node_count);
}

void json_health(struct mg_connection *c) {
    time_t now = time(NULL);
    mg_printf(c,
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
        "{ \"status\": \"healthy\", \"version\": \"1.0.0\", \"timestamp\": \"%ld\", \"uptime\": \"2h 15m 30s\" }\n", now);
}

void json_status(struct mg_connection *c) {
    mg_printf(c,
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
        "{ \"cluster_name\": \"test-cluster\", \"current_term\": 5, \"leader_id\": 1, \"quorum_size\": 2, \"nodes_online\": %d, \"consensus_state\": \"stable\" }\n", node_count);
}

void add_node(const char *json, struct mg_connection *c) {
    char name[32] = "node";
    char ip[16] = "127.0.0.1";
    sscanf(json, "{\"name\":\"%31[^"]\",\"ip\":\"%15[^"]", name, ip);
    if (node_count < MAX_NODES) {
        Node *n = &cluster_nodes[node_count];
        n->id = node_count + 1;
        strncpy(n->name, name, 31);
        strncpy(n->ip, ip, 15);
        n->rale_port = 7400 + node_count * 10;
        n->dstore_port = 7401 + node_count * 10;
        strcpy(n->status, "follower");
        node_count++;
        mg_printf(c,
            "HTTP/1.1 201 Created\r\nContent-Type: application/json\r\n\r\n"
            "{ \"message\": \"Node %s added successfully\", \"node\": {\"id\":%d,\"name\":\"%s\"} }\n",
            n->name, n->id, n->name);
    } else {
        mg_printf(c, "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Max nodes reached\"}\n");
    }
}

void remove_node(int id, struct mg_connection *c) {
    int found = 0;
    for (int i = 0; i < node_count; i++) {
        if (cluster_nodes[i].id == id) {
            for (int j = i; j < node_count - 1; j++)
                cluster_nodes[j] = cluster_nodes[j + 1];
            node_count--;
            found = 1;
            break;
        }
    }
    if (found)
        mg_printf(c, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"message\":\"Node %d removed successfully\"}\n", id);
    else
        mg_printf(c, "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Node not found\"}\n");
}

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (ev == MG_EV_HTTP_MSG) {
        if (mg_http_match_uri(hm, "/api/v1/health")) {
            json_health(c);
        } else if (mg_http_match_uri(hm, "/api/v1/nodes") && mg_vcmp(&hm->method, "GET") == 0) {
            json_nodes(c);
        } else if (mg_http_match_uri(hm, "/api/v1/nodes") && mg_vcmp(&hm->method, "POST") == 0) {
            add_node(hm->body.ptr, c);
        } else if (mg_http_match_uri(hm, "/api/v1/status")) {
            json_status(c);
        } else if (mg_http_match_uri(hm, "/api/v1/nodes/") && mg_vcmp(&hm->method, "DELETE") == 0) {
            int id = atoi(hm->uri.ptr + strlen("/api/v1/nodes/"));
            remove_node(id, c);
        } else {
            mg_printf(c, "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Not Found\"}\n");
        }
    }
}

int main(void) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8090", fn, NULL);
    printf("pgbalancer REST server running on port 8090\n");
    for (;;) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
    return 0;
}
