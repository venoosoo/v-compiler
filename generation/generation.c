#include "./generation.h"
#include "../libs/sds.h"
#include "../libs/khashl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


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


void gen_bin_stmt(gen_data* g, BindExprRec stmt) {
    if (stmt.type == NODE_EXPR) {
        NodeExpr* n = stmt.as.node_expr;
        if (!n) return;

        if (n->kind == NODE_EXPR_INT_LIT) {
            g->m_output = sdscatprintf(g->m_output, "   mov rdi, %s\n", n->as.int_lit.int_lit.value);
            return;
        } else if (n->kind == NODE_EXPR_IDENT) {
            khint_t k = str_var_get(g->m_vars, n->as.ident.ident.value);
            if (k == kh_end(g->m_vars)) {
                fprintf(stderr, "Undefined variable: %s\n", n->as.ident.ident.value);
                return;
            }
            var* vptr = kh_val(g->m_vars, k);
            if (!vptr) {
                fprintf(stderr, "Internal error: variable has NULL value: %s\n", n->as.ident.ident.value);
                return;
            }
            int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
            g->m_output = sdscatprintf(g->m_output, "   mov rdi, qword [rsp + %d]\n", offset);
            return;
        } else {
            return;
        }
    }

    if (stmt.type != BIN_EXPR) return;
    BinExpr* b = stmt.as.bin_expr;
    if (!b) return;

    if (b->kind == BIN_EXPR_ADD) {
        BindExprRec lhs = b->as.add.lhs;
        BindExprRec rhs = b->as.add.rhs;

        if (lhs.type == BIN_EXPR && rhs.type != BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            if (rhs.type == NODE_EXPR) {
                NodeExpr* rn = rhs.as.node_expr;
                if (rn->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, %s\n", rn->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   add rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                } else if (rn->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   add rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                }
            }
            return;
        }

        if (rhs.type == BIN_EXPR && lhs.type != BIN_EXPR) {
            gen_bin_stmt(g, rhs);
            if (lhs.type == NODE_EXPR) {
                NodeExpr* ln = lhs.as.node_expr;
                if (ln->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, %s\n", ln->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   add rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                } else if (ln->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   add rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                }
            }
            return;
        }

        if (lhs.type == BIN_EXPR && rhs.type == BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            push(g, "rdi");
            gen_bin_stmt(g, rhs);
            pop(g, "rcx");
            g->m_output = sdscatprintf(g->m_output, "   add rcx, rdi\n");
            g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
            return;
        }

        if (lhs.type == NODE_EXPR && rhs.type == NODE_EXPR) {
            bool lgood = false, rgood = false;
            NodeExpr* ln = lhs.as.node_expr;
            if (ln->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rcx, qword [rsp + %d]\n", offset);
                lgood = true;
            } else if (ln->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rcx, %s\n", ln->as.int_lit.int_lit.value);
                lgood = true;
            }

            NodeExpr* rn = rhs.as.node_expr;
            if (rn->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rdx, qword [rsp + %d]\n", offset);
                rgood = true;
            } else if (rn->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rdx, %s\n", rn->as.int_lit.int_lit.value);
                rgood = true;
            }

            if (lgood && rgood) {
                g->m_output = sdscatprintf(g->m_output, "   add rcx, rdx\n");
                g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
            }
            return;
        }

        printf("Some critical binary error (add)\n");
        exit(1);
    }

    if (b->kind == BIN_EXPR_MINUS) {
        BindExprRec lhs = b->as.minus.lhs;
        BindExprRec rhs = b->as.minus.rhs;

        if (lhs.type == BIN_EXPR && rhs.type != BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            if (rhs.type == NODE_EXPR) {
                NodeExpr* rn = rhs.as.node_expr;
                if (rn->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, %s\n", rn->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   sub rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                } else if (rn->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   sub rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                }
            }
            return;
        }

        if (rhs.type == BIN_EXPR && lhs.type != BIN_EXPR) {
            gen_bin_stmt(g, rhs);
            if (lhs.type == NODE_EXPR) {
                NodeExpr* ln = lhs.as.node_expr;
                if (ln->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, %s\n", ln->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   sub rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                } else if (ln->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   sub rcx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
                }
            }
            return;
        }

        if (lhs.type == BIN_EXPR && rhs.type == BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            push(g, "rdi");
            gen_bin_stmt(g, rhs);
            pop(g, "rcx");
            g->m_output = sdscatprintf(g->m_output, "   sub rcx, rdi\n");
            g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
            return;
        }

        if (lhs.type == NODE_EXPR && rhs.type == NODE_EXPR) {
            bool lgood = false, rgood = false;
            NodeExpr* ln = lhs.as.node_expr;
            if (ln->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rcx, qword [rsp + %d]\n", offset);
                lgood = true;
            } else if (ln->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rcx, %s\n", ln->as.int_lit.int_lit.value);
                lgood = true;
            }

            NodeExpr* rn = rhs.as.node_expr;
            if (rn->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rdx, qword [rsp + %d]\n", offset);
                rgood = true;
            } else if (rn->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rdx, %s\n", rn->as.int_lit.int_lit.value);
                rgood = true;
            }

            if (lgood && rgood) {
                g->m_output = sdscatprintf(g->m_output, "   sub rcx, rdx\n");
                g->m_output = sdscatprintf(g->m_output, "   mov rdi, rcx\n");
            }
            return;
        }

        printf("Some critical binary error (minus)\n");
        exit(1);
    }

    if (b->kind == BIN_EXPR_MULTI) {
        BindExprRec lhs = b->as.multi.lhs;
        BindExprRec rhs = b->as.multi.rhs;

        if (lhs.type == BIN_EXPR && rhs.type != BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            if (rhs.type == NODE_EXPR) {
                NodeExpr* rn = rhs.as.node_expr;
                if (rn->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, %s\n", rn->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   mul rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                } else if (rn->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   mul rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                }
            }
            return;
        }

        if (rhs.type == BIN_EXPR && lhs.type != BIN_EXPR) {
            gen_bin_stmt(g, rhs);
            if (lhs.type == NODE_EXPR) {
                NodeExpr* ln = lhs.as.node_expr;
                if (ln->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, %s\n", ln->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mul rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                } else if (ln->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   mov rdx, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mul rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                }
            }
            return;
        }

        if (lhs.type == BIN_EXPR && rhs.type == BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            push(g, "rdi");
            gen_bin_stmt(g, rhs);
            pop(g, "rcx");
            g->m_output = sdscatprintf(g->m_output, "   mov rax, rcx\n");
            g->m_output = sdscatprintf(g->m_output, "   mul rdi\n");
            g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
            return;
        }

        if (lhs.type == NODE_EXPR && rhs.type == NODE_EXPR) {
            bool lgood = false, rgood = false;
            NodeExpr* ln = lhs.as.node_expr;
            if (ln->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rax, qword [rsp + %d]\n", offset);
                lgood = true;
            } else if (ln->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rax, %s\n", ln->as.int_lit.int_lit.value);
                lgood = true;
            }

            NodeExpr* rn = rhs.as.node_expr;
            if (rn->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rdx, qword [rsp + %d]\n", offset);
                rgood = true;
            } else if (rn->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rdx, %s\n", rn->as.int_lit.int_lit.value);
                rgood = true;
            }

            if (lgood && rgood) {
                g->m_output = sdscatprintf(g->m_output, "   mul rdx\n");
                g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
            }
            return;
        }

        printf("Some critical binary error (multi)\n");
        exit(1);
    }

    if (b->kind == BIN_EXPR_DIVIDE) {
        BindExprRec lhs = b->as.divide.lhs;
        BindExprRec rhs = b->as.divide.rhs;

        if (lhs.type == BIN_EXPR && rhs.type != BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            if (rhs.type == NODE_EXPR) {
                NodeExpr* rn = rhs.as.node_expr;
                if (rn->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   xor rdx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, %s\n", rn->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   div rcx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                } else if (rn->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   xor rdx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rcx, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   div rcx\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                }
            }
            return;
        }

        if (rhs.type == BIN_EXPR && lhs.type != BIN_EXPR) {
            gen_bin_stmt(g, rhs);
            if (lhs.type == NODE_EXPR) {
                NodeExpr* ln = lhs.as.node_expr;
                if (ln->kind == NODE_EXPR_INT_LIT) {
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, %s\n", ln->as.int_lit.int_lit.value);
                    g->m_output = sdscatprintf(g->m_output, "   xor rdx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   div rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                } else if (ln->kind == NODE_EXPR_IDENT) {
                    khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                    if (k == kh_end(g->m_vars)) {
                        fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    var* vptr = kh_val(g->m_vars, k);
                    if (!vptr) {
                        fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                        return;
                    }
                    int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                    g->m_output = sdscatprintf(g->m_output, "   mov rax, qword [rsp + %d]\n", offset);
                    g->m_output = sdscatprintf(g->m_output, "   xor rdx, rdx\n");
                    g->m_output = sdscatprintf(g->m_output, "   div rdi\n");
                    g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
                }
            }
            return;
        }

        if (lhs.type == BIN_EXPR && rhs.type == BIN_EXPR) {
            gen_bin_stmt(g, lhs);
            push(g, "rdi");
            gen_bin_stmt(g, rhs);
            pop(g, "rcx");
            g->m_output = sdscatprintf(g->m_output, "   mov rax, rcx\n");
            g->m_output = sdscatprintf(g->m_output, "   xor rdx, rdx\n");
            g->m_output = sdscatprintf(g->m_output, "   div rdi\n");
            g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
            return;
        }

        if (lhs.type == NODE_EXPR && rhs.type == NODE_EXPR) {
            bool lgood = false, rgood = false;
            NodeExpr* ln = lhs.as.node_expr;
            if (ln->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, ln->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", ln->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", ln->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rax, qword [rsp + %d]\n", offset);
                g->m_output = sdscatprintf(g->m_output, "   xor rdx, rdx\n");
                lgood = true;
            } else if (ln->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rax, %s\n", ln->as.int_lit.int_lit.value);
                g->m_output = sdscatprintf(g->m_output, "   xor rdx, rdx\n");
                lgood = true;
            }

            NodeExpr* rn = rhs.as.node_expr;
            if (rn->kind == NODE_EXPR_IDENT) {
                khint_t k = str_var_get(g->m_vars, rn->as.ident.ident.value);
                if (k == kh_end(g->m_vars)) {
                    fprintf(stderr, "Undefined variable: %s\n", rn->as.ident.ident.value);
                    return;
                }
                var* vptr = kh_val(g->m_vars, k);
                if (!vptr) {
                    fprintf(stderr, "Internal error: variable has NULL value: %s\n", rn->as.ident.ident.value);
                    return;
                }
                int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);
                g->m_output = sdscatprintf(g->m_output, "   mov rcx, qword [rsp + %d]\n", offset);
                rgood = true;
            } else if (rn->kind == NODE_EXPR_INT_LIT) {
                g->m_output = sdscatprintf(g->m_output, "   mov rcx, %s\n", rn->as.int_lit.int_lit.value);
                rgood = true;
            }

            if (lgood && rgood) {
                g->m_output = sdscatprintf(g->m_output, "   div rcx\n");
                g->m_output = sdscatprintf(g->m_output, "   mov rdi, rax\n");
            }
            return;
        }

        printf("Some critical binary error (divide)\n");
        exit(1);
    }

    printf("Some critical binary error (unknown kind)\n");
    exit(1);
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
        } else {
            printf("ERROR\n");
        }

    } else if (stmt->kind == NODE_STMT_EXIT) {
        get_expr(g, &stmt->as.exit_.expr);
        pop(g, "rdi");
        g->m_output = sdscatprintf(g->m_output, "   mov rax, 60\n");
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

        int offset = (int)((g->m_stack_pos - vptr->stack_pos - 1) * 8);

        sds tmp = sdscatprintf(sdsempty(), "qword [rsp + %d]", offset);
        push(g, tmp);
        sdsfree(tmp);
    } else if (expr->kind == NODE_EXPR_BIN) {
        BindExprRec rec;
        rec.type = BIN_EXPR;
        rec.as.bin_expr = expr->as.bin;
        gen_bin_stmt(g, rec);
        push(g, "rdi");
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
