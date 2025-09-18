#include "./generation.h"
#include "../libs/sds.h"
#include "../libs/khashl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


gen_data* generate_gen_data(const NodeProg* root) {
    gen_data* g = malloc(sizeof(gen_data));
    if (!g) { perror("malloc"); return NULL; }

    g->m_prog = root;
    g->m_stack_pos = 0;
    g->m_output = sdsempty();

    g->m_vars = str_var_init();
    if (!g->m_vars) { free(g); return NULL; }

    return g;
}

void push(gen_data* g, const char* reg) {
    g->m_output = sdscatprintf(g->m_output, "   push %s\n", reg);
    g->m_stack_pos++;
}

void pop(gen_data* g, const char* reg) {
    g->m_output = sdscatprintf(g->m_output, "   pop %s\n", reg);
    g->m_stack_pos--;
}

void get_stmt(gen_data* g, const NodeStmt* stmt) {
    if (!g || !stmt) return;

    if (stmt->kind == NODE_STMT_LET) {
        khint_t k = str_var_get(g->m_vars, stmt->as.let.ident.value);
        if (k != kh_end(g->m_vars)) {
            fprintf(stderr, "Variable already used: %s\n", stmt->as.let.ident.value);
            return;
        }

        int absent = 0;
        k = str_var_put(g->m_vars, stmt->as.let.ident.value, &absent);
        if (absent) {
            kh_key(g->m_vars, k) = strdup(stmt->as.let.ident.value);
            if (!kh_key(g->m_vars, k)) {
                perror("strdup");
                str_var_del(g->m_vars, k);
                return;
            }
            var* vp = malloc(sizeof(var));
            if (!vp) { perror("malloc"); str_var_del(g->m_vars, k); return; }
            vp->stack_pos = g->m_stack_pos;
            kh_val(g->m_vars, k) = vp;
            get_expr(g,&stmt->as.let.expr);
        }

    } else if (stmt->kind == NODE_STMT_EXIT) {
        get_expr(g, &stmt->as.exit_.expr);
        g->m_output = sdscatprintf(g->m_output, "   mov rax, 60\n");
        pop(g, "rdi");
        g->m_output = sdscatprintf(g->m_output, "   syscall\n");
        g->m_output = sdscatprintf(g->m_output, "   mov rdi, 0\n");
        g->m_output = sdscatprintf(g->m_output, "   syscall\n");
    } else {
        printf("Unkown statement");
        exit(1);
    }
}

void get_expr(gen_data* g, const NodeExpr* expr) {
    if (!g || !expr) return;

    if (expr->kind == NODE_EXPR_INT_LIT) {
        g->m_output = sdscatprintf(g->m_output, "   mov rax, %s\n", expr->as.int_lit.int_lit.value);
        push(g, "rax");

    } else if (expr->kind == NODE_EXPR_IDENT) {
        khint_t k = str_var_get(g->m_vars, expr->as.ident.ident.value);
        if (k == kh_end(g->m_vars)) {
            fprintf(stderr, "Undefined variable: %s\n", expr->as.ident.ident.value);
            return;
        }

        var* vptr = kh_val(g->m_vars, k);
        if (!vptr) {
            fprintf(stderr, "Internal error: variable has NULL value: %s\n", expr->as.ident.ident.value);
            return;
        }

        int offset = (int)((g->m_stack_pos - vptr->stack_pos) * 4);

        sds tmp = sdscatprintf(sdsempty(), "qword [rsp + %d]", offset);
        push(g, tmp);
        sdsfree(tmp);
    }
}

char* gen_prog(gen_data* g) {
    if (!g) return NULL;

    g->m_output = sdscatprintf(g->m_output, "global _start\n_start:\n");

    for (size_t i = 0; i < kv_size(g->m_prog->stmt); i++) {
        NodeStmt* stmt = &kv_A(g->m_prog->stmt, i);
        get_stmt(g, stmt);
    }

    return g->m_output;
}
