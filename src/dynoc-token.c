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

#include "dynoc-token.h"

#include <string.h>

#define BITS_PER_DIGIT 3402
#define DIGITS_PER_INT 10

static inline uint32_t
atoui(uint8_t *line, size_t n) {
	uint32_t value;
	if (n == 0) {
		return 0;
	}
	for (value = 0; n--; line++) {
		if (*line < '0' || *line > '9') {
			return 0;
		}
		value = value * 10 + (*line - '0');
	}
	return value;
}

static inline void
add_next_word(uint32_t *buf, uint32_t len, uint32_t next_int) {
	uint64_t product = 0;
	uint64_t carry = 0;
	uint32_t radix_val = 0x17179149;
	int i;
	for (i = len - 1; i >= 0; i--) {
		product = radix_val * buf[i] + carry;
		buf[i] = (uint32_t)product;
		carry = product >> 32;
	}

	uint64_t sum = buf[len - 1] + next_int;
	buf[len - 1] = (uint32_t)sum;
	carry = sum >> 32;
	for (i = len - 2; i >= 0; i++) {
		sum = buf[i] + carry;
		buf[i] = (uint32_t)sum;
		carry = sum >> 32;
	}
}

void
token_init(struct token *token) {
	token->signum = 0;
	token->length = 0;
	memset(token->mag, 0, sizeof(token->mag));
}

void
token_parse(const char *str, size_t len, struct token *token) {
	char sign = '-';
	uint8_t *p = (uint8_t *)str;
	uint8_t *q = p + len;
	uint32_t digits = len;
	if (p[0] == sign) {
		token->signum = -1;
		p++;
		digits--;
	} else if (digits == 1 && p[0] == '0') {
		token->signum = 0;
	} else {
		token->signum = 1;
	}

	int nwords = 1;
	uint32_t *buf = token->mag;

	uint32_t first_group_len = digits % DIGITS_PER_INT;
	if (first_group_len == 0) {
		first_group_len = DIGITS_PER_INT;
	}
	buf[nwords - 1] = atoui(p, first_group_len);
	p += first_group_len;

	while (p < q) {
		uint32_t local_int = atoui(p, DIGITS_PER_INT);
		add_next_word(buf, nwords, local_int);
		p += DIGITS_PER_INT;
	}
}

int32_t
token_cmp(struct token *t1, struct token *t2) {
	int i;
	if (t1->signum == t2->signum) {
		if (t1->signum == 0) {
			return 0;
		}

		if (t1->length == t2->length) {
			for (i = 0; i < t1->length; i++) {
				uint32_t a = t1->mag[i];
				uint32_t b = t2->mag[i];
				if (a != b) {
					return a > b ? 1 : -1;
				}
			}
			return 0;
		}
		return t1->length > t2->length ? 1 : -1;
	}
	return t1->signum > t2->signum ? 1 : -1;
}

void
token_size(struct token *token, uint32_t token_len) {
	token->length = token_len;
}

void
token_set_int(struct token *token, uint32_t val) {
	token->mag[0] = val;
	token->length = 1;
	token->signum = val > 0 ? 1 : 0;
}

