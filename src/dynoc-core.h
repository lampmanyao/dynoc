/*
 * Dynoc is a minimalistic C client library for the dynomite.
 * Copyright (C) 2016-2017 huya.com, Lampman Yao
 */

/*
 * Dynomite - A thin, distributed replication layer for multi non-distributed storages.
 * Copyright (C) 2014 Netflix, Inc.
 */

/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#include <hiredis.h>

#include "dynoc-hashkit.h"
#include "dynoc-token.h"

#define VALID   1
#define INVALID 0
#define DEFAULT_HASH HASH_MURMUR

typedef enum dc_type {
	REMOTE_DC,
	LOCAL_DC
} dc_type_t;

struct endpoint {
	char *host;
	int port;
	char *pass;
};

struct continuum {
	uint32_t index;
	uint32_t value;
	struct endpoint endpoint;
	struct token *token;
};

struct redis_connection {
	uint32_t status;
	pthread_mutex_t lock;
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
	uint32_t rack_count;
	dc_type_t dc_type;
};

struct dynoc {
	pthread_t tid;
	hash_type_t hash_type;
	hash_func_t hash_func;
	struct datacenter* local_dc;
	struct datacenter* remote_dc;
};

#ifdef __cplusplus
extern "C"{
#endif 

int dynoc_init(struct dynoc *dynoc);

/*
 * Set hash algorithm for key.
 * If caller does not call this API, dynoc will use default algorithm (murmur)
 * as key hash algorithm.
 */
int dynoc_hash_type_init(struct dynoc *dynoc, const char *hash_name);
int dynoc_datacenter_init(struct dynoc *dynoc, uint32_t rack_count, const char *name, dc_type_t dc_type);
int dynoc_rack_init(struct dynoc *dynoc, uint32_t node_count, const char *name, dc_type_t dc_type);
int dynoc_add_node(struct dynoc *dynoc, const char *ip, int port, const char *pass, const char *token, const char *rc_name, dc_type_t dc_type);
int dynoc_start(struct dynoc *dynoc);
void dynoc_destroy(struct dynoc *dynoc);


int dynoc_set(struct dynoc *dynoc, const char *key, const char *value);
int dynoc_setex(struct dynoc *dynoc, const char *key, const char *value, int seconds);
int dynoc_psetex(struct dynoc *dynoc, const char *key, const char *value, int milliseconds);
int dynoc_del(struct dynoc *dynoc, const char *key);
int dynoc_hset(struct dynoc *dynoc, const char *key, const char *field, const char *value);
int dynoc_incr(struct dynoc *dynoc, const char *key);
int dynoc_incrby(struct dynoc *dynoc, const char *key, int val);

redisReply *dynoc_get(struct dynoc *dynoc, const char *key);
redisReply *dynoc_hget(struct dynoc *dynoc, const char *key, const char *field);

#ifdef __cplusplus
}
#endif

