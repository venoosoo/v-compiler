// parser/binstmt/binstmt.c
#include "../parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ---------------------
// Operator precedence & associativity
// ---------------------
typedef enum { ASSOC_LEFT, ASSOC_RIGHT } Assoc;

static int op_precedence(TokenType t) {
    switch (t) {
        // highest: * /
        case token_type_multi: case token_type_divide: return 80;
        // + -
        case token_type_plus: case token_type_minus:  return 70;
        // comparisons
        case token_type_less: case token_type_less_eq:
        case token_type_more: case token_type_more_eq: return 50;
        // equality
        case token_type_cmp: case token_type_not_eq:  return 45;
        // logical AND, OR
        case token_type_and:        /* logical && */   return 25;
        case token_type_or:         /* logical || */   return 20;
        default: return -1;
    }
}

static Assoc op_assoc(TokenType t) {
    // assignment would be right-assoc; none of our binary ops are right-assoc
    (void)t;
    return ASSOC_LEFT;
}

// Map token types -> BinExprKind.
static BinExprKind token_to_bin_kind(TokenType t) {
    switch (t) {
        case token_type_plus:   return BIN_EXPR_ADD;
        case token_type_minus:  return BIN_EXPR_MINUS;
        case token_type_multi:  return BIN_EXPR_MULTI;
        case token_type_divide: return BIN_EXPR_DIVIDE;
        case token_type_cmp:    return BIN_EXPR_EQ;
        case token_type_not_eq: return BIN_EXPR_NEQ;
        case token_type_less:   return BIN_EXPR_LT;
        case token_type_less_eq:return BIN_EXPR_LTE;
        case token_type_more:   return BIN_EXPR_MR;
        case token_type_more_eq:return BIN_EXPR_MRE;
        case token_type_and:    return BIN_EXPR_AND;
        case token_type_or:     return BIN_EXPR_OR;
        default:
            fprintf(stderr, "token_to_bin_kind: unhandled token %d\n", t);
            exit(1);
    }
}

// ---------------------
// Helpers that operate on a Parser_data "slice"
// ---------------------
static OptionalToken slice_peek(Parser_data* p, int offset) {
    OptionalToken res;
    int idx = p->m_index + offset;
    if (idx < 0 || idx >= kv_size(p->m_tokens)) {
        res.has_value = 0;
        Token empty = { .type = token_empty, .value = NULL };
        res.value = empty;
    } else {
        res.has_value = 1;
        res.value = kv_A(p->m_tokens, idx);
    }
    return res;
}

static Token slice_consume(Parser_data* p) {
    Token t = kv_A(p->m_tokens, p->m_index);
    p->m_index += 1;
    return t;
}

// ---------------------
// Constructors for NodeExpr/BinExpr
// ---------------------
static NodeExpr make_int(Token tok) {
    NodeExpr n;
    n.kind = NODE_EXPR_INT_LIT;
    n.as.int_lit.int_lit = tok;
    return n;
}

static NodeExpr make_ident(Token tok) {
    NodeExpr n;
    n.kind = NODE_EXPR_IDENT;
    n.as.ident.ident = tok;
    return n;
}

static BindExprRec nodeexpr_value_to_bindrec(const NodeExpr *v) {
    BindExprRec rec;
    if (v->kind == NODE_EXPR_BIN && v->as.bin != NULL) {
        rec.type = BIN_EXPR;
        rec.as.bin_expr = v->as.bin;
    } else {
        NodeExpr *heap_n = (NodeExpr*)malloc(sizeof(NodeExpr));
        if (!heap_n) { fprintf(stderr,"Out of memory\n"); exit(1); }
        *heap_n = *v;
        rec.type = NODE_EXPR;
        rec.as.node_expr = heap_n;
    }
    return rec;
}

static NodeExpr make_bin_from_nodes(BinExprKind kind, NodeExpr left_val, NodeExpr right_val) {
    BinExpr* b = (BinExpr*)malloc(sizeof(BinExpr));
    if (!b) { fprintf(stderr,"Out of memory\n"); exit(1); }
    b->kind = kind;

    // initialize union members/children to safe NULLs
    b->as.add.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.add.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.minus.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.minus.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.multi.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.multi.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.divide.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.divide.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.binop.lhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };
    b->as.binop.rhs = (BindExprRec){ .type = NODE_EXPR, .as.node_expr = NULL };

    BindExprRec left_rec  = nodeexpr_value_to_bindrec(&left_val);
    BindExprRec right_rec = nodeexpr_value_to_bindrec(&right_val);

    switch (kind) {
        case BIN_EXPR_ADD:    b->as.add.lhs = left_rec;    b->as.add.rhs = right_rec; break;
        case BIN_EXPR_MINUS:  b->as.minus.lhs = left_rec;  b->as.minus.rhs = right_rec; break;
        case BIN_EXPR_MULTI:  b->as.multi.lhs = left_rec;  b->as.multi.rhs = right_rec; break;
        case BIN_EXPR_DIVIDE: b->as.divide.lhs = left_rec; b->as.divide.rhs = right_rec; break;
        case BIN_EXPR_EQ: case BIN_EXPR_NEQ:
        case BIN_EXPR_LT: case BIN_EXPR_LTE:
        case BIN_EXPR_MR: case BIN_EXPR_MRE:
        case BIN_EXPR_AND: case BIN_EXPR_OR:
            b->as.binop.lhs = left_rec;
            b->as.binop.rhs = right_rec;
            break;
        default:
            fprintf(stderr,"make_bin_from_nodes: unsupported BinExprKind %d\n", kind);
            exit(1);
    }

    NodeExpr out;
    out.kind = NODE_EXPR_BIN;
    out.as.bin = b;
    return out;
}

// ---------------------
// Pratt / precedence-climbing parser on the given Parser_data
// ---------------------
static NodeExpr parse_expr_prec_on_slice(Parser_data* p, int min_prec); // forward

static NodeExpr parse_primary_on_slice(Parser_data* p) {
    OptionalToken t = slice_peek(p, 0);
    if (!t.has_value) {
        fprintf(stderr,"parse_primary_on_slice: unexpected end\n");
        exit(1);
    }

    if (t.value.type == token_type_int_lit) {
        Token tk = slice_consume(p);
        return make_int(tk);
    }
    if (t.value.type == token_type_ident) {
        Token tk = slice_consume(p);
        return make_ident(tk);
    }
    if (t.value.type == token_type_open_paren) {
        slice_consume(p); // consume '('
        NodeExpr inside = parse_expr_prec_on_slice(p, 0);
        OptionalToken close = slice_peek(p, 0);
        if (!close.has_value || close.value.type != token_type_close_paren) {
            fprintf(stderr,"parse_primary_on_slice: expected ')'\n");
            exit(1);
        }
        slice_consume(p); // consume ')'
        return inside;
    }

    fprintf(stderr,"parse_primary_on_slice: unexpected token %d\n", t.value.type);
    exit(1);
}

static NodeExpr parse_expr_prec_on_slice(Parser_data* p, int min_prec) {
    NodeExpr left = parse_primary_on_slice(p);

    while (1) {
        OptionalToken op = slice_peek(p, 0);
        if (!op.has_value) break;
        int prec = op_precedence(op.value.type);
        if (prec < min_prec) break;

        Assoc assoc = op_assoc(op.value.type);
        int next_min = (assoc == ASSOC_LEFT) ? prec + 1 : prec;

        // consume operator
        Token op_tok = slice_consume(p);

        // parse RHS
        NodeExpr right = parse_expr_prec_on_slice(p, next_min);

        // map token -> BinExprKind and build node
        BinExprKind kind = token_to_bin_kind(op_tok.type);
        left = make_bin_from_nodes(kind, left, right);
    }

    return left;
}

// ---------------------
// Public: parse a contiguous expression starting at p->m_index until the first ')' or ';'.
// It consumes expression tokens by advancing p->m_index TO the terminator (but not the terminator).
// ---------------------
OptionalNodeExpr parse_expr_to_terminator(Parser_data* p) {
    OptionalNodeExpr res = {0};

    // scan for next ')' or ';'
    int base = p->m_index;
    int found_idx = -1;
    int n = kv_size(p->m_tokens);
    for (int i = base; i < n; ++i) {
        Token tk = kv_A(p->m_tokens, i);
        if (tk.type == token_type_close_paren || tk.type == token_type_semi) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) {
        // no terminator: fail
        res.has_value = 0;
        return res;
    }

    int ptr_max = found_idx - base - 1; // last token index of expression relative to base
    if (ptr_max < 0) {
        // empty expression
        res.has_value = 0;
        return res;
    }

    // build temporary slice [base .. base + ptr_max]
    TokenArray slice;
    kv_init(slice);
    for (int i = 0; i <= ptr_max; ++i) {
        kv_push(Token, slice, kv_A(p->m_tokens, base + i));
    }

    Parser_data subp;
    subp.m_index = 0;
    subp.m_tokens = slice;

    NodeExpr parsed = parse_expr_prec_on_slice(&subp, 0);

    // advance parent parser index to the terminator (leave terminator for caller)
    p->m_index = base + ptr_max + 1;

    // convert to return type: if bin -> return BIN_EXPR, else NODE_EXPR (heap-allocate leaf)
    if (parsed.kind == NODE_EXPR_BIN && parsed.as.bin != NULL) {
        res.has_value = 1;
        res.value = parsed;
    } else {
        // copy leaf to heap to match NodeExpr usage elsewhere (caller expects NodeExpr by value)
        NodeExpr heap_n;
        heap_n.kind = parsed.kind;
        heap_n.as = parsed.as;
        res.has_value = 1;
        res.value = heap_n;
    }

    // Note: slice tokens are copies of tokenizer tokens; tokenizer-owned memory remains valid.
    return res;
}
