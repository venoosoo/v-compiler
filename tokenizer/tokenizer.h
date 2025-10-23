#pragma once

#include "../libs/kvec.h"
#include <stddef.h>

#define BUF_SIZE 256

typedef struct Token_data {
    int m_index;
    char* m_src;
    char m_buf[BUF_SIZE];
    size_t m_len;
} Token_data;

typedef enum {
    token_type_exit_kw,
    token_type_int_lit, // the int value
    token_type_semi,
    token_type_open_paren,
    token_type_close_paren,
    token_type_ident,
    token_type_int, //int type
    token_type_short,
    token_type_long,
    token_type_char_t, // char type
    token_type_char_v, // char value
    token_type_eq_kw,
    token_type_plus,
    token_type_multi,
    token_type_minus,
    token_type_divide,
    token_type_if,
    token_type_quote,
    token_type_else,
    token_type_open_braces,
    token_type_close_braces,
    token_type_cmp,
    token_type_less,
    token_type_less_eq,
    token_type_more,
    token_type_more_eq,
    token_type_not_eq,
    token_type_and,
    token_type_or,
    token_type_for,
    token_type_while,
    token_empty,

} TokenType;

typedef struct Token {
    TokenType type;
    char* value;
} Token;

typedef kvec_t(Token) TokenArray;

// Token_data functions
Token_data* tokenizer_create(const char* src);

// Token handling
void push_token(Token_data* t, TokenArray* tokens, TokenType type);
void push_token_value(Token_data* t, TokenArray* tokens, TokenType type);

// Tokenizer
TokenArray tokenize(Token_data* t);

// Inline helpers
// can be used only in tokenizer.c aka private functions
static inline char peek(Token_data* t, int offset);
static inline char consume(Token_data* t);
static inline void push_back(Token_data* t, char c);
