#include "dynoc-cmd.h"
#include "dynoc-debug.h"

#include <assert.h>
#include <string.h>

static uint32_t
select_continuum(struct continuum *continuum, uint32_t ncontinuum, struct token *token) {
	struct continuum *left, *right, *middle;

	left = continuum;
	right = continuum + ncontinuum - 1;

	if (cmp_token(right->token, token) < 0 || cmp_token(left->token, token) >= 0) {
		return left->index;
	}

	while (left < right) {
		middle = left + (right - left) / 2;
		int32_t cmp = cmp_token(middle->token, token);
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
select_connection(struct dynoc_hiredis_client *client, const char *key, struct token *token, dc_type_t *dc_type, uint32_t *rc_idx) {
	uint32_t index, hash;
	struct datacenter *dc;
	struct rack *rack;

	parse_token(key, strlen(key), token);
	hash = hash_murmur(key, strlen(key));
	size_token(token, 1);
	set_int_token(token, hash);

	dc = dc_type ? client->local_dc : client->remote_dc;

	if (!dc) {
		return NULL;
	}

	if (*rc_idx >= dc->rack_count) {
		if (*dc_type == LOCAL_DC) {
			*dc_type = REMOTE_DC;
			*rc_idx = 0;
			dc = client->remote_dc;
		} else {
			return NULL;
		}
	}
	rack = &dc->rack[(*rc_idx)++];
	index = select_continuum(rack->continuum, rack->ncontinuum, token);
	return &rack->redis_conn_pool[index];
}

int
set(struct dynoc_hiredis_client *client, const char *key, const char *value) {
	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	init_token(&token);

	while ((redis_conn = select_connection(client, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);
		if (redis_conn->valid) {
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

redisReply *
get(struct dynoc_hiredis_client *client, const char *key) {
	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply = NULL;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	init_token(&token);

	while ((redis_conn = select_connection(client, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);

		if (redis_conn->valid) {
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
hset(struct dynoc_hiredis_client *client, const char *key, const char *field, const char *value) {
	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	init_token(&token);

	while ((redis_conn = select_connection(client, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);

		if (redis_conn->valid) {
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
hget(struct dynoc_hiredis_client *client, const char *key, const char *field) {
	struct token token;
	struct redis_connection *redis_conn;
	redisReply *reply;
	dc_type_t dc_type = LOCAL_DC;
	uint32_t rc_idx = 0;

	init_token(&token);

	while ((redis_conn = select_connection(client, key, &token, &dc_type, &rc_idx))) {
		pthread_mutex_lock(&redis_conn->lock);

		if (redis_conn->valid) {
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

