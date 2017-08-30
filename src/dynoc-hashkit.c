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

#include "dynoc-hashkit.h"

#include <string.h>

#define DEFINE_ACTION(_hash, _name) #_name,
const char* hash_strings[] = {
	HASH_CODEC(DEFINE_ACTION)
	NULL
};
#undef DEFINE_ACTION

#define DEFINE_ACTION(_hash, _name) int hash_##_name(const char *key, size_t length);
HASH_CODEC(DEFINE_ACTION) \

#undef DEFINE_ACTION

#define DEFINE_ACTION(_hash, _name) hash_##_name,
static hash_func_t hash_algos[] = {
	HASH_CODEC(DEFINE_ACTION)
	NULL
};
#undef DEFINE_ACTION

hash_func_t
get_hash_func(hash_type_t hash_type) {
	if ((hash_type >= 0) && (hash_type < HASH_INVALID)) {
		return hash_algos[hash_type];
	}
	return NULL;
}

hash_type_t
get_hash_type(const char *hash_name) {
	hash_type_t i = HASH_ONE_AT_A_TIME;
	for (; i < HASH_INVALID; i++) {
		if (strcmp(hash_strings[i], hash_name) != 0) {
			continue;
		}

		return i;
	}

	return HASH_INVALID;
}

