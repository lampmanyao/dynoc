#include "dynoc-core.h"
#include "dynoc-cmd.h"

#include <unistd.h>

int main(int argc, char **argv)
{
	struct dynoc_hiredis_client client;
	dynoc_client_init(&client);

	init_datacenter(&client, 2, "wuxi-datacenter", LOCAL_DC);
	init_datacenter(&client, 1, "shenzhen-datacenter", REMOTE_DC);

	init_rack(&client, 3, "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8305, "1431655765", "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8306, "2863311530", "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8307, "4294967295", "rack1", LOCAL_DC);

	init_rack(&client, 3, "rack2", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8204, "1431655765", "rack2", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8205, "2863311530", "rack2", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8207, "4294967295", "rack2", LOCAL_DC);

	init_rack(&client, 3, "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "10.211.55.17", 8305, "1431655765", "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "10.211.55.17", 8306, "2863311530", "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "10.211.55.17", 8307, "4294967295", "rack1", REMOTE_DC);

	dynoc_client_start(&client);

	for (int i = 0; i < 10; i++) {
		char key[32];
		char value[32];
		snprintf(key, 32, "keykey%d", i);
		snprintf(value, 32, "vlaue%d", i);
		set(&client, key, value);
	}

	int n = 10;
	while (n--) {
		printf("%d\n", n);
		sleep(1);
	}

	for (int i = 0; i < 10; i++) {
		char key[32];
		snprintf(key, 32, "keykey%d", i);
		redisReply *r = get(&client, key);
		if (r) {
			printf("GET %s: %s\n\n", key, r->str);
			freeReplyObject(r);
		} else {
			printf("GET %s: failed\n\n", key);
		}
	}

	while (1) {
		sleep(6);
	}

	dynoc_client_destroy(&client);

	return 0;
}

