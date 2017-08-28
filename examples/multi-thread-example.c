#include "dynoc-core.h"
#include "dynoc-cmd.h"

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

struct dynoc_hiredis_client client;
int nrequest;

inline unsigned long
gettimeus() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void *
thread(void *arg) {
	struct dynoc_hiredis_client *local_client = &client;
	int i;

#ifdef __linux__
	int tid = syscall(__NR_gettid);
#else
	int tid = syscall(SYS_thread_selfid);
#endif
	unsigned long begin = gettimeus();

	for (i = 0; i < nrequest; i++) {
		char key[32];
		char value[32];
		snprintf(key, 32, "thread%d%d", tid, i);
		snprintf(value, 32, "vlaue%d", i);
		int ret = dynoc_set(local_client, key, value);
		if (ret < 0) {
			printf("thread1: SET %s: ret=%d\n", key, ret);
		}
	}
	unsigned long end = gettimeus();
	printf("thread %d exit. %d requests need %lu us.\n", tid, nrequest, end - begin);
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("%s nthread nrequest\n", argv[0]);
		return -1;
	}

	int nthread = atoi(argv[1]);
	nrequest = atoi(argv[2]);

	dynoc_client_init(&client, "murmur");
	init_datacenter(&client, 1, "local_dc", LOCAL_DC);
	init_datacenter(&client, 2, "remote_dc", REMOTE_DC);

	init_rack(&client, 3, "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "113.107.149.188", 6501, "1024xxx!yy", "1431655765", "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "113.107.149.188", 6502, "1024xxx!yy", "2863311530", "rack1", LOCAL_DC);
	dynoc_client_add_node(&client, "113.107.149.188", 6503, "1024xxx!yy", "4294967295", "rack1", LOCAL_DC);

	init_rack(&client, 3, "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "61.145.54.225", 6501, "1024xxx!yy", "1431655765", "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "61.145.54.225", 6502, "1024xxx!yy", "2863311530", "rack1", REMOTE_DC);
	dynoc_client_add_node(&client, "61.145.54.225", 6503, "1024xxx!yy", "4294967295", "rack1", REMOTE_DC);
                                                              
	init_rack(&client, 3, "rack2", REMOTE_DC);            
	dynoc_client_add_node(&client, "58.215.169.12", 6501, "1024xxx!yy", "1431655765", "rack2", REMOTE_DC);
	dynoc_client_add_node(&client, "58.215.169.12", 6502, "1024xxx!yy", "2863311530", "rack2", REMOTE_DC);
	dynoc_client_add_node(&client, "58.215.169.12", 6503, "1024xxx!yy", "4294967295", "rack2", REMOTE_DC);

	dynoc_client_start(&client);

	pthread_t* tids = malloc(nthread * sizeof(pthread_t));
	int i;
	for (i = 0; i < nthread; i++) {
		pthread_create(&tids[i], NULL, thread, NULL);
	}

	for (i = 0; i < nthread; i++) {
		pthread_join(tids[i], NULL);
	}

	free(tids);
	dynoc_client_destroy(&client);

	return 0;
}

