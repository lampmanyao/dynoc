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

#include <unistd.h>

int main(int argc, char **argv)
{
	int i;
	struct dynoc dynoc;
	dynoc_init(&dynoc);

	dynoc_hash_type_init(&dynoc, "murmur");

	dynoc_datacenter_init(&dynoc, 1, "wuxi-datacenter", LOCAL_DC);

	dynoc_rack_init(&dynoc, 1, "rack1", LOCAL_DC);
	dynoc_add_node(&dynoc, "10.211.55.19", 8102, "hello", "437425602", "rack1", LOCAL_DC);

	dynoc_start(&dynoc);

	for (i = 0; i < 10; i++) {
		char key[32];
		char value[32];
		snprintf(key, 32, "keykey%d", i);
		snprintf(value, 32, "vlaue%d", i);
		int ret = dynoc_set(&dynoc, key, value);
		printf("SET %s: ret=%d\n", key, ret);
	}

	for (i = 0; i < 10; i++) {
		char key[32];
		snprintf(key, 32, "keykey%d", i);
		redisReply *r = dynoc_get(&dynoc, key);
		if (r && r->str) {
			printf("GET %s: %s\n\n", key, r->str);
			freeReplyObject(r);
		} else {
			printf("GET %s: failed\n\n", key);
		}
	}

	for (i = 0; i < 100; i++) {
		char key[32];
		char field1[32];
		char field2[32];
		char val1[32];
		char val2[32];
		snprintf(key, 32, "hkey%d", i);
		snprintf(field1, 32, "field1_%d", i);
		snprintf(val1, 32, "val1_%d", i);
		snprintf(field2, 32, "field2_%d", i);
		snprintf(val2, 32, "val2_%d", i);

		int ret = dynoc_hset(&dynoc, key, field1, val1);
		int ret1 = dynoc_hset(&dynoc, key, field2, val2);
		printf("HSET %s: ret=%d, ret1=%d\n", key, ret, ret1);
	}

	for (i = 0; i < 100; i++) {
		char key[32];
		char field1[32];
		char field2[32];
		snprintf(key, 32, "hkey%d", i);
		snprintf(field1, 32, "field1_%d", i);
		snprintf(field2, 32, "field2_%d", i);

		redisReply *r1 = dynoc_hget(&dynoc, key, field1);
		redisReply *r2 = dynoc_hget(&dynoc, key, field2);

		if (r1 && r1->str) {
			printf("HGET %s: %s\n", field1, r1->str);
			freeReplyObject(r1);
		}
		if (r2 && r2->str) {
			printf("HGET %s: %s\n", field2, r2->str);
			freeReplyObject(r2);
		}
	}

	dynoc_set(&dynoc, "mykey", "10");
	dynoc_incr(&dynoc, "mykey");

	while (1) {
		sleep(6);
	}

	dynoc_destroy(&dynoc);

	return 0;
}

