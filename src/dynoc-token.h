#pragma once

#include <stdint.h>
#include <stddef.h>

struct token {
	uint32_t signum;
	uint32_t length;
	uint32_t mag[4];
};

void init_token(struct token *token);
void parse_token(const char *str, size_t len, struct token *token);
int32_t cmp_token(struct token *t1, struct token *t2);
void set_int_token(struct token *token, uint32_t val);
void size_token(struct token *token, uint32_t token_len);

