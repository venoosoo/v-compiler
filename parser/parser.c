#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

// ---- Parser data initialization ----
Parser_data* init_parser(TokenArray src) {
    Parser_data* p = (Parser_data*)malloc(sizeof(Parser_data));
    if (!p) return NULL;
    p->m_index = 0;
    p->m_tokens = src;
    return p;
}

// ---- Peek next token ----
static inline OptionalToken parser_peek(Parser_data* p, int offset) {
    OptionalToken result;
    if (p->m_index + offset >= kv_size(p->m_tokens)) {
        result.has_value = 0;
        Token empty = { .type = token_empty, .value = NULL };
        result.value = empty;
    } else {
        result.has_value = 1;
        result.value = kv_A(p->m_tokens, p->m_index + offset);
    }
    return result;
}

// ---- Consume token ----
static inline Token parser_consume(Parser_data* p) {
    return kv_A(p->m_tokens, p->m_index++);
}

// ---- Parse expression ----
OptionalNodeExpr parse_expr(Parser_data* p) {
    OptionalNodeExpr result;
    OptionalToken t = parser_peek(p, 0);

    if (t.has_value && t.value.type == token_type_int_lit) {
        result.has_value = 1;
        NodeExpr expr;
        expr.kind = NODE_EXPR_INT_LIT;
        expr.as.int_lit.int_lit = parser_consume(p);
        result.value = expr;
    } else if (t.has_value && t.value.type == token_type_ident) {
        result.has_value = 1;
        NodeExpr expr;
        expr.kind = NODE_EXPR_IDENT;
        expr.as.ident.ident = parser_consume(p);
        result.value = expr;
    } else {
        result.has_value = 0;
        NodeExpr expr = {0};
        expr.kind = NODE_EXPR_EMPTY;
        result.value = expr;
    }

    return result;
}






// ---- Parse statement ----
OptionalNodeStmt parse_stmt(Parser_data* p) {



    if (parser_peek(p, 0).has_value && parser_peek(p, 0).value.type == token_type_exit_kw) {
        parser_consume(p);
        

        if (!(parser_peek(p, 0).has_value && parser_peek(p, 0).value.type == token_type_open_paren)) {
            fprintf(stderr, "Expected '(' after exit\n");
        }
        parser_consume(p);

        NodeStmtExit stmt_exit;
        OptionalNodeExpr expr = parse_expr(p);
        if (expr.has_value) stmt_exit.expr = expr.value;
        else fprintf(stderr, "Invalid expression after exit\n");

        if (!(parser_peek(p, 0).has_value && parser_peek(p, 0).value.type == token_type_close_paren)) {
            fprintf(stderr, "Expected ')'\n");
        } else {
            parser_consume(p);
        }

        if (!(parser_peek(p, 0).has_value && parser_peek(p, 0).value.type == token_type_semi)) {
            fprintf(stderr, "Expected ';'\n");
        } else {
            parser_consume(p);
        }

        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_EXIT;
        node_stmt.as.exit_ = stmt_exit;

        OptionalNodeStmt result = { .has_value = 1, .value = node_stmt };
        return result;
    }

    // ---- Parse let statement ----
    OptionalToken t1 = parser_peek(p, 1);
    OptionalToken t2 = parser_peek(p, 2);





    if (parser_peek(p,0).has_value && parser_peek(p,0).value.type  == token_type_let_kw &&
        t1.has_value && t1.value.type == token_type_ident &&
        t2.has_value && t2.value.type == token_type_eq_kw) {
        parser_consume(p); // consume let
        NodeStmtLet stmt_let;
        stmt_let.ident = parser_consume(p);
        parser_consume(p); // consume equal
        OptionalNodeExpr expr = parse_expr(p);
        if (expr.has_value) {
            stmt_let.expr = expr.value;
        }else {
            fprintf(stderr, "Invalid expression after let\n");
        }
        if (!(parser_peek(p, 0).has_value && parser_peek(p, 0).value.type == token_type_semi)) {
            fprintf(stderr, "Expected ';'\n");
        } else { 
            parser_consume(p);
        }
        NodeStmt node_stmt;
        node_stmt.kind = NODE_STMT_LET;
        node_stmt.as.let = stmt_let;

        OptionalNodeStmt result = { .has_value = 1, .value = node_stmt };
        return result;
    }

    return (OptionalNodeStmt){ .has_value = 0 };
}

// ---- Parse program ----
OptionalNodeProg parse_prog(Parser_data* p) {
    OptionalNodeProg result;
    kv_init(result.value.stmt);


    while (parser_peek(p, 0).has_value) {

        OptionalNodeStmt stmt = parse_stmt(p);
        if (stmt.has_value) {
            kv_push(NodeStmt, result.value.stmt, stmt.value);
        } else {
            //fprintf(stderr, "Invalid statement2\n");
        }
    }

    result.has_value = 1;
    return result;
}
