#pragma once

#include "../libs/sds.h"
#include "../libs/khashl.h"
#include "../parser/parser.h"
#include <string.h>
#include <stdlib.h>

/* value struct (typedef so name is simple) */
typedef struct var {
    size_t stack_pos;
} var;

/*
 * Instantiate a khashl map here.
 *
 * - scope: KH_LOCAL (functions are local/static)
 * - table typedef name: str_var_t
 * - prefix (function name prefix): str_var
 * - key type: const char*
 * - value type: var*   <-- we'll store pointers to allocated var
 * - hash/equal: kh_hash_str / kh_eq_str (provided by khashl)
 *
 * NOTE: keep this invocation only in this header (remove any duplicate in .c).
 */
KHASHL_MAP_INIT(KH_LOCAL, str_var_t, str_var,
                const char*,   /* key type */
                var*,          /* value type (pointer to var) */
                kh_hash_str,   /* string hash provided by khashl */
                kh_eq_str)     /* string equality provided by khashl */
typedef kvec_t(char*) StrVec;
// vector of vector-of-strings
typedef kvec_t(StrVec) Str2DVec;
/* gen_data */
typedef struct gen_data {
    const NodeProg* m_prog;   /* pointer to parsed program */
    size_t m_stack_pos;
    sds m_output;
    str_var_t *m_vars;        /* map instance pointer (returned by str_var_init()) */
    Str2DVec *m_block; // for local visibility
} gen_data;





/* functions */
gen_data* generate_gen_data(const NodeProg* root);
char* gen_prog(gen_data* g);
void get_stmt(gen_data* g, const NodeStmt* stmt);
void get_expr(gen_data* g, const NodeExpr* expr);
void push(gen_data* g, const char* reg);
void pop(gen_data* g, const char* reg);
