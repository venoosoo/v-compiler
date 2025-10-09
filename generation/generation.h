#pragma once

#include "../libs/sds.h"
#include "../parser/parser.h"
#include <string.h>
#include <stdlib.h>

typedef struct var {
    size_t stack_pos;
} var;


typedef struct {
    TokenType kind;       // token_type_int_lit, token_type_char_lit
    size_t size;          // size in bytes
    const char *asm_size; // "byte", "qword", etc.
} VarTypeInfo;


typedef struct stack_vars {
    char* name;
    Token type;
} stack_vars;

typedef kvec_t(char*) StrVec;
typedef kvec_t(stack_vars) StackVec;
// vector of vector-of-strings
typedef kvec_t(StrVec) Str2DVec;
/* gen_data */
typedef struct gen_data {
    const NodeProg* m_prog;   /* pointer to parsed program */
    size_t m_stack_pos;
    sds m_output;
    StackVec *m_vars;    // unordered map of all vars
    Str2DVec *m_block; // for local visibility
} gen_data;





/* functions */
gen_data* generate_gen_data(const NodeProg* root);
char* gen_prog(gen_data* g);
void get_stmt(gen_data* g, const NodeStmt* stmt);
void get_expr(gen_data* g, const NodeExpr* expr);
void push(gen_data* g, const char* reg);
void pop(gen_data* g, const char* reg);
