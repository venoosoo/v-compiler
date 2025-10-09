#include "./helper.h"
#include "../../libs/sds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>





void emit(gen_data* g, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    sds out = sdscatvprintf(sdsempty(), fmt, ap);
    va_end(ap);
    g->m_output = sdscatprintf(g->m_output, "%s", out);
    sdsfree(out);
}

int slot_to_offset(gen_data* g,int slot_index) {
    int size = 0;
    for(int i = 0; i <= slot_index; i++) {
        NodeStmt stmt = kv_A(g->m_prog->stmt, i);
        switch(stmt.kind) {
            case NODE_STMT_INT: size+=8; break;
            case NODE_STMT_CHAR: size +=1; break;
        }
    }
    return size;
}
int __label_counter = 0;
int next_label(void) { return __label_counter++; }



// ------------------------
// Variable collection phase
//   - first pass: create hash entries for every let identifier (keys present and var allocated)
//   - second pass: assign slot indices (0..n-1) in source-order, including nested lets
// ------------------------
void ensure_var_slot(gen_data* g, const char* name) {
    if (!g || !name) return;

    // Check if the variable already exists
    for (size_t i = 0; i < kv_size(*g->m_vars); i++) {
        if (strcmp(kv_A(*g->m_vars, i).name, name) == 0) {
            return; // already present
        }
    }

    // Variable not found, add new slot
    stack_vars sv;
    sv.name = strdup(name);
    if (!sv.name) { perror("strdup"); exit(1); }

    kv_push(stack_vars, *g->m_vars, sv);
}


void collect_vars(const NodeProg* prog, gen_data* g) {
    if (!prog || !g) return;
    // Walk top-level statements to ensure every let identifier gets an entry in the hash.
    for (size_t i = 0; i < kv_size(prog->stmt); ++i) {
        const NodeStmt* s = &kv_A(prog->stmt, i);
        collect_vars_in_stmt(s, g);
    }
}

void collect_vars_in_stmt(const NodeStmt* stmt, gen_data* g) {
    if (!stmt) return;
    if (stmt->kind == NODE_STMT_INT) {
        ensure_var_slot(g, stmt->as.int_.ident.value);
        collect_vars_in_expr(&stmt->as.int_.expr, g);
    } else if (stmt->kind == NODE_STMT_CHAR) {
        printf("debug: %s\n", stmt->as.char_.ident.value);
        ensure_var_slot(g, stmt->as.char_.ident.value);
        collect_vars_in_expr(&stmt->as.char_.expr, g);
    } else if (stmt->kind == NODE_STMT_EXIT) {
        collect_vars_in_expr(&stmt->as.exit_.expr, g);
    } else if (stmt->kind == NODE_STMT_IF) {
        collect_vars_in_expr(&stmt->as.if_.cond, g);
        for (size_t i = 0; i < kv_size(stmt->as.if_.body); ++i) {
            collect_vars_in_stmt(&kv_A(stmt->as.if_.body, i), g);
        }
    } else if (stmt->kind == NODE_STMT_FOR) {
        collect_vars_in_stmt(stmt->as.for_.cond1,g);    
    }
}

void collect_vars_in_expr(const NodeExpr* expr, gen_data* g) {
    if (!expr) return;
    if (expr->kind == NODE_EXPR_INT_LIT) return;
    if (expr->kind == NODE_EXPR_IDENT) {
        // usage of an identifier doesn't implicitly create a "let"; we do not call ensure_var_slot here
        // to preserve the generator's behavior (undefined variable at codegen should be an error later).
        return;
    }
    if (expr->kind == NODE_EXPR_BIN) {
        const BinExpr* b = expr->as.bin;
        if (!b) return;
        switch (b->kind) {
            case BIN_EXPR_ADD:
                if (b->as.add.lhs.type == NODE_EXPR) collect_vars_in_expr(b->as.add.lhs.as.node_expr, g);
                if (b->as.add.rhs.type == NODE_EXPR) collect_vars_in_expr(b->as.add.rhs.as.node_expr, g);
                if (b->as.add.lhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.add.lhs.as.bin_expr } }, g);
                if (b->as.add.rhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.add.rhs.as.bin_expr } }, g);
                break;
            case BIN_EXPR_MINUS:
                if (b->as.minus.lhs.type == NODE_EXPR) collect_vars_in_expr(b->as.minus.lhs.as.node_expr, g);
                if (b->as.minus.rhs.type == NODE_EXPR) collect_vars_in_expr(b->as.minus.rhs.as.node_expr, g);
                if (b->as.minus.lhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.minus.lhs.as.bin_expr } }, g);
                if (b->as.minus.rhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.minus.rhs.as.bin_expr } }, g);
                break;
            case BIN_EXPR_MULTI:
                if (b->as.multi.lhs.type == NODE_EXPR) collect_vars_in_expr(b->as.multi.lhs.as.node_expr, g);
                if (b->as.multi.rhs.type == NODE_EXPR) collect_vars_in_expr(b->as.multi.rhs.as.node_expr, g);
                if (b->as.multi.lhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.multi.lhs.as.bin_expr } }, g);
                if (b->as.multi.rhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.multi.rhs.as.bin_expr } }, g);
                break;
            case BIN_EXPR_DIVIDE:
                if (b->as.divide.lhs.type == NODE_EXPR) collect_vars_in_expr(b->as.divide.lhs.as.node_expr, g);
                if (b->as.divide.rhs.type == NODE_EXPR) collect_vars_in_expr(b->as.divide.rhs.as.node_expr, g);
                if (b->as.divide.lhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.divide.lhs.as.bin_expr } }, g);
                if (b->as.divide.rhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.divide.rhs.as.bin_expr } }, g);
                break;
            default:
                if (b->as.binop.lhs.type == NODE_EXPR) collect_vars_in_expr(b->as.binop.lhs.as.node_expr, g);
                if (b->as.binop.rhs.type == NODE_EXPR) collect_vars_in_expr(b->as.binop.rhs.as.node_expr, g);
                if (b->as.binop.lhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.binop.lhs.as.bin_expr } }, g);
                if (b->as.binop.rhs.type == BIN_EXPR) collect_vars_in_expr(&(NodeExpr){ .kind = NODE_EXPR_BIN, .as = { .bin = b->as.binop.rhs.as.bin_expr } }, g);
                break;
        }
    }
}


void assign_slots_in_stmt(const NodeStmt* stmt, gen_data* g) {
    if (!stmt || !g) return;
    //TODO: refactor it so i dont create this shit for every type
    if (stmt->kind == NODE_STMT_INT) {
        const char* name = stmt->as.int_.ident.value;
        ensure_var_slot(g, name);
        collect_vars_in_expr(&stmt->as.int_.expr, g);

    } else if (stmt->kind == NODE_STMT_CHAR) {
        const char* name = stmt->as.char_.ident.value;
        ensure_var_slot(g, name);
        collect_vars_in_expr(&stmt->as.char_.expr, g);

    } else if (stmt->kind == NODE_STMT_EXIT) {
        collect_vars_in_expr(&stmt->as.exit_.expr, g);

    } else if (stmt->kind == NODE_STMT_IF) {
        for (size_t i = 0; i < kv_size(stmt->as.if_.body); ++i) {
            assign_slots_in_stmt(&kv_A(stmt->as.if_.body, i), g);
        }

    } else if (stmt->kind == NODE_STMT_FOR) {
        if (stmt->as.for_.cond1)
            assign_slots_in_stmt(stmt->as.for_.cond1, g);
        if (stmt->as.for_.cond3)
            assign_slots_in_stmt(stmt->as.for_.cond3, g);
        for (size_t i = 0; i < kv_size(stmt->as.for_.body); ++i) {
            assign_slots_in_stmt(&kv_A(stmt->as.for_.body, i), g);
        }
    }
}


int lookup_var_slot(gen_data* g, const char* name) {
    if (!g || !g->m_vars || !name) {
        fprintf(stderr, "Invalid arguments to lookup_var_slot\n");
        exit(1);
    }

    StackVec* vars = g->m_vars;
    for (size_t i = 0; i < kv_size(*vars); i++) {
        if (strcmp(kv_A(*vars, i).name, name) == 0) {
            return (int)i;  // index in the array is the slot
        }
    }

    fprintf(stderr, "Undefined variable at codegen: %s\n", name);
    exit(1);
}

// Evaluate a BindExprRec into rax
void gen_bindexpr_to_rax(gen_data* g, const BindExprRec rec) {
    if (rec.type == NODE_EXPR) {
        NodeExpr* n = rec.as.node_expr;
        if (!n) { emit(g, "   ; gen: NULL node\n"); return; }
        if (n->kind == NODE_EXPR_INT_LIT) {
            emit(g, "   mov rax, %s\n", n->as.int_lit.int_lit.value);
            return;
        } else if (n->kind == NODE_EXPR_IDENT) {
            int slot = lookup_var_slot(g, n->as.ident.ident.value);
            int off = slot_to_offset(g,slot);
            emit(g, "   mov rax, qword [rbp - %d]\n", off);
            return;
        } else if (n->kind == NODE_EXPR_BIN && n->as.bin) {
            gen_binexpr_to_rax(g, n->as.bin);
            return;
        } else {
            emit(g, "   ; gen: unsupported node kind %d\n", n->kind);
            return;
        }
    } else if (rec.type == BIN_EXPR) {
        gen_binexpr_to_rax(g, rec.as.bin_expr);
        return;
    } else {
        emit(g, "   ; gen: unknown BindExprRec.type %d\n", rec.type);
    }
}

// Evaluate a BinExpr (pointer) into rax
void gen_binexpr_to_rax(gen_data* g, const BinExpr* b) {
    if (!b) return;

    BindExprRec lhs, rhs;
    switch (b->kind) {
        case BIN_EXPR_ADD:    lhs = b->as.add.lhs;    rhs = b->as.add.rhs;    break;
        case BIN_EXPR_MINUS:  lhs = b->as.minus.lhs;  rhs = b->as.minus.rhs;  break;
        case BIN_EXPR_MULTI:  lhs = b->as.multi.lhs;  rhs = b->as.multi.rhs;  break;
        case BIN_EXPR_DIVIDE: lhs = b->as.divide.lhs; rhs = b->as.divide.rhs; break;
        default:              lhs = b->as.binop.lhs; rhs = b->as.binop.rhs;   break;
    }

    // Arithmetic operations
    if (b->kind == BIN_EXPR_ADD || b->kind == BIN_EXPR_MINUS ||
        b->kind == BIN_EXPR_MULTI || b->kind == BIN_EXPR_DIVIDE) {
        gen_bindexpr_to_rax(g, lhs);     // lhs -> rax
        emit(g, "   push rax\n");
        gen_bindexpr_to_rax(g, rhs);     // rhs -> rax
        emit(g, "   mov rbx, rax\n");    // rbx = rhs
        emit(g, "   pop rax\n");         // rax = lhs

        switch (b->kind) {
            case BIN_EXPR_ADD:   emit(g, "   add rax, rbx\n"); break;
            case BIN_EXPR_MINUS:  emit(g, "   sub rax, rbx\n"); break;
            case BIN_EXPR_MULTI:  emit(g, "   imul rax, rbx\n"); break;
            case BIN_EXPR_DIVIDE: emit(g, "   cqo\n   idiv rbx\n"); break;
            default: break;
        }
        return;
    }

    // Comparisons -> 0/1 in rax
    if (b->kind == BIN_EXPR_EQ || b->kind == BIN_EXPR_NEQ ||
        b->kind == BIN_EXPR_LT || b->kind == BIN_EXPR_LTE ||
        b->kind == BIN_EXPR_MR || b->kind == BIN_EXPR_MRE) {

        gen_bindexpr_to_rax(g, lhs);
        emit(g, "   push rax\n");
        gen_bindexpr_to_rax(g, rhs);
        emit(g, "   mov rbx, rax\n");
        emit(g, "   pop rax\n");
        emit(g, "   cmp rax, rbx\n");

        switch (b->kind) {
            case BIN_EXPR_EQ:  emit(g, "   sete al\n"); break;
            case BIN_EXPR_NEQ: emit(g, "   setne al\n"); break;
            case BIN_EXPR_LT:  emit(g, "   setl al\n"); break;
            case BIN_EXPR_LTE: emit(g, "   setle al\n"); break;
            case BIN_EXPR_MR:  emit(g, "   setg al\n"); break;
            case BIN_EXPR_MRE: emit(g, "   setge al\n"); break;
            default: break;
        }
        emit(g, "   movzx rax, al\n");
        return;
    }

    // Logical AND / OR with short-circuit
    if (b->kind == BIN_EXPR_AND || b->kind == BIN_EXPR_OR) {
        int id = next_label();
        gen_bindexpr_to_rax(g, lhs);
        emit(g, "   test rax, rax\n");

        if (b->kind == BIN_EXPR_AND) {
            emit(g, "   je .L_and_false_%d\n", id);
            gen_bindexpr_to_rax(g, rhs);
            emit(g, "   test rax, rax\n");
            emit(g, "   setne al\n");
            emit(g, "   movzx rax, al\n");
            emit(g, "   jmp .L_and_end_%d\n", id);
            emit(g, ".L_and_false_%d:\n", id);
            emit(g, "   mov rax, 0\n");
            emit(g, ".L_and_end_%d:\n", id);
        } else { // OR
            emit(g, "   jne .L_or_true_%d\n", id);
            gen_bindexpr_to_rax(g, rhs);
            emit(g, "   test rax, rax\n");
            emit(g, "   setne al\n");
            emit(g, "   movzx rax, al\n");
            emit(g, "   jmp .L_or_end_%d\n", id);
            emit(g, ".L_or_true_%d:\n", id);
            emit(g, "   mov rax, 1\n");
            emit(g, ".L_or_end_%d:\n", id);
        }
        return;
    }

    emit(g, "   ; gen_binexpr_to_rax: unhandled kind %d\n", b->kind);
}


// Evaluate NodeExpr (result in rax)
void gen_expr_to_rax(gen_data* g, const NodeExpr* expr, NodeStmtKind kind) {
    if (expr->kind == NODE_EXPR_EMPTY) { emit(g, "   ; gen_expr: NULL\n"); return; }
    if (expr->kind == NODE_EXPR_INT_LIT) {
        if (kind == NODE_STMT_INT) {
            emit(g, "   mov eax, %s\n", expr->as.int_lit.int_lit.value);
        } else if (kind == NODE_STMT_CHAR) {
            emit(g, "   mov al, %s\n", expr->as.char_.char_.value);
        }
        return;
    } else if (expr->kind == NODE_EXPR_IDENT) {
        int slot = lookup_var_slot(g, expr->as.ident.ident.value);
        int off = slot_to_offset(g,slot);
        if (kind == NODE_STMT_INT) {
            emit(g, "   mov eax, qword [rbp - %d]\n", off);
        } else if (kind == NODE_STMT_CHAR) {
            emit(g, "   mov al, byte [rbp - %d]\n", off);
        }
        return;
    } else if (expr->kind == NODE_EXPR_BIN) {
        gen_binexpr_to_rax(g, expr->as.bin);
        return;
    } else if (expr->kind == NODE_EXPR_CHAR) {
        emit(g, "   mov al, %d\n", (int)*expr->as.char_.char_.value);
        printf("value: %s\n", expr->as.char_.char_.value);
        printf("num: %d\n", (int)*expr->as.char_.char_.value);
        return;
    } else {
        printf("gen_expr: unknown kind %d\n", expr->kind);
        exit(1);
    }
}

void delete_local_var(gen_data *g) {
    if (!g || !g->m_block || kv_size(*g->m_block) == 0 || !g->m_vars) {
        fprintf(stderr, "Error: gen_data or m_block or m_vars is null/empty\n");
        exit(EXIT_FAILURE);
    }

    Str2DVec *blocks = g->m_block;
    size_t last_block_idx = kv_size(*blocks) - 1;
    StrVec *last_block = &kv_A(*blocks, last_block_idx);

    // Iterate strings in last_block
    for (size_t i = 0; i < kv_size(*last_block); i++) {
        char *key = kv_A(*last_block, i);

        // Search for key in StackVec
        size_t j;
        int found = 0;
        for (j = 0; j < kv_size(*g->m_vars); j++) {
            if (strcmp(kv_A(*g->m_vars, j).name, key) == 0) {
                found = 1;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "Error: key '%s' not found in m_vars\n", key);
            exit(EXIT_FAILURE);
        }

        // free name string
        free(kv_A(*g->m_vars, j).name);

        // remove from StackVec by shifting elements
        for (size_t k = j; k + 1 < kv_size(*g->m_vars); k++) {
            kv_A(*g->m_vars, k) = kv_A(*g->m_vars, k + 1);
        }
        kv_pop(*g->m_vars); // reduce size by 1
    }

    // remove the last block
    kv_pop(*blocks);
}


// removes last block from g->m_block
void remove_last_block(gen_data *g) {
    if (!g || !g->m_block || kv_size(*g->m_block) == 0)
        return; // nothing to remove

    Str2DVec *blocks = g->m_block;
    size_t last_block_idx = kv_size(*blocks) - 1;

    // Get pointer to the last block (the real one inside the outer vector)
    StrVec *last_block = &kv_A(*blocks, last_block_idx);

    // Free all strings in the last block
    for (size_t i = 0; i < kv_size(*last_block); i++) {
        free(kv_A(*last_block, i));
    }

    // Destroy the inner vector (pass the actual vector)
    kv_destroy(*last_block);

    // Remove the last element from outer vector
    kv_pop(*blocks);

}