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

#include "dynoc-debug.h"
#include "dynoc-core.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

static void
reset_redis_connection(struct redis_connection *redis_conn) {
	redis_conn->status = INVALID;
	redisFree(redis_conn->ctx);
	redis_conn->ctx = NULL;
}

static uint32_t
select_continuum(struct continuum *continuum, uint32_t ncontinuum, struct token *token) {
	struct continuum *left, *right, *middle;

	left = continuum;
	right = continuum + ncontinuum - 1;

	if (token_cmp(right->token, token) < 0 || token_cmp(left->token, token) >= 0) {
		return left->index;
	}

	while (left < right) {
		middle = left + (right - left) / 2;
		int32_t cmp = token_cmp(middle->token, token);
		if (cmp == 0) {
			return middle->index;
		} else if (cmp < 0) {
			left = middle + 1;
		} else {
			right = middle;
		}
	}

	return right->index;
}

static struct redis_connection*
select_connection(struct dynoc *dynoc, const char *key,
                  struct token *token, dc_type_t *dc_type, uint32_t *rc_idx) {
	uint32_t index, hash;
	struct datacenter *dc;
	struct rack *rack;

	token_parse(key, strlen(key), token);
	hash = dynoc->hash_func(key, strlen(key));
	token_size(token, 1);
	token_set_int(token, hash);

	dc = dc_type ? dynoc->local_dc : dynoc->remote_dc;

	if (!dc) {
		return NULL;
	}

	if (*rc_idx >= dc->rack_count) {
		if (*dc_type == LOCAL_DC) {
			*dc_type = REMOTE_DC;
			*rc_idx = 0;
			dc = dynoc->remote_dc;
		} else {
			return NULL;
		}
	}

	if (!dc) {
		return NULL;
	}

	rack = &dc->rack[(*rc_idx)++];
	index = select_continuum(rack->continuum, rack->ncontinuum, token);
	return &rack->redis_conn_pool[index];
}

int
dynoc_set(struct dynoc *dynoc, const char *key, const char *value) {
	if (!key || !value) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "SET %s %s", key, value);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

int
dynoc_setex(struct dynoc *dynoc, const char *key, const char *value, int seconds) {
	if (!key || !value) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "SETEX %s %d %s", key, seconds, value);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
				log_debug("redis is downed");
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
			log_debug("dynomite is downed");
		}
	}

	return -1;
}

int
dynoc_psetex(struct dynoc *dynoc, const char *key, const char *value, int milliseconds) {
	if (!key || !value) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "PSETEX %s %d %s", key, milliseconds, value);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

redisReply *
dynoc_get(struct dynoc *dynoc, const char *key) {
	if (!key) {
		return NULL;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply = NULL;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);

		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "GET %s", key);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				pthread_mutex_unlock(&redis_conn->lock);
				return reply;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return NULL;
}

int
dynoc_del(struct dynoc *dynoc, const char *key) {
	if (!key) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply = NULL;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);

		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "DEL %s", key);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

int
dynoc_hset(struct dynoc *dynoc, const char *key, const char *field, const char *value) {
	if (!key || !field || !value) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);

		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "HSET %s %s %s", key, field, value);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

redisReply *
dynoc_hget(struct dynoc *dynoc, const char *key, const char *field) {
	if (!key || !field) {
		return NULL;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);

		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "HGET %s %s", key, field);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				pthread_mutex_unlock(&redis_conn->lock);
				return reply;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return NULL;
}

int
dynoc_incr(struct dynoc *dynoc, const char *key) {
	if (!key) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "INCR %s", key);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

int
dynoc_incrby(struct dynoc *dynoc, const char *key, int val) {
	if (!key) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "INCRBY %s %d", key, val);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

int
dynoc_decr(struct dynoc *dynoc, const char *key) {
	if (!key) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "DECR %s", key);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

int
dynoc_decrby(struct dynoc *dynoc, const char *key, int val) {
	if (!key) {
		return -1;
	}

	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	token_init(&token);

	while ((redis_conn = select_connection(dynoc, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->status) {
			reply = redisCommand(redis_conn->ctx, "DECRBY %s %d", key, val);
			if (reply && reply->type != REDIS_REPLY_ERROR && redis_conn->ctx->err == 0) {
				freeReplyObject(reply);
				pthread_mutex_unlock(&redis_conn->lock);
				return 0;
			} else {
				if (reply) {
					freeReplyObject(reply);
				}
				reset_redis_connection(redis_conn);
				pthread_mutex_unlock(&redis_conn->lock);
			}
		} else {
			pthread_mutex_unlock(&redis_conn->lock);
		}
	}

	return -1;
}

