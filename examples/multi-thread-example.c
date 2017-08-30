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

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

struct dynoc dynoc;
int nrequest;

unsigned long
gettimeus() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void *
thread(void *arg) {
	struct dynoc *local_dynoc = &dynoc;
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
		int ret = dynoc_set(local_dynoc, key, value);
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

	dynoc_init(&dynoc);
	dynoc_datacenter_init(&dynoc, 1, "local_dc", LOCAL_DC);
	dynoc_datacenter_init(&dynoc, 2, "remote_dc", REMOTE_DC);

	dynoc_rack_init(&dynoc, 3, "rack1", LOCAL_DC);
	dynoc_add_node(&dynoc, "113.107.149.188", 6501, "1024xxx!yy", "1431655765", "rack1", LOCAL_DC);
	dynoc_add_node(&dynoc, "113.107.149.188", 6502, "1024xxx!yy", "2863311530", "rack1", LOCAL_DC);
	dynoc_add_node(&dynoc, "113.107.149.188", 6503, "1024xxx!yy", "4294967295", "rack1", LOCAL_DC);

	dynoc_rack_init(&dynoc, 3, "rack1", REMOTE_DC);
	dynoc_add_node(&dynoc, "61.145.54.225", 6501, "1024xxx!yy", "1431655765", "rack1", REMOTE_DC);
	dynoc_add_node(&dynoc, "61.145.54.225", 6502, "1024xxx!yy", "2863311530", "rack1", REMOTE_DC);
	dynoc_add_node(&dynoc, "61.145.54.225", 6503, "1024xxx!yy", "4294967295", "rack1", REMOTE_DC);
                                                              
	dynoc_rack_init(&dynoc, 3, "rack2", REMOTE_DC);            
	dynoc_add_node(&dynoc, "58.215.169.12", 6501, "1024xxx!yy", "1431655765", "rack2", REMOTE_DC);
	dynoc_add_node(&dynoc, "58.215.169.12", 6502, "1024xxx!yy", "2863311530", "rack2", REMOTE_DC);
	dynoc_add_node(&dynoc, "58.215.169.12", 6503, "1024xxx!yy", "4294967295", "rack2", REMOTE_DC);

	dynoc_start(&dynoc);

	pthread_t* tids = malloc(nthread * sizeof(pthread_t));
	int i;
	for (i = 0; i < nthread; i++) {
		pthread_create(&tids[i], NULL, thread, NULL);
	}

	for (i = 0; i < nthread; i++) {
		pthread_join(tids[i], NULL);
	}

	free(tids);
	dynoc_destroy(&dynoc);

	return 0;
}

