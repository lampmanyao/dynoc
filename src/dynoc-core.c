#include "dynoc-core.h"
#include "hash-murmur.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void
reconnect_datacenter(struct datacenter *dc) {
	struct rack *rack;
	struct continuum *continuum;
	struct redis_connection *redis_conn;
	redisContext *c;

	for (int i = 0; i < dc->rack_count; i++) {
		rack = &dc->rack[i];
		for (int j = 0; j < rack->node_count; j++) {
			redis_conn = &rack->redis_conn_pool[j];
			continuum = &rack->continuum[j];
			pthread_mutex_lock(&redis_conn->lock);
			if (redis_conn->valid) {
				redisReply *reply = redisCommand(redis_conn->ctx, "PING");
				if (reply) {
					printf("%s:%s:%s:%d alive!\n",
						dc->name, rack->name, redis_conn->endpoint.ip, redis_conn->endpoint.port);
					freeReplyObject(reply);
				} else {
					redis_conn->valid = false;
					redisFree(redis_conn->ctx);
					redis_conn->ctx = NULL;
				}
			}
			if (!redis_conn->valid) {
				c = redisConnect(redis_conn->endpoint.ip, redis_conn->endpoint.port);
				if (c == NULL || c->err) {
					if (c) {
						redisFree(c);
					}
					printf("%s:%s:%s:%d reconnect failed!\n",
						dc->name, rack->name, redis_conn->endpoint.ip, redis_conn->endpoint.port);
				} else {
					redis_conn->valid = true;
					redis_conn->ctx = c;
					printf("%s:%s:%s:%d reconnect ok!\n",
						dc->name, rack->name, redis_conn->endpoint.ip, redis_conn->endpoint.port);
				}
			}
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}
}

static void *
reconnect_thread(void *arg) {
	struct dynoc_hiredis_client *client = arg;
	struct datacenter *dc;
	while (1) {
		sleep(10);

		dc = client->local_dc;
		if (dc) {
			reconnect_datacenter(dc);
		}

		dc = client->remote_dc;
		if (dc) {
			reconnect_datacenter(dc);
		}

	}
	return NULL;
}

int
dynoc_client_init(struct dynoc_hiredis_client *client) {
	client->local_dc = NULL;
	client->remote_dc = NULL;
	return 0;
}

void
dynoc_client_destroy(struct dynoc_hiredis_client *client) {
	pthread_cancel(client->tid);
	if (client) {
		if (client->local_dc) {
			destroy_datacenter(client->local_dc);
			free(client->local_dc);
		}

		if (client->remote_dc) {
			destroy_datacenter(client->remote_dc);
			free(client->remote_dc);
		}
	}
}

int
dynoc_client_add_node(struct dynoc_hiredis_client *client, const char *ip, int port,
                      const char *token_str, const char *rc_name, dc_type_t dc_type) {
	int count;
	uint32_t index;
	struct datacenter *dc;
	struct rack *rack;
	struct continuum *continuum;

	dc = dc_type ? client->local_dc : client->remote_dc;
	count = dc->rack_count;

	for (int i = 0; i < count; i++) {
		rack = &dc->rack[i];
		index = rack->ncontinuum;
		if (rack->name && strcmp(rack->name, rc_name) == 0) {
			continuum = &rack->continuum[index];
			init_continuum(continuum, token_str, index);

			rack->redis_conn_pool[index].endpoint.port = port;
			strncpy(rack->redis_conn_pool[index].endpoint.ip, ip, sizeof(rack->redis_conn_pool[index].endpoint.ip));

			redisContext *c = redisConnect(ip, port);
			if (c == NULL || c->err) {
				if (c) {
					redisFree(c);
				}
				return -1;
			} else {
				rack->redis_conn_pool[index].valid = true;
				rack->redis_conn_pool[index].ctx = c;
			}
			rack->ncontinuum++;
		}
	}
	return 0;
}

int
dynoc_client_start(struct dynoc_hiredis_client *client) {
	pthread_create(&client->tid, NULL, reconnect_thread, client);
	return 0;
}

int
init_datacenter(struct dynoc_hiredis_client *client, size_t rack_count, const char *name, dc_type_t dc_type) {
	struct datacenter *dc;
	dc = dc_type ? client->local_dc : client->remote_dc;

	if (dc) {
		return 0;
	}

	dc = malloc(sizeof(struct datacenter));
	if (!dc) {
		return -1;
	} else {
		dc->name = strdup(name);
		dc->rack = calloc(rack_count, sizeof(struct rack));
		dc->rack_count = rack_count;
		dc->dc_type = dc_type;
		if (dc_type) {
			client->local_dc = dc;
		} else {
			client->remote_dc = dc;
		}
	}
	return 0;
}

void
destroy_datacenter(struct datacenter *dc) {
	if (dc->name) {
		free(dc->name);
	}
	for (int i = 0; i < dc->rack_count; i++) {
		destroy_rack(&dc->rack[i]);
	}
	free(dc->rack);
}

int
init_rack(struct dynoc_hiredis_client *client, size_t node_count, const char *name, dc_type_t dc_type) {
	int count;
	struct datacenter *dc;
	struct rack *rack;

	dc = dc_type ? client->local_dc : client->remote_dc;
	count = dc->rack_count;
	for (int i = 0; i < count; i++) {
		rack = &dc->rack[i];
		if (!rack->name) {
			rack->name = strdup(name);
			rack->node_count = node_count;
			rack->ncontinuum = 0;
			rack->continuum = calloc(node_count, sizeof(struct continuum));
			rack->redis_conn_pool = calloc(node_count, sizeof(struct redis_connection));
			for (int j = 0; j < node_count; j++) {
				pthread_mutex_init(&rack->redis_conn_pool[j].lock, NULL);
				rack->redis_conn_pool[j].valid = false;
				rack->redis_conn_pool[j].ctx = NULL;
			}
			break;
		}
	}
	return 0;
}

void
destroy_rack(struct rack *rack) {
	if (rack->name) {
		free(rack->name);
	}

	if (rack->continuum) {
		for (int i = 0; i < rack->ncontinuum; i++) {
			destroy_continuum(&rack->continuum[i]);
		}
		free(rack->continuum);
	}

	if (rack->redis_conn_pool) {
		for (int i = 0; i < rack->node_count; i++) {
			if (rack->redis_conn_pool[i].ctx) {
				pthread_mutex_destroy(&rack->redis_conn_pool[i].lock);
				redisFree(rack->redis_conn_pool[i].ctx);
			}
		}
		free(rack->redis_conn_pool);
	}
}

void
init_continuum(struct continuum *continuum, const char *token_str, uint32_t index) {
	struct token *token = malloc(sizeof(struct token));
	init_token(token);
	size_token(token, 1);
	parse_token(token_str, strlen(token_str), token);
	continuum->index = index;
	continuum->token = token;
} 

void
destroy_continuum(struct continuum *continuum) {
	free(continuum->token);
}

