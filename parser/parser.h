#pragma once

#include "../libs/kvec.h"
#include "../tokenizer/tokenizer.h"  // Token, TokenArray

// ---- Forward declarations ----
struct Parser_data;
typedef struct Parser_data Parser_data;

// ---- Parser data ----
struct Parser_data {
    int m_index;
    TokenArray m_tokens;
};

// ---- Parser functions ----
Parser_data* init_parser(TokenArray src);

// ---- Expression types ----
typedef struct NodeExprIntLit {
    Token int_lit;
} NodeExprIntLit;

typedef struct NodeExprIdent {
    Token ident;
} NodeExprIdent;

typedef struct BinExprAdd BinExprAdd;
typedef struct BinExprMulti BinExprMulti;
typedef struct BinExpr BinExpr;



struct BinExprAdd {
    struct NodeExpr* lhs;
    struct NodeExpr* rhs;
};

struct BinExprMulti {
    struct NodeExpr* lhs;
    struct NodeExpr* rhs;
};

typedef enum {
    BIN_EXPR_ADD,
    BIN_EXPR_MULTI
} BinExprKind;



struct BinExpr {
    BinExprKind kind;
    union {
        BinExprAdd add;
        BinExprMulti multi;
    } as;
};

typedef enum {
    NODE_EXPR_INT_LIT,
    NODE_EXPR_IDENT,
    NODE_EXPR_BIN,
    NODE_EXPR_EMPTY 
} NodeExprKind;

typedef struct NodeExpr {
    NodeExprKind kind;
    union {
        NodeExprIntLit int_lit;
        NodeExprIdent ident;
        BinExpr* bin;
    } as;
} NodeExpr;

// ---- Statement types ----
typedef struct NodeStmtExit {
    NodeExpr expr;
} NodeStmtExit;

typedef struct NodeStmtLet {
    Token ident;
    NodeExpr expr;
} NodeStmtLet;

typedef enum {
    NODE_STMT_EXIT,
    NODE_STMT_LET
} NodeStmtKind;

typedef struct NodeStmt {
    NodeStmtKind kind;
    union {
        NodeStmtLet let;
        NodeStmtExit exit_;
    } as;
} NodeStmt;

typedef kvec_t(NodeStmt) NodeStmtArray;

// ---- Program ----
typedef struct NodeProg {
    NodeStmtArray stmt;
} NodeProg;

// ---- Optional wrappers ----
typedef struct OptionalNodeExpr {
    int has_value;
    NodeExpr value;
} OptionalNodeExpr;

typedef struct OptionalNodeStmt {
    int has_value;
    NodeStmt value;
} OptionalNodeStmt;

typedef struct OptionalNodeProg {
    int has_value;
    NodeProg value;
} OptionalNodeProg;

typedef struct OptionalToken {
    int has_value;
    Token value;
} OptionalToken;

// ---- Parser function declarations ----
OptionalNodeExpr parse_expr(Parser_data* p);
OptionalNodeStmt parse_stmt(Parser_data* p);
OptionalNodeProg parse_prog(Parser_data* p);

// ---- Private helpers (renamed to avoid conflict) ----
static inline OptionalToken parser_peek(Parser_data* p, int offset);
static inline Token parser_consume(Parser_data* p);
