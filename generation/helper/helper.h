#ifndef GENERATION_INTERNAL_H
#define GENERATION_INTERNAL_H

#include "../generation.h"
#include "../../libs/sds.h"
#include "../../libs/khashl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void emit(gen_data* g, const char* fmt, ...);
inline int slot_to_offset(int slot_index);
int next_label(void);

void collect_vars_in_stmt(const NodeStmt* stmt, gen_data* g);
void collect_vars_in_expr(const NodeExpr* expr, gen_data* g);
void assign_slots_in_stmt(const NodeStmt* stmt, gen_data* g, int* next_slot);

void gen_stmt(gen_data* g, const NodeStmt* stmt);
void gen_expr_to_rax(gen_data* g, const NodeExpr* expr);
void gen_bindexpr_to_rax(gen_data* g, const BindExprRec rec);
void gen_binexpr_to_rax(gen_data* g, const BinExpr* b);

void ensure_var_slot(gen_data* g, const char* name);
void collect_vars(const NodeProg* prog, gen_data* g);

int lookup_var_slot(gen_data* g, const char* name);

void delete_local_var(gen_data *g);
void remove_last_block(gen_data *g);

#endif
