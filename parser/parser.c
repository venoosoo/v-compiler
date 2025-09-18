// parser.c
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

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

// ---- Consume token ----
static inline Token parser_consume(Parser_data* p) {
    return kv_A(p->m_tokens, p->m_index++);
}

// ---- Print binary expression (debug helper) ----
void print_bin_expr(BinExpr* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("  ");

    switch (node->kind) {
        case BIN_EXPR_ADD:
            printf("+\n");
            if (node->as.add.lhs.type == BIN_EXPR) print_bin_expr(node->as.add.lhs.as.bin_expr, depth + 1);
            else {
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
                for (int i = 0; i < depth + 1; i++) printf("  ");
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
        default:
            printf("???\n");
            break;
    }
}

// ---- Parse binary statement ----
OptionalBinExpr parse_bin_stmt(Parser_data* p) {
    int ptr = 0;
    while (parser_peek(p, ptr).has_value && parser_peek(p, ptr).value.type != token_type_semi) {
        ptr++;
    }
    const int ptr_max = ptr - 1;

    // debug
    printf("[debug] parse_bin_stmt: m_index=%d ptr=%d ptr_max=%d\n", p->m_index, ptr, ptr_max);
    printf("[debug] tokens slice:\n");
    for (int i = 0; i <= ptr_max; i++) {
        OptionalToken tt = parser_peek(p, i);
        if (!tt.has_value) printf("  [%d]: <no token>\n", i);
        else printf("  [%d]: type=%d val=%s\n", i, tt.value.type, tt.value.value ? tt.value.value : "NULL");
    }

    // Find operator index (+,- first, then *,/)
    int op_index = -1;
    TokenType op_type = 0;
    for (int i = ptr_max; i >= 0; i--) {
        OptionalToken t = parser_peek(p, i);
        if (!t.has_value) continue;
        if (t.value.type == token_type_plus || t.value.type == token_type_minus) {
            op_index = i;
            op_type = t.value.type;
            break;
        }
    }
    if (op_index == -1) {
        for (int i = ptr_max; i >= 0; i--) {
            OptionalToken t = parser_peek(p, i);
            if (!t.has_value) continue;
            if (t.value.type == token_type_multi || t.value.type == token_type_divide) {
                op_index = i;
                op_type = t.value.type;
                break;
            }
        }
    }

    OptionalBinExpr res = {0};
    if (op_index == -1) {
        printf("[debug] parse_bin_stmt: no operator found in slice (ptr_max=%d)\n", ptr_max);
        res.has_value = 0;
        return res;
    }

    printf("[debug] parse_bin_stmt: found op at index=%d type=%d\n", op_index, op_type);

    BinExpr* top = (BinExpr*)malloc(sizeof(BinExpr));
    if (!top) { fprintf(stderr,"Out of memory\n"); exit(1); }

    switch (op_type) {
        case token_type_plus: top->kind = BIN_EXPR_ADD; break;
        case token_type_minus: top->kind = BIN_EXPR_MINUS; break;
        case token_type_multi: top->kind = BIN_EXPR_MULTI; break;
        case token_type_divide: top->kind = BIN_EXPR_DIVIDE; break;
        default:
            fprintf(stderr, "Unknown operator\n");
            free(top);
            exit(1);
    }

    // initialize children to NODE_EXPR with NULL pointer to avoid garbage
    top->as.add.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    top->as.add.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    top->as.minus.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    top->as.minus.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    top->as.multi.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    top->as.multi.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    top->as.divide.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    top->as.divide.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };

    BindExprRec rec = parse_bin_stmt_rec(p, top, op_index, ptr_max);

    if (rec.type != BIN_EXPR || rec.as.bin_expr == NULL) {
        printf("[debug] parse_bin_stmt: parse_bin_stmt_rec returned non-BIN_EXPR or NULL\n");
        if (top) free(top);
        res.has_value = 0;
        return res;
    }

    res.has_value = 1;
    res.value = *rec.as.bin_expr;

    printf("[debug] parse_bin_stmt: parsed tree:\n");
    print_bin_expr(rec.as.bin_expr, 0);

    // consume tokens that belonged to this expression
    for (int i = 0; i <= ptr_max; i++) parser_consume(p);
    return res;
}

// ---- Parse expression ----
OptionalNodeExpr parse_expr(Parser_data* p) {
    OptionalNodeExpr result = {0};
    OptionalToken t = parser_peek(p, 0);
    if (!t.has_value) return result;

    if (t.value.type == token_type_int_lit || t.value.type == token_type_ident) {
        OptionalToken t1 = parser_peek(p, 1);
        OptionalToken t2 = parser_peek(p, 2);

        if (t1.has_value && t2.has_value &&
            (t1.value.type == token_type_plus || t1.value.type == token_type_minus ||
             t1.value.type == token_type_multi || t1.value.type == token_type_divide) &&
            (t2.value.type == token_type_int_lit || t2.value.type == token_type_ident)) {

            OptionalBinExpr bin = parse_bin_stmt(p);
            printf("[debug] parse_expr: bin stmt has_value result: %d\n", bin.has_value);
            if (!bin.has_value) {
                printf("[debug] parse_expr: parse_bin_stmt failed, returning empty expr\n");
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

            printf("[debug] parse_expr: created NODE_EXPR_BIN, tree:\n");
            print_bin_expr(expr_heap->as.bin, 0);
            return result;
        } else {
            NodeExpr expr;
            if (t.value.type == token_type_int_lit) {
                expr.kind = NODE_EXPR_INT_LIT;
                expr.as.int_lit.int_lit = parser_consume(p);
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
        parser_consume(p);
        if (!(parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_open_paren)) {
            fprintf(stderr,"Expected '('\n"); exit(1);
        }
        parser_consume(p);
        NodeStmtExit stmt_exit;
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
    if (t0.has_value && t0.value.type == token_type_let_kw &&
        t1.has_value && t1.value.type == token_type_ident &&
        t2.has_value && t2.value.type == token_type_eq_kw) {

        parser_consume(p); // let
        NodeStmtLet stmt_let;
        stmt_let.ident = parser_consume(p);
        parser_consume(p); // =
        OptionalNodeExpr expr = parse_expr(p);
        if (!expr.has_value) { fprintf(stderr,"Invalid expression after let\n"); exit(1); }
        stmt_let.expr = expr.value;
        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_LET;
        node_stmt.as.let = stmt_let;
        if (parser_peek(p,0).has_value && parser_peek(p,0).value.type == token_type_semi) parser_consume(p);
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
        if (!stmt.has_value) { fprintf(stderr,"Failed to parse statement\n"); exit(1); }
        kv_push(NodeStmt, result.value.stmt, stmt.value);
    }

    result.has_value = 1;
    return result;
}
