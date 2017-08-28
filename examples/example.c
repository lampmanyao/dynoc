#include "dynoc-core.h"
#include "dynoc-cmd.h"

#include <unistd.h>

int main(int argc, char **argv)
{
	int i;
	struct dynoc_hiredis_client client;
	dynoc_client_init(&client, "murmur");

	init_datacenter(&client, 1, "wuxi-datacenter", LOCAL_DC);

	init_rack(&client, 2, "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8301, "hello", "1431655765", "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8302, "hello", "3434343432", "rack1", LOCAL_DC);

	dynoc_client_start(&client);

	int n = 10;
	while (n--) {
		printf("%d\n", n);
		sleep(1);
	}

	for (i = 0; i < 10; i++) {
		char key[32];
		char value[32];
		snprintf(key, 32, "keykey%d", i);
		snprintf(value, 32, "vlaue%d", i);
		int ret = dynoc_set(&client, key, value);
		printf("SET %s: ret=%d\n", key, ret);
	}

	n = 10;
	while (n--) {
		printf("%d\n", n);
		sleep(1);
	}

	for (i = 0; i < 10; i++) {
		char key[32];
		snprintf(key, 32, "keykey%d", i);
		redisReply *r = dynoc_get(&client, key);
		if (r) {
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

		int ret = dynoc_hset(&client, key, field1, val1);
		int ret1 = dynoc_hset(&client, key, field2, val2);
		printf("HSET %s: ret=%d, ret1=%d\n", key, ret, ret1);
	}

	for (i = 0; i < 100; i++) {
		char key[32];
		char field1[32];
		char field2[32];
		snprintf(key, 32, "hkey%d", i);
		snprintf(field1, 32, "field1_%d", i);
		snprintf(field2, 32, "field2_%d", i);

		redisReply *r1 = dynoc_hget(&client, key, field1);
		redisReply *r2 = dynoc_hget(&client, key, field2);

		if (r1) {
			printf("HGET %s: %s\n", field1, r1->str);
			freeReplyObject(r1);
		}
		if (r2) {
			printf("HGET %s: %s\n", field2, r2->str);
			freeReplyObject(r2);
		}
	}

	while (1) {
		
		sleep(6);
	}

	dynoc_client_destroy(&client);

	return 0;
}

