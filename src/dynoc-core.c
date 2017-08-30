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

#include "dynoc-core.h"
#include "dynoc-debug.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void datacenter_destroy(struct datacenter *);
static void rack_destroy(struct rack *);
static void continuum_init(struct continuum *, const char *host, int port, const char *pass, const char *token_str, uint32_t idx);
static void continuum_destroy(struct continuum *);

static void
reconnect_datacenter(struct datacenter *dc) {
	struct rack *rack;
	struct redis_connection *redis_conn;
	struct continuum *continuum;
	redisContext *ctx;
	uint32_t i, j;
	int status = VALID;

	for (i = 0; i < dc->rack_count; i++) {
		rack = &dc->rack[i];
		for (j = 0; j < rack->node_count; j++) {
			redis_conn = &rack->redis_conn_pool[j];
			continuum = &rack->continuum[j];
			pthread_mutex_lock(&redis_conn->lock);

			if (redis_conn->status) {
				redisReply *reply = redisCommand(redis_conn->ctx, "PING");
				if (reply) {
					log_debug("%s:%s:%s:%d alive",
						dc->name, rack->name, continuum->endpoint.host, continuum->endpoint.port);
					freeReplyObject(reply);
				} else {
					redis_conn->status = INVALID;
					redisFree(redis_conn->ctx);
					redis_conn->ctx = NULL;
				}
			}

			if (!redis_conn->status) {
				struct timeval tv;
				tv.tv_sec = 3;
				tv.tv_usec = 0;
				ctx = redisConnectWithTimeout(continuum->endpoint.host, continuum->endpoint.port, tv);
				if (ctx == NULL || ctx->err) {
					if (ctx) {
						redisFree(ctx);
					}
					log_debug("%s:%s:%s:%d reconnect failed",
						dc->name, rack->name, continuum->endpoint.host, continuum->endpoint.port);
				} else {
					log_debug("%s:%s:%s:%d reconnect ok",
						dc->name, rack->name, continuum->endpoint.host, continuum->endpoint.port);
					if (continuum->endpoint.pass) {
						redisReply *reply = redisCommand(ctx, "AUTH %s", continuum->endpoint.pass);
						if (reply && reply->type != REDIS_REPLY_ERROR && ctx->err == 0) {
							status = VALID;
							log_debug("auth ok");
						} else {
							status = INVALID;
							if (reply) {
								freeReplyObject(reply);
							}
							log_debug("auth failed");
						}
					}
					redis_conn->status = status;
					redis_conn->ctx = ctx;
				}
			}
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}
}

static void *
reconnect_thread(void *arg) {
	struct dynoc *dynoc = arg;
	struct datacenter *dc;

	while (1) {
		dc = dynoc->local_dc;
		if (dc) {
			reconnect_datacenter(dc);
		}

		dc = dynoc->remote_dc;
		if (dc) {
			reconnect_datacenter(dc);
		}

		sleep(30);
	}
	return NULL;
}

int
dynoc_init(struct dynoc *dynoc) {
	dynoc->local_dc = NULL;
	dynoc->remote_dc = NULL;

	dynoc->hash_type = DEFAULT_HASH;
	return 0;
}

int
dynoc_hash_type_init(struct dynoc *dynoc, const char *hash_name) {
	if (!hash_name) {
		return -1;
	}

	dynoc->hash_type = get_hash_type(hash_name);
	if (dynoc->hash_type == HASH_INVALID) {
		log_debug("invalid hash name: %s. Use default hash aka murmur.", hash_name);
		dynoc->hash_type = DEFAULT_HASH;
	}

	return 0;
}

void
dynoc_destroy(struct dynoc *dynoc) {
	if (!dynoc) {
		return;
	}

	pthread_cancel(dynoc->tid);
	if (dynoc->local_dc) {
		datacenter_destroy(dynoc->local_dc);
		free(dynoc->local_dc);
	}

	if (dynoc->remote_dc) {
		datacenter_destroy(dynoc->remote_dc);
		free(dynoc->remote_dc);
	}
}

int
dynoc_add_node(struct dynoc *dynoc, const char *ip, int port,
               const char *pass, const char *token_str,
               const char *rc_name, dc_type_t dc_type) {
	struct datacenter *dc;
	struct rack *rack;
	struct continuum *continuum;
	uint32_t index, rc_count, i;

	dc = dc_type ? dynoc->local_dc : dynoc->remote_dc;
	rc_count = dc->rack_count;

	for (i = 0; i < rc_count; i++) {
		rack = &dc->rack[i];
		index = rack->ncontinuum;
		if (rack->name && strcmp(rack->name, rc_name) == 0) {
			continuum = &rack->continuum[index];
			continuum_init(continuum, ip, port, pass, token_str, index);
			rack->ncontinuum++;
		}
	}
	return 0;
}

static inline int
cmp(const void *t1, const void *t2) {
	const struct continuum *ct1 = t1, *ct2 = t2;
	return token_cmp(ct1->token, ct2->token);
}

static void
redis_connection_pool_init(struct rack *rack) {
	uint32_t i;
	int status = VALID;

	qsort(rack->continuum, rack->ncontinuum, sizeof(*rack->continuum), cmp);
	for (i = 0; i < rack->ncontinuum; i++) {
		struct continuum *continuum = &rack->continuum[i];
		struct redis_connection *redis_conn = &rack->redis_conn_pool[i];
		continuum->index = i;
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		redisContext *ctx = redisConnectWithTimeout(continuum->endpoint.host, continuum->endpoint.port, tv);
		if (ctx == NULL || ctx->err) {
			if (ctx) {
				redisFree(ctx);
			}
			log_debug("connect to %s:%d failed", continuum->endpoint.host, continuum->endpoint.port);
		} else {
			log_debug("connect to %s:%d ok", continuum->endpoint.host, continuum->endpoint.port);
			if (continuum->endpoint.pass) {
				redisReply *reply = redisCommand(ctx, "AUTH %s", continuum->endpoint.pass);
				if (reply && reply->type != REDIS_REPLY_ERROR && ctx->err == 0) {
					status = VALID;
					log_debug("auth ok");
				} else {
					if (reply) {
						freeReplyObject(reply);
					}
					status = INVALID;
					log_debug("auth failed");
				}
			}

			redis_conn->status = status;
			redis_conn->ctx = ctx;
		}
	}
}

int
dynoc_start(struct dynoc *dynoc) {
	struct datacenter *dc;
	uint32_t rc_count, i;

	dynoc->hash_func = get_hash_func(dynoc->hash_type);

	dc = dynoc->local_dc;
	if (dc) {
		rc_count = dc->rack_count;
		for (i = 0; i < rc_count; i++) {
			redis_connection_pool_init(&dc->rack[i]);
		}
	}

	dc = dynoc->remote_dc;
	if (dc) {
		rc_count = dc->rack_count;
		for (i = 0; i < rc_count; i++) {
			redis_connection_pool_init(&dc->rack[i]);
		}
	}

	pthread_create(&dynoc->tid, NULL, reconnect_thread, dynoc);
	return 0;
}

int
dynoc_datacenter_init(struct dynoc *dynoc, uint32_t rack_count, const char *name, dc_type_t dc_type) {
	struct datacenter *dc;
	dc = dc_type ? dynoc->local_dc : dynoc->remote_dc;

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
			dynoc->local_dc = dc;
		} else {
			dynoc->remote_dc = dc;
		}
	}
	return 0;
}

static void
datacenter_destroy(struct datacenter *dc) {
	if (dc->name) {
		free(dc->name);
	}

	uint32_t i;
	for (i = 0; i < dc->rack_count; i++) {
		rack_destroy(&dc->rack[i]);
	}
	free(dc->rack);
}

int
dynoc_rack_init(struct dynoc *dynoc, uint32_t node_count, const char *name, dc_type_t dc_type) {
	uint32_t count, i, j;
	struct datacenter *dc;
	struct rack *rack;

	dc = dc_type ? dynoc->local_dc : dynoc->remote_dc;
	count = dc->rack_count;

	for (i = 0; i < count; i++) {
		rack = &dc->rack[i];
		if (!rack->name) {
			rack->name = strdup(name);
			rack->node_count = node_count;
			rack->ncontinuum = 0;
			rack->continuum = calloc(node_count, sizeof(struct continuum));
			rack->redis_conn_pool = calloc(node_count, sizeof(struct redis_connection));

			for (j = 0; j < node_count; j++) {
				pthread_mutex_init(&rack->redis_conn_pool[j].lock, NULL);
				rack->redis_conn_pool[j].status = INVALID;
				rack->redis_conn_pool[j].ctx = NULL;
			}
			break;
		}
	}
	return 0;
}

static void
rack_destroy(struct rack *rack) {
	uint32_t i;

	if (rack->name) {
		free(rack->name);
	}

	if (rack->continuum) {
		for (i = 0; i < rack->ncontinuum; i++) {
			continuum_destroy(&rack->continuum[i]);
		}
		free(rack->continuum);
	}

	if (rack->redis_conn_pool) {
		for (i = 0; i < rack->node_count; i++) {
			if (rack->redis_conn_pool[i].ctx) {
				pthread_mutex_destroy(&rack->redis_conn_pool[i].lock);
				redisFree(rack->redis_conn_pool[i].ctx);
			}
		}
		free(rack->redis_conn_pool);
	}
}

static void
continuum_init(struct continuum *continuum, const char *ip, int port, const char *pass, const char *token_str, uint32_t index) {
	struct token *token = malloc(sizeof(struct token));
	token_init(token);
	token_size(token, 1);
	token_parse(token_str, strlen(token_str), token);
	continuum->index = index;
	continuum->token = token;
	continuum->endpoint.host = strdup(ip);
	continuum->endpoint.port = port;
	if (pass) {
		continuum->endpoint.pass = strdup(pass);
	} else {
		continuum->endpoint.pass = NULL;
	}
} 

static void
continuum_destroy(struct continuum *continuum) {
	free(continuum->endpoint.host);
	if (continuum->endpoint.pass) {
		free(continuum->endpoint.pass);
	}
	free(continuum->token);
}

