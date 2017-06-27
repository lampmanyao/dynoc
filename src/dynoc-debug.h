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

