#pragma once

#include "../generation.h"
#include "../../libs/sds.h"
#include "../../libs/khashl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void emit(gen_data* g, const char* fmt, ...);
int slot_to_offset(gen_data* g,int slot_index);
int next_label(void);

typedef struct {
    const char *dst; // e.g. "rbx"
    const char *src; // e.g. "rax"
} RegPair;

void collect_vars_in_stmt(const NodeStmt* stmt, gen_data* g);
void collect_vars_in_expr(const NodeExpr* expr, gen_data* g);
void assign_slots_in_stmt(const NodeStmt* stmt, gen_data* g);

void gen_stmt(gen_data* g, const NodeStmt* stmt);
void gen_expr_to_rax(gen_data* g, const NodeExpr* expr, NodeStmt* kind);
void gen_bindexpr_to_rax(gen_data* g, const BindExprRec rec, NodeStmt* stmt);
void gen_binexpr_to_rax(gen_data* g, const BinExpr* b, NodeStmt* stmt);
void emit_move_to_ident(gen_data* g, int off, NodeStmt* stmt);
void emit_ident_to_move(gen_data* g, int off, int type);
void ensure_var_slot(gen_data* g, const char* name, int type);
void collect_vars(const NodeProg* prog, gen_data* g);
int get_type_by_name(gen_data* g, const char* name);
int lookup_var_slot(gen_data* g, const char* name);
RegPair get_regpair_for_stmt(int type);
void delete_local_var(gen_data *g);
void remove_last_block(gen_data *g);
VarTypeInfo get_type_info(TokenType t);

