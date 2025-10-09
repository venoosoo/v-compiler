#include "tokenizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define INVALID_CHAR '\xFF'

// ---------- Token_data helpers ----------

Token_data* tokenizer_create(const char* src) {
    Token_data* t = (Token_data*)malloc(sizeof(Token_data));
    t->m_index = 0;
    t->m_src = strdup(src);
    t->m_buf[0] = '\0';
    t->m_len = 0;
    return t;
}

static inline char peek(Token_data* t, int offset) {
    size_t pos = t->m_index + offset;
    if (!t->m_src || pos >= strlen(t->m_src)) return INVALID_CHAR;
    return t->m_src[pos];
}

static inline char consume(Token_data* t) {
    if (!t->m_src || t->m_index >= strlen(t->m_src)) return INVALID_CHAR;
    return t->m_src[t->m_index++];
}

static inline void push_back(Token_data* t, char c) {
    if (t->m_len < BUF_SIZE - 1) {
        t->m_buf[t->m_len++] = c;
        t->m_buf[t->m_len] = '\0';  // null terminate
    }
}

// ---------- Push token functions ----------

void push_token_value(Token_data* t, TokenArray* tokens, TokenType type) {
    if (t->m_len == 0) return;

    Token tok;
    tok.type = type;
    tok.value = strndup(t->m_buf, t->m_len);  // allocate string
    kv_push(Token, *tokens, tok);

    t->m_len = 0;
    t->m_buf[0] = '\0';
}

void push_token(Token_data* t, TokenArray* tokens, TokenType type) {
    Token tok;
    tok.type = type;
    tok.value = NULL;
    kv_push(Token, *tokens, tok);

    t->m_len = 0;
    t->m_buf[0] = '\0';
}

// ---------- Tokenizer ----------

TokenArray tokenize(Token_data* t) {
    TokenArray tokens;
    kv_init(tokens);

    char c;
    while ((c = peek(t, 0)) != INVALID_CHAR) {
        if (isalpha(c)) {
            // read identifier / keyword
            push_back(t, consume(t));
            while (isalnum(peek(t,0))) {
                push_back(t, consume(t));
            }
            if (strcmp(t->m_buf, "exit") == 0) {
                push_token(t, &tokens, token_type_exit_kw);
            } else if (strcmp(t->m_buf, "if") == 0) {
                push_token(t, &tokens, token_type_if);
            } else if (strcmp(t->m_buf, "else") == 0) {
                push_token(t, &tokens, token_type_else);
            } else if (strcmp(t->m_buf, "for") == 0) {
                push_token(t, &tokens, token_type_for);
            } else if (strcmp(t->m_buf, "while") == 0) {
                push_token(t, &tokens, token_type_while);
            } else if (strcmp(t->m_buf, "for") == 0) {
                push_token(t, &tokens, token_type_for);
            } else if (strcmp(t->m_buf, "int") == 0) {
                push_token(t, &tokens, token_type_int);
            } else if (strcmp(t->m_buf, "char") == 0) {
                push_token(t, &tokens, token_type_char_t);
            } else {
                push_token_value(t, &tokens, token_type_ident);
            }

        } else if (isdigit(c)) {
            // read number literal
            push_back(t, consume(t));
            while (isdigit(peek(t,0))) {
                push_back(t, consume(t));
            }
            push_token_value(t, &tokens, token_type_int_lit);

        } else {
            consume(t); 
            switch (c) {
                case '(': push_token(t, &tokens, token_type_open_paren); break;
                case ')': push_token(t, &tokens, token_type_close_paren); break;
                case ';': push_token(t, &tokens, token_type_semi); break;
                case '=': 
                    
                    if (peek(t,0) != INVALID_CHAR && peek(t,0) == '=') {
                        push_token(t, &tokens, token_type_cmp);
                        consume(t); // consume second equal
                         break;
                    } else {
                        push_token(t, &tokens, token_type_eq_kw); break;
                    }
                case '<':
                    if (peek(t,0) != INVALID_CHAR && peek(t,0) == '=') {
                        push_token(t, &tokens, token_type_less_eq);
                        consume(t); // consume second equal
                         break;
                    } else {
                        push_token(t, &tokens, token_type_less); break;
                    }
                case '>':
                    if (peek(t,0) != INVALID_CHAR && peek(t,0) == '=') {
                        push_token(t, &tokens, token_type_more_eq);
                        consume(t); // consume second equal
                         break;
                    } else {
                        push_token(t, &tokens, token_type_more); break;
                    }
                case '!':
                    if (peek(t,0) != INVALID_CHAR && peek(t,0) == '=') {
                        push_token(t, &tokens, token_type_not_eq);
                        consume(t); // consume second equal
                         break;
                    } else {
                        printf("unexcpected !");
                        exit(1);
                    }
                case '\'':
                    push_back(t, consume(t));
                    consume(t);
                    push_token_value(t, &tokens, token_type_char_v);
                    break;
                case '+': push_token(t, &tokens, token_type_plus); break;
                case '*': push_token(t, &tokens, token_type_multi); break;
                case '/': push_token(t, &tokens, token_type_divide); break;
                case '-': push_token(t, &tokens, token_type_minus); break;
                case '{': push_token(t, &tokens, token_type_open_braces); break;
                case '}': push_token(t, &tokens, token_type_close_braces); break;                default: break; // ignore whitespace or unknown chars
            }
        }
    }

    return tokens;
}
