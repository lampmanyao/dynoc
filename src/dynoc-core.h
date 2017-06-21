#pragma once

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#include <hiredis.h>

#include "dynoc-token.h"

#define VALID   1
#define INVALID 0

typedef enum dc_type {
	REMOTE_DC,
	LOCAL_DC
} dc_type_t;

struct endpoint {
	char ip[16];
	int port;
};

struct continuum {
	uint32_t index;
	uint32_t value;
	struct token *token;
};

struct redis_connection {
	int valid;
	pthread_mutex_t lock;
	struct endpoint endpoint;
	redisContext *ctx;
};

struct rack {
	char *name;
	uint32_t node_count;
	uint32_t ncontinuum;
	struct continuum *continuum;
	struct redis_connection *redis_conn_pool;
};

struct datacenter {
	char *name;
	struct rack *rack;
	int rack_count;
	dc_type_t dc_type;
};

struct dynoc_hiredis_client {
	pthread_t tid;
	struct datacenter* local_dc;
	struct datacenter* remote_dc;
};

int dynoc_client_init(struct dynoc_hiredis_client *client);
int dynoc_client_add_node(struct dynoc_hiredis_client *client, const char *ip, int port,
                          const char *token, const char *rc_name, dc_type_t dc_type);
int dynoc_client_start(struct dynoc_hiredis_client *client);
void dynoc_client_destroy(struct dynoc_hiredis_client *client);

int init_datacenter(struct dynoc_hiredis_client *client, size_t rack_count, const char *name, dc_type_t dc_type);
void destroy_datacenter(struct datacenter *dc);

int init_rack(struct dynoc_hiredis_client *client, size_t node_count, const char *name, dc_type_t dc_type);
void destroy_rack(struct rack *rack);

void init_continuum(struct continuum *continuum, const char *token_str, uint32_t index);
void destroy_continuum(struct continuum *continuum);

void reset_redis_connection(struct redis_connection *redis_conn);
