/*
 * Dynoc is a minimalistic C client library for the dynomite.
 * Copyright (C) 2017 Lampman Yao (lampmanyao@gmail.com)
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

#include <hiredis.h>

#include "dynoc-core.h"
#include "dynoc-token.h"

#ifdef __cplusplus
extern "C"{
#endif 

int dynoc_set(struct dynoc_hiredis_client *client, const char *key, const char *value);
int dynoc_setex(struct dynoc_hiredis_client *client, const char *key, const char *value, int seconds);
int dynoc_psetex(struct dynoc_hiredis_client *client, const char *key, const char *value, int milliseconds);
redisReply *dynoc_get(struct dynoc_hiredis_client *client, const char *key);
int dynoc_del(struct dynoc_hiredis_client *client, const char *key);


int dynoc_hset(struct dynoc_hiredis_client *client, const char *key, const char *field, const char *value);
redisReply *dynoc_hget(struct dynoc_hiredis_client *client, const char *key, const char *field);

#ifdef __cplusplus
}
#endif

