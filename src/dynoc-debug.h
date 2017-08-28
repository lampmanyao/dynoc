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

#include <stdio.h>

#if defined DEBUG
#define log_debug(fmt, args ...) do { \
	fprintf(stderr, "[%s:%u:%s()] debug " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
	fflush(stderr); \
} while (0)
#else
#define log_debug(fmt, args ...)
#endif

