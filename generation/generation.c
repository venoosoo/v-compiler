
#include "./generation.h"
#include "../libs/sds.h"
#include "../libs/khashl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "./helper/helper.h"



void gen_stmt(gen_data* g, const NodeStmt* stmt) {
    if (!stmt) return;

    if (stmt->kind == NODE_STMT_LET) {

        if (kv_size(*g->m_block) > 0) {
            size_t last_row_index = kv_size(*g->m_block) - 1;
            StrVec *last_row = &kv_A(*g->m_block, last_row_index); // pointer to real inner vector
            char *s = strdup(stmt->as.let.ident.value);
            if (!s) { perror("strdup"); exit(1); }
            kv_push(char*, *last_row, s); // push into the actual inner vector
        }
        gen_expr_to_rax(g, &stmt->as.let.expr);
        int slot = lookup_var_slot(g, stmt->as.let.ident.value);
        int off = slot_to_offset(slot);
        emit(g, "   mov qword [rbp - %d], rax\n", off);
        return;
    }

    if (stmt->kind == NODE_STMT_EXIT) {
        gen_expr_to_rax(g, &stmt->as.exit_.expr);
        emit(g, "   mov rdi, rax\n");
        emit(g, "   mov rax, 60\n");
        emit(g, "   syscall\n");
        return;
    }

    if (stmt->kind == NODE_STMT_IF) {
        // evaluate condition -> rax ; test ; je skip
        gen_expr_to_rax(g, &stmt->as.if_.cond);
        emit(g, "   test rax, rax\n");
        int id = next_label();
        emit(g, "   je .L_if_end_%d\n", id);
        // body
        StrVec row;
        kv_init(row);
        kv_push(StrVec, *g->m_block, row);
        for (size_t i = 0; i < kv_size(stmt->as.if_.body); ++i) {
            gen_stmt(g, &kv_A(stmt->as.if_.body, i));
        }
        delete_local_var(g);
        remove_last_block(g);
        emit(g, ".L_if_end_%d:\n", id);
        return;
    }
    if (stmt->kind == NODE_STMT_ELSE) {
        int id = next_label();
        StrVec row;
        kv_init(row);
        kv_push(StrVec, *g->m_block, row);
        for (size_t i = 0; i < kv_size(stmt->as.else_.body); ++i) {
            gen_stmt(g, &kv_A(stmt->as.else_.body, i));
        }
        delete_local_var(g);
        remove_last_block(g);
        emit(g, ".L_else_end_%d:\n", id);
        return;
    }

    emit(g, "   ; gen_stmt: unknown stmt kind %d\n", stmt->kind);
}

// function for deleting local var when their block ends


gen_data* generate_gen_data(const NodeProg* root) {
    if (!root) return NULL;
    gen_data* g = malloc(sizeof(gen_data));
    if (!g) { perror("malloc"); return NULL; }

    // reset label counter to avoid leaking labels between invocations
    int __label_counter = 0;

    g->m_prog = root;
    g->m_stack_pos = 0;
    g->m_output = sdsempty();
    Str2DVec vec2d;
    g->m_block = malloc(sizeof(Str2DVec));
    kv_init(*g->m_block);

    g->m_vars = str_var_init();
    if (!g->m_vars) { free(g); return NULL; }

    // 1) collect var keys
    collect_vars(root, g);

    // 2) assign slots deterministically in source order (including nested lets)
    int next_slot = 0;
    for (size_t i = 0; i < kv_size(root->stmt); ++i) {
        assign_slots_in_stmt(&kv_A(root->stmt, i), g, &next_slot);
    }

    int slots = next_slot;

    // Compute bytes to reserve on stack: slots * 8, round up to 16 for alignment
    int bytes = slots * 8;
    if (bytes > 0) {
        // round up to multiple of 16
        bytes = ((bytes + 15) / 16) * 16;
    }

    // Emit prologue
    emit(g, "global _start\n");
    emit(g, "_start:\n");
    emit(g, "   push rbp\n");
    emit(g, "   mov rbp, rsp\n");
    if (bytes > 0) emit(g, "   sub rsp, %d\n", bytes);

    // Generate code for statements
    for (size_t i = 0; i < kv_size(root->stmt); ++i) {
        gen_stmt(g, &kv_A(root->stmt, i));
    }

    // Note: program usually exits via syscall in exit statements; if not, we still syscall(60) with rdi=0
    emit(g, "   mov rax, 60\n");
    emit(g, "   mov rdi, 0\n");
    emit(g, "   syscall\n");

    return g;
}

char* gen_prog(gen_data* g) {
    if (!g) return NULL;
    return g->m_output;
}

char* gen_prog_from_root(const NodeProg* root) {
    gen_data* g = generate_gen_data(root);
    if (!g) return NULL;
    return g->m_output;
}
