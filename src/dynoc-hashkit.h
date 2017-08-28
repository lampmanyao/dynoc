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

#include <stdint.h>
#include <stddef.h>

typedef int (*hash_func_t)(const char *, size_t);

#define HASH_CODEC(ACTION)                        \
	ACTION(HASH_ONE_AT_A_TIME, one_at_a_time) \
	ACTION(HASH_MD5, md5)                     \
	ACTION(HASH_CRC16, crc16)                 \
	ACTION(HASH_CRC32, crc32)                 \
	ACTION(HASH_CRC32A, crc32a)               \
	ACTION(HASH_FNV1_64, fnv1_64)             \
	ACTION(HASH_FNV1A_64, fnv1a_64)           \
	ACTION(HASH_FNV1_32, fnv1_32)             \
	ACTION(HASH_FNV1A_32, fnv1a_32)           \
	ACTION(HASH_HSIEH, hsieh)                 \
	ACTION(HASH_MURMUR, murmur)               \
	ACTION(HASH_JENKINS, jenkins)             \
	ACTION(HASH_MURMUR3, murmur3)             \

#define DEFINE_ACTION(_hash, _name) _hash,
typedef enum hash_type {
	HASH_CODEC(DEFINE_ACTION)
	HASH_INVALID
} hash_type_t;
#undef DEFINE_ACTION

hash_func_t get_hash_func(hash_type_t hash_type);
hash_type_t get_hash_type(const char *hash_name);

