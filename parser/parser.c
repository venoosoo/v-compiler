// parser.c
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "./binstmt/binstmt.h"
#include <stdbool.h>
// forward declaration implemented in binstmt.c
extern BindExprRec parse_bin_stmt_rec(Parser_data* p, BinExpr* top, int ptr, const int ptr_max);

// ---- Parser data initialization ----
Parser_data* init_parser(TokenArray src) {
    Parser_data* p = (Parser_data*)malloc(sizeof(Parser_data));
    if (!p) return NULL;
    p->m_index = 0;
    p->m_tokens = src;
    return p;
}


// ---- Peek next token ----
// offset is relative to current p->m_index
OptionalToken parser_peek(Parser_data* p, int offset) {
    OptionalToken result;
    int idx = p->m_index + offset;
    if (idx < 0 || idx >= kv_size(p->m_tokens)) {
        result.has_value = 0;
        Token empty = { .type = token_empty, .value = NULL };
        result.value = empty;
    } else {
        result.has_value = 1;
        result.value = kv_A(p->m_tokens, idx);
    }
    return result;
}

bool is_type(TokenType type) {
    return type == token_type_char_t ||
           type == token_type_int ||
           type == token_type_short ||
           type == token_type_long;
}



// ---- Consume token ----
Token parser_consume(Parser_data* p) {
    return kv_A(p->m_tokens, p->m_index++);
}

// ---- Print binary expression (debug helper) ----
void print_bin_expr(BinExpr* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("-");

        switch (node->kind) {
        case BIN_EXPR_ADD:
            printf("+\n");
            if (node->as.add.lhs.type == BIN_EXPR) print_bin_expr(node->as.add.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.add.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.add.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.add.rhs.type == BIN_EXPR) print_bin_expr(node->as.add.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.add.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.add.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_MINUS:
            printf("-\n");
            if (node->as.minus.lhs.type == BIN_EXPR) print_bin_expr(node->as.minus.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.minus.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.minus.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.minus.rhs.type == BIN_EXPR) print_bin_expr(node->as.minus.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.minus.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.minus.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_MULTI:
            printf("*\n");
            if (node->as.multi.lhs.type == BIN_EXPR) print_bin_expr(node->as.multi.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.multi.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.multi.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.multi.rhs.type == BIN_EXPR) print_bin_expr(node->as.multi.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.multi.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.multi.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_DIVIDE:
            printf("/\n");
            if (node->as.divide.lhs.type == BIN_EXPR) print_bin_expr(node->as.divide.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.divide.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.divide.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.divide.rhs.type == BIN_EXPR) print_bin_expr(node->as.divide.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.divide.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.divide.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        /* NEW: equality / relational / logical ops (use binop union) */
        case BIN_EXPR_EQ:
            printf("==\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_NEQ:
            printf("!=\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_LT:
            printf("<\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_LTE:
            printf("<=\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_MR:
            printf(">\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_MRE:
            printf(">=\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_AND:
            printf("&&\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        case BIN_EXPR_OR:
            printf("||\n");
            if (node->as.binop.lhs.type == BIN_EXPR) print_bin_expr(node->as.binop.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.lhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.lhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_LHS_TYPE\n");
            }
            if (node->as.binop.rhs.type == BIN_EXPR) print_bin_expr(node->as.binop.rhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("-");
                if (node->as.binop.rhs.type == NODE_EXPR) {
                    NodeExpr *n = node->as.binop.rhs.as.node_expr;
                    if (n) {
                        if (n->kind == NODE_EXPR_INT_LIT) printf("INT(%s)\n", n->as.int_lit.int_lit.value);
                        else if (n->kind == NODE_EXPR_IDENT) printf("IDENT(%s)\n", n->as.ident.ident.value);
                        else printf("NODE(kind=%d)\n", n->kind);
                    } else printf("NODE(NULL)\n");
                } else printf("UNKNOWN_RHS_TYPE\n");
            }
            break;

        default:
            printf("UNKNOWN_BIN_KIND(%d)\n", node->kind);
            break;
    }

}

// ---- Parse binary statement ----
OptionalBinExpr parse_bin_stmt(Parser_data* p) {
    OptionalBinExpr res = {0};

    OptionalNodeExpr expr_opt = parse_expr_to_terminator(p);
    if (!expr_opt.has_value) {
        res.has_value = 0;
        return res;
    }

    // only accept binary expression results
    if (expr_opt.value.kind != NODE_EXPR_BIN || expr_opt.value.as.bin == NULL) {
        res.has_value = 0;
        return res;
    }

    // copy the BinExpr value out (caller often allocates a heap NodeExpr and copies this in)
    res.has_value = 1;
    res.value = *(expr_opt.value.as.bin);
    return res;
}


bool is_comparison_op(TokenType t) {
    return t == token_type_cmp ||
           t == token_type_less ||
           t == token_type_less_eq ||
           t == token_type_more ||
           t == token_type_more_eq ||
           t == token_type_not_eq ||
           t == token_type_and ||
           t == token_type_or;
}

// ---- Parse expression ----
OptionalNodeExpr parse_expr(Parser_data* p) {
    OptionalNodeExpr result = {0};
    OptionalToken t = parser_peek(p, 0);
    if (!t.has_value) return result;
    if ((t.value.type == token_type_int_lit || t.value.type == token_type_char_v) || t.value.type == token_type_ident) {
        OptionalToken t1 = parser_peek(p, 1);
        OptionalToken t2 = parser_peek(p, 2);

        // so we check if in expr second token is some operation and it has left and right
        if (t1.has_value && t2.has_value &&
            (t1.value.type == token_type_plus || t1.value.type == token_type_minus ||
             t1.value.type == token_type_multi || t1.value.type == token_type_divide) &&
            ((t2.value.type == token_type_int_lit || t2.value.type == token_type_int_lit) || t2.value.type == token_type_ident)) {

            OptionalBinExpr bin = parse_bin_stmt(p);
            if (!bin.has_value) {
                return result;
            }

            NodeExpr* expr_heap = (NodeExpr*)malloc(sizeof(NodeExpr));
            if (!expr_heap) { fprintf(stderr,"Out of memory\n"); exit(1); }
            expr_heap->kind = NODE_EXPR_BIN;
            expr_heap->as.bin = (BinExpr*)malloc(sizeof(BinExpr));
            if (!expr_heap->as.bin) { fprintf(stderr,"Out of memory\n"); exit(1); }
            *expr_heap->as.bin = bin.value;

            result.has_value = 1;
            result.value = *expr_heap;


            print_bin_expr(expr_heap->as.bin, 0);
            return result;
        }
        // we have like x == 5
        if (t1.has_value && is_comparison_op(t1.value.type)) {
            OptionalBinExpr bin = parse_bin_stmt(p);
            if (!bin.has_value) {
                return result;
            }

            NodeExpr* expr_heap = (NodeExpr*)malloc(sizeof(NodeExpr));
            if (!expr_heap) { fprintf(stderr,"Out of memory\n"); exit(1); }
            expr_heap->kind = NODE_EXPR_BIN;
            expr_heap->as.bin = (BinExpr*)malloc(sizeof(BinExpr));
            if (!expr_heap->as.bin) { fprintf(stderr,"Out of memory\n"); exit(1); }
            *expr_heap->as.bin = bin.value;

            result.has_value = 1;
            result.value = *expr_heap;

            print_bin_expr(expr_heap->as.bin, 0);
            return result;
        }
        else {
            printf("t_val: %d\n", t.value.type);
            NodeExpr expr;

            if (t.value.type == token_type_int_lit) {
                expr.kind = NODE_EXPR_INT_LIT;
                expr.as.int_lit.int_lit = parser_consume(p);
            } else if (t.value.type == token_type_char_v) {
                printf("HEY\n");
                expr.kind = NODE_EXPR_CHAR;
                expr.as.char_.char_ = parser_consume(p);
            } else {
                expr.kind = NODE_EXPR_IDENT;
                expr.as.ident.ident = parser_consume(p);
            }
            result.has_value = 1;
            result.value = expr;
            return result;
        }
    }
    return result;
}

// ---- Parse statement ----
OptionalNodeStmt parse_stmt(Parser_data* p) {
    OptionalNodeStmt result = {0};

    OptionalToken t0 = parser_peek(p, 0);
    if (t0.has_value && t0.value.type == token_type_exit_kw) {
        parser_consume(p); // consume exit
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_paren)) {
            fprintf(stderr,"Expected '('\n"); exit(1);
        }
        parser_consume(p); // (
        NodeStmtExit stmt_exit;
        // parsing expr inside parentheses
        OptionalNodeExpr expr = parse_expr(p);
        if (!expr.has_value) { fprintf(stderr,"Invalid expression after exit\n"); exit(1); }
        stmt_exit.expr = expr.value;
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_close_paren)) {
            fprintf(stderr,"Expected ')'\n"); exit(1);
        }
        parser_consume(p);
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_semi)) {
            fprintf(stderr,"Expected ';'\n"); exit(1);
        }
        parser_consume(p);
        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_EXIT;
        node_stmt.as.exit_ = stmt_exit;
        result.has_value = 1;
        result.value = node_stmt;
        return result;
    }

    OptionalToken t1 = parser_peek(p, 1);
    OptionalToken t2 = parser_peek(p, 2);


    if (t0.has_value && is_type(t0.value.type) &&
        t1.has_value && t1.value.type == token_type_ident &&
        t2.has_value && t2.value.type == token_type_eq_kw) {
        Token type = parser_consume(p);
        Token ident = parser_consume(p);
        parser_consume(p); // consume =
        OptionalNodeExpr expr = parse_expr(p);
        if (parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_semi) {
            parser_consume(p);
        } else {
            printf("debug: m_index: %d\n", p->m_index);
            printf("Expected ;\n");
            exit(1);
        }
        if (!expr.has_value) { fprintf(stderr,"Invalid expression after let\n"); exit(1); }
        NodeStmt node_stmt;
        if (type.type == token_type_short) {
            node_stmt.kind = NODE_STMT_SHORT;
            node_stmt.as.short_.ident = ident;
            node_stmt.as.short_.expr = expr.value;
        } else if (type.type == token_type_long) {
            node_stmt.kind = NODE_STMT_LONG;
            node_stmt.as.long_.ident = ident;
            node_stmt.as.long_.expr = expr.value;
        }
        else if (type.type == token_type_int) {
            node_stmt.kind = NODE_STMT_INT;
            node_stmt.as.int_.ident = ident;
            node_stmt.as.int_.expr = expr.value;
        } else if (type.type == token_type_char_t) {
            node_stmt.kind = NODE_STMT_CHAR;
            node_stmt.as.char_.ident = ident;
            node_stmt.as.char_.expr = expr.value;
        }
        result.has_value = 1;
        result.value = node_stmt;
        return result;
    }

    if (t0.has_value && t0.value.type == token_type_ident &&
        t1.has_value && t1.value.type == token_type_eq_kw) {
            NodeStmtVchange stmt_vchange;
            stmt_vchange.ident = parser_consume(p);
            parser_consume(p); // consume =
            OptionalNodeExpr expr = parse_expr(p);
            if (!(parser_peek(p,0).has_value && (parser_peek(p,0).value.type == token_type_semi || parser_peek(p,0).value.type == token_type_close_paren))) {
                printf("m_index: %d\n", p->m_index);
                fprintf(stderr,"Expected '; or )'\n"); exit(1);
            }
            parser_consume(p); // '; or )'
            if (!expr.has_value) { fprintf(stderr,"Invalid expression after let\n"); exit(1); }
            stmt_vchange.expr = expr.value;
            NodeStmt node_stmt;
            node_stmt.kind = NODE_STMT_VCHANGE;
            node_stmt.as.vchange = stmt_vchange;
            result.has_value = 1;
            result.value = node_stmt;
            return result;
        }

    if (t0.has_value && t0.value.type == token_type_if) {
        // consume 'if'
        parser_consume(p);
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_paren)) {
            fprintf(stderr,"Expected '('\n"); exit(1);
        }
        parser_consume(p); // '('
        OptionalNodeExpr cond = parse_expr(p);
        if (!cond.has_value) { fprintf(stderr,"Invalid expression in if condition\n"); exit(1); }

        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_close_paren)) {
            fprintf(stderr,"Expected ')'\n"); exit(1);
        }
        parser_consume(p); // ')'

        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_braces)) {
            fprintf(stderr,"Expected '{'\n"); exit(1);
        }
        parser_consume(p); // '{'

        // parse inner statements until closing brace
        NodeStmtArray body = {};
        kv_init(body);
        while (parser_peek(p,0).has_value && parser_peek(p,0).value.type != token_type_close_braces) {
            OptionalNodeStmt inner = parse_stmt(p);
            if (!inner.has_value) {
                // print context to help debugging: show current token & index
                OptionalToken cur = parser_peek(p, 0);
                if (cur.has_value) {
                    fprintf(stderr, "Failed to parse statement inside if at index %d: token type=%d\n", p->m_index, cur.value.type);
                } else {
                    fprintf(stderr, "Failed to parse statement inside if at index %d: no token\n", p->m_index);
                }
                exit(1);
            }
            kv_push(NodeStmt, body, inner.value);
        }


        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_close_braces)) {
            fprintf(stderr,"Expected '}'\n"); exit(1);
        }
        parser_consume(p); // '}'

        NodeStmtIf n_if;
        n_if.cond = cond.value;
        n_if.body = body;

        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_IF;
        node_stmt.as.if_ = n_if;
        result.has_value = 1;
        result.value = node_stmt;
        return result;
    }
    if (t0.has_value && t0.value.type == token_type_else) {
        parser_consume(p);
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_braces)) {
            fprintf(stderr,"Expected '{'\n"); exit(1);
        }
        parser_consume(p); // '{'

        NodeStmtArray body;
        kv_init(body);
        while (parser_peek(p,0).has_value && parser_peek(p,0).value.type != token_type_close_braces) {
            OptionalNodeStmt inner = parse_stmt(p);
            if (!inner.has_value) { fprintf(stderr,"Failed to parse statement inside if\n"); exit(1); }
            kv_push(NodeStmt, body, inner.value);
        }
        printf("m_index: %d\n", p->m_index);

        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_close_braces)) {
            fprintf(stderr,"Expected '}'\n"); exit(1);
        }
        parser_consume(p); // '}'
        NodeStmtElse n_else;
        n_else.body = body;
        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_ELSE;
        node_stmt.as.else_ = n_else;
        result.has_value = 1;
        result.value = node_stmt;
        printf("result: %d\n", result.has_value);
        return result;
    }
    if (t0.has_value && t0.value.type == token_type_while) {
        parser_consume(p); //consume while
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_paren)) {
            fprintf(stderr,"Expected '('\n"); exit(1);
        }
        parser_consume(p); // '('
        OptionalNodeExpr cond = parse_expr(p);
        if (!cond.has_value) { fprintf(stderr,"Invalid expression in while condition\n"); exit(1); }

        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_close_paren)) {
            printf("m_index: %d\n", p->m_index);
            fprintf(stderr,"Expected ')'\n"); exit(1);
        }
        parser_consume(p); // ')'

        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_braces)) {
            fprintf(stderr,"Expected '{'\n"); exit(1);
        }
        parser_consume(p); // '{'

        // parse inner statements until closing brace
        NodeStmtArray body = {};
        kv_init(body);
        while (parser_peek(p,0).has_value && parser_peek(p,0).value.type != token_type_close_braces) {
            OptionalNodeStmt inner = parse_stmt(p);
            if (!inner.has_value) {
                // print context to help debugging: show current token & index
                OptionalToken cur = parser_peek(p, 0);
                if (cur.has_value) {
                    fprintf(stderr, "Failed to parse statement inside while at index %d: token type=%d\n", p->m_index, cur.value.type);
                } else {
                    fprintf(stderr, "Failed to parse statement inside while at index %d: no token\n", p->m_index);
                }
                exit(1);
            }
            kv_push(NodeStmt, body, inner.value);
        }
        printf("m_index: %d\n", p->m_index);

        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_close_braces)) {
            fprintf(stderr,"Expected '}'\n"); exit(1);
        }
        parser_consume(p); // '}'
        NodeStmtWhile n_while;
        n_while.body = body;
        n_while.cond = cond.value;
        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_WHILE;
        node_stmt.as.while_ = n_while;
        result.has_value = 1;
        result.value = node_stmt;
        printf("result: %d\n", result.has_value);
        return result;
    }
    if (t0.has_value && t0.value.type == token_type_for) {
        parser_consume(p); // for 
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_paren)) {
            fprintf(stderr,"Expected '('\n"); exit(1);
        }
        parser_consume(p); // '('
        OptionalNodeStmt cond1 = parse_stmt(p);
        OptionalNodeExpr cond2 = parse_expr(p);
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_semi)) {
            fprintf(stderr,"Expected ';'\n"); exit(1);
        }
        parser_consume(p);
        OptionalNodeStmt cond3 = parse_stmt(p);

        // no checking for ) cause it checking in conditions  

        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_braces)) {
            fprintf(stderr,"Expected '{'\n"); exit(1);
        }
        parser_consume(p); // '{'

        NodeStmtArray body = {};
        kv_init(body);
        while (parser_peek(p,0).has_value && parser_peek(p,0).value.type != token_type_close_braces) {
            printf("m_index: %d\n", p->m_index);
            OptionalNodeStmt inner = parse_stmt(p);
            if (!inner.has_value) {
                // print context to help debugging: show current token & index
                OptionalToken cur = parser_peek(p, 0);
                if (cur.has_value) {
                    fprintf(stderr, "Failed to parse statement inside for at index %d: token type=%d\n", p->m_index, cur.value.type);
                } else {
                    fprintf(stderr, "Failed to parse statement inside for at index %d: no token\n", p->m_index);
                }
                exit(1);
            }
            printf("m_index22: %d\n", p->m_index);
            kv_push(NodeStmt, body, inner.value);
        }
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_close_braces)) {
            fprintf(stderr,"Expected '}'\n"); exit(1);
        }
        parser_consume(p); // '}'
        NodeStmtFor n_for;
        n_for.body = body;
        n_for.cond1 = malloc(sizeof(NodeStmt));
        *n_for.cond1 = cond1.value;
        n_for.cond2 = cond2.value; // stop logic operation
        n_for.cond3 = malloc(sizeof(NodeStmt)); // expr that goes every iteration
        *n_for.cond3 = cond3.value;
        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_FOR;
        node_stmt.as.for_ = n_for;
        result.has_value = 1;
        result.value = node_stmt;
        return result;
    }

    return result;
}


// ---- Parse program ----
OptionalNodeProg parse_prog(Parser_data* p) {
    OptionalNodeProg result = {0};
    kv_init(result.value.stmt);

    while (parser_peek(p,0).has_value) {
        OptionalNodeStmt stmt = parse_stmt(p);
        if (!stmt.has_value) { fprintf(stderr,"Failed to parse statement\n"); printf("m_index: %d\n", p->m_index); exit(1); }
        kv_push(NodeStmt, result.value.stmt, stmt.value);
    }

    result.has_value = 1;
    return result;
}
