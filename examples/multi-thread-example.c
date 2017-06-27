#include "dynoc-core.h"
#include "dynoc-cmd.h"

#include <unistd.h>
#include <pthread.h>

void *
thread1(void *arg) {
	struct dynoc_hiredis_client *client = arg;
	int i;

	for (i = 0; i < 100; i++) {
		char key[32];
		char value[32];
		snprintf(key, 32, "thread1%d", i);
		snprintf(value, 32, "vlaue%d", i);
		int ret = set(client, key, value);
		printf("thread1: SET %s: ret=%d\n", key, ret);
	}

	return NULL;
}

void *
thread2(void *arg) {
	struct dynoc_hiredis_client *client = arg;
	int i;

	for (i = 0; i < 100; i++) {
		char key[32];
		char value[32];
		snprintf(key, 32, "thread2%d", i);
		snprintf(value, 32, "vlaue%d", i);
		int ret = set(client, key, value);
		printf("thread2: SET %s: ret=%d\n", key, ret);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	struct dynoc_hiredis_client client;
	dynoc_client_init(&client);

	init_datacenter(&client, 2, "wuxi-datacenter", LOCAL_DC);
	init_datacenter(&client, 1, "shenzhen-datacenter", REMOTE_DC);

	init_rack(&client, 3, "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8306, "2863311530", "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8305, "1431655765", "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8307, "4294967295", "rack1", LOCAL_DC);

	init_rack(&client, 3, "rack2", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8207, "4294967295", "rack2", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8205, "2863311530", "rack2", LOCAL_DC);
	dynoc_client_add_node(&client, "10.211.55.8", 8204, "1431655765", "rack2", LOCAL_DC);

	init_rack(&client, 3, "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "10.211.55.17", 8306, "2863311530", "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "10.211.55.17", 8307, "4294967295", "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "10.211.55.17", 8305, "1431655765", "rack1", REMOTE_DC);

	dynoc_client_start(&client);

	pthread_t tids[2];
	pthread_create(&tids[0], NULL, thread1, &client);
	pthread_create(&tids[1], NULL, thread2, &client);

	int i;
	for (i = 0; i < 2; i++) {
		pthread_join(tids[i], NULL);
	}

	dynoc_client_destroy(&client);

	return 0;
}

