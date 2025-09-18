// binstmt.c
#include "../parser.h"
#include <stdlib.h>
#include <stdio.h>

// Recursive builder: builds tree with heap allocations for leaves/subtrees
BindExprRec parse_bin_stmt_rec(Parser_data* p, BinExpr* top, int ptr, const int ptr_max) {
    if (!top) {
        fprintf(stderr, "internal parser error: top is NULL\n");
        exit(1);
    }

    // ensure children default to NODE_EXPR with NULL pointers to avoid garbage
    // (only set the appropriate union fields as we go)
    if (top->kind == BIN_EXPR_ADD || top->kind == BIN_EXPR_MINUS) {
        /* LEFT side */
        int i;
        int found = 0;
        for (i = 1; ptr - 2 * i >= 0; i++) {
            OptionalToken ot = parser_peek(p, ptr - 2 * i);
            if (!ot.has_value) break;
            if (ot.value.type == token_type_plus || ot.value.type == token_type_minus) {
                found = 1;
                break;
            }
        }
        if (found) {
            OptionalToken op = parser_peek(p, ptr - 2 * i);
            BinExpr* next = (BinExpr*)malloc(sizeof(BinExpr));
            if (!next) { fprintf(stderr,"Out of memory\n"); exit(1); }
            next->kind = (op.value.type == token_type_plus) ? BIN_EXPR_ADD : BIN_EXPR_MINUS;
            next->as.add.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            next->as.add.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            next->as.minus.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            next->as.minus.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            BindExprRec left_rec = parse_bin_stmt_rec(p, next, ptr - 2 * i, ptr_max);
            if (top->kind == BIN_EXPR_ADD) top->as.add.lhs = left_rec;
            else top->as.minus.lhs = left_rec;
        } else {
            /* no + or - to the left, search for * or / to the left */
            found = 0;
            for (i = 1; ptr - 2 * i >= 0; i++) {
                OptionalToken ot = parser_peek(p, ptr - 2 * i);
                if (!ot.has_value) break;
                if (ot.value.type == token_type_multi || ot.value.type == token_type_divide) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                OptionalToken op = parser_peek(p, ptr - 2 * i);
                BinExpr* next = (BinExpr*)malloc(sizeof(BinExpr));
                if (!next) { fprintf(stderr,"Out of memory\n"); exit(1); }
                next->kind = (op.value.type == token_type_multi) ? BIN_EXPR_MULTI : BIN_EXPR_DIVIDE;
                next->as.multi.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                next->as.multi.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                next->as.divide.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                next->as.divide.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                BindExprRec left_rec = parse_bin_stmt_rec(p, next, ptr - 2 * i, ptr_max);
                if (top->kind == BIN_EXPR_ADD) top->as.add.lhs = left_rec;
                else top->as.minus.lhs = left_rec;
            } else {
                /* leaf at ptr - 1 */
                OptionalToken tok = parser_peek(p, ptr - 1);
                if (!tok.has_value) { fprintf(stderr,"some error in writing equation (lhs missing)\n"); exit(1); }
                NodeExpr* node = (NodeExpr*)malloc(sizeof(NodeExpr));
                if (!node) { fprintf(stderr,"Out of memory\n"); exit(1); }
                if (tok.value.type == token_type_ident) {
                    node->kind = NODE_EXPR_IDENT;
                    node->as.ident.ident = tok.value;
                } else if (tok.value.type == token_type_int_lit) {
                    node->kind = NODE_EXPR_INT_LIT;
                    node->as.int_lit.int_lit = tok.value;
                } else {
                    fprintf(stderr,"unexpected token type for lhs leaf: %d\n", tok.value.type);
                    exit(1);
                }
                BindExprRec left = { .type = NODE_EXPR, .as.node_expr = node };
                if (top->kind == BIN_EXPR_ADD) top->as.add.lhs = left;
                else top->as.minus.lhs = left;
            }
        }

        /* RIGHT side */
        int j = 1;
        if (parser_peek(p, ptr + 2).has_value
            && (parser_peek(p, ptr + 2).value.type != token_type_minus
                && parser_peek(p, ptr + 2).value.type != token_type_plus)) {

            /* Find the most-right (largest index) multi/divide on the right side */
            int last_j = 0;
            for (j = 1; ptr + 2 * j <= ptr_max; j++) {
                OptionalToken ot = parser_peek(p, ptr + 2 * j);
                if (!ot.has_value) break;
                if (ot.value.type == token_type_multi || ot.value.type == token_type_divide) {
                    last_j = j;
                }
            }
            if (last_j > 0) {
                OptionalToken op = parser_peek(p, ptr + 2 * last_j);
                BinExpr* next = (BinExpr*)malloc(sizeof(BinExpr));
                if (!next) { fprintf(stderr,"Out of memory\n"); exit(1); }
                next->kind = (op.value.type == token_type_multi) ? BIN_EXPR_MULTI : BIN_EXPR_DIVIDE;
                next->as.multi.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                next->as.multi.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                next->as.divide.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                next->as.divide.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
                BindExprRec right_rec = parse_bin_stmt_rec(p, next, ptr + 2 * last_j, ptr_max);
                if (top->kind == BIN_EXPR_ADD) top->as.add.rhs = right_rec;
                else top->as.minus.rhs = right_rec;
            } else {
                /* no higher-precedence op found on right; if we've reached end, take token at ptr+2*j-1 */
                /* j now equals the value where loop stopped; compute candidate index = ptr + 2*j - 1 */
                int candidate_idx = ptr + 2 * j - 1;
                if (candidate_idx <= ptr_max) {
                    OptionalToken tok2 = parser_peek(p, candidate_idx);
                    if (tok2.has_value) {
                        NodeExpr* node = (NodeExpr*)malloc(sizeof(NodeExpr));
                        if (!node) { fprintf(stderr,"Out of memory\n"); exit(1); }
                        if (tok2.value.type == token_type_ident) {
                            node->kind = NODE_EXPR_IDENT;
                            node->as.ident.ident = tok2.value;
                        } else if (tok2.value.type == token_type_int_lit) {
                            node->kind = NODE_EXPR_INT_LIT;
                            node->as.int_lit.int_lit = tok2.value;
                        } else {
                            free(node);
                            fprintf(stderr,"unexpected token type for rhs leaf: %d\n", tok2.value.type);
                            exit(1);
                        }
                        BindExprRec right = { .type = NODE_EXPR, .as.node_expr = node };
                        if (top->kind == BIN_EXPR_ADD) top->as.add.rhs = right;
                        else top->as.minus.rhs = right;
                    } else {
                        fprintf(stderr,"some error in writing equation (rhs missing)\n");
                        exit(1);
                    }
                }
            }
        }

        /* immediate rhs (in case not already set) */
        OptionalToken tok3 = parser_peek(p, ptr + 1);
        if (tok3.has_value) {
            BindExprRec *maybe_rhs = (top->kind == BIN_EXPR_ADD) ? &top->as.add.rhs : &top->as.minus.rhs;
            if (maybe_rhs->type != BIN_EXPR || maybe_rhs->as.node_expr == NULL) {
                NodeExpr* node = (NodeExpr*)malloc(sizeof(NodeExpr));
                if (!node) { fprintf(stderr,"Out of memory\n"); exit(1); }
                if (tok3.value.type == token_type_ident) {
                    node->kind = NODE_EXPR_IDENT;
                    node->as.ident.ident = tok3.value;
                } else if (tok3.value.type == token_type_int_lit) {
                    node->kind = NODE_EXPR_INT_LIT;
                    node->as.int_lit.int_lit = tok3.value;
                } else {
                    free(node);
                    fprintf(stderr,"unexpected token type for immediate rhs: %d\n", tok3.value.type);
                    exit(1);
                }
                BindExprRec right = { .type = NODE_EXPR, .as.node_expr = node };
                if (top->kind == BIN_EXPR_ADD) top->as.add.rhs = right;
                else top->as.minus.rhs = right;
            }
        }
    }
    else if (top->kind == BIN_EXPR_MULTI || top->kind == BIN_EXPR_DIVIDE) {
        /* MULTI / DIVIDE: handle left (chain of * or /) and immediate rhs */

        // left: check if token at ptr-2 is multi/divide -> chain, else leaf at ptr-1
        if (parser_peek(p, ptr - 2).has_value && 
            (parser_peek(p, ptr - 2).value.type == token_type_multi || parser_peek(p, ptr - 2).value.type == token_type_divide)) {
            OptionalToken op = parser_peek(p, ptr - 2);
            BinExpr* next = (BinExpr*)malloc(sizeof(BinExpr));
            if (!next) { fprintf(stderr,"Out of memory\n"); exit(1); }
            next->kind = (op.value.type == token_type_multi) ? BIN_EXPR_MULTI : BIN_EXPR_DIVIDE;
            next->as.multi.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            next->as.multi.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            next->as.divide.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            next->as.divide.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
            BindExprRec left_rec = parse_bin_stmt_rec(p, next, ptr - 2, ptr_max);
            if (top->kind == BIN_EXPR_MULTI) top->as.multi.lhs = left_rec;
            else top->as.divide.lhs = left_rec;
        } else {
            OptionalToken tok = parser_peek(p, ptr - 1);
            if (!tok.has_value) { fprintf(stderr,"some error in writing equation (multi/div lhs missing)\n"); exit(1); }
            NodeExpr* node = (NodeExpr*)malloc(sizeof(NodeExpr));
            if (!node) { fprintf(stderr,"Out of memory\n"); exit(1); }
            if (tok.value.type == token_type_ident) {
                node->kind = NODE_EXPR_IDENT;
                node->as.ident.ident = tok.value;
            } else if (tok.value.type == token_type_int_lit) {
                node->kind = NODE_EXPR_INT_LIT;
                node->as.int_lit.int_lit = tok.value;
            } else {
                fprintf(stderr,"unexpected token type for multi/div lhs leaf: %d\n", tok.value.type);
                exit(1);
            }
            BindExprRec left = { .type = NODE_EXPR, .as.node_expr = node };
            if (top->kind == BIN_EXPR_MULTI) top->as.multi.lhs = left;
            else top->as.divide.lhs = left;
        }

        // rhs is immediate token at ptr+1
        OptionalToken tok2 = parser_peek(p, ptr + 1);
        if (!tok2.has_value) { fprintf(stderr,"some error in writing equation (multi/div rhs missing)\n"); exit(1); }
        NodeExpr* node2 = (NodeExpr*)malloc(sizeof(NodeExpr));
        if (!node2) { fprintf(stderr,"Out of memory\n"); exit(1); }
        if (tok2.value.type == token_type_ident) {
            node2->kind = NODE_EXPR_IDENT;
            node2->as.ident.ident = tok2.value;
        } else if (tok2.value.type == token_type_int_lit) {
            node2->kind = NODE_EXPR_INT_LIT;
            node2->as.int_lit.int_lit = tok2.value;
        } else {
            free(node2);
            fprintf(stderr,"unexpected token type for multi/div rhs leaf: %d\n", tok2.value.type);
            exit(1);
        }
        BindExprRec right = { .type = NODE_EXPR, .as.node_expr = node2 };
        if (top->kind == BIN_EXPR_MULTI) top->as.multi.rhs = right;
        else top->as.divide.rhs = right;
    }

    return (BindExprRec){ .type = BIN_EXPR, .as.bin_expr = top };
}
