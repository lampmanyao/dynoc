#pragma once

#include <hiredis.h>

#include "dynoc-core.h"
#include "hash-murmur.h"
#include "dynoc-token.h"

int set(struct dynoc_hiredis_client *client, const char *key, const char *value);
redisReply *get(struct dynoc_hiredis_client *client, const char *key);

int hset(struct dynoc_hiredis_client *client, const char *key, const char *field, const char *value);
redisReply *hget(struct dynoc_hiredis_client *client, const char *key, const char *field);
