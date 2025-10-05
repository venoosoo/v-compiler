#pragma once

#include "../libs/kvec.h"
#include "../tokenizer/tokenizer.h"

struct Parser_data;
typedef struct Parser_data Parser_data;
typedef struct NodeExpr NodeExpr;
typedef struct NodeStmt NodeStmt;
typedef kvec_t(NodeStmt) NodeStmtArray;

struct Parser_data {
    int m_index;
    TokenArray m_tokens;
};

typedef struct NodeExprIntLit {
    Token int_lit;
} NodeExprIntLit;

typedef struct NodeExprIdent {
    Token ident;
} NodeExprIdent;

typedef struct BinExprAdd BinExprAdd;
typedef struct BinExprMulti BinExprMulti;
typedef struct BinExprMinus BinExprMinus;
typedef struct BinExprDivide BinExprDivide;
typedef struct BinExprBinOp BinExprBinOp;
typedef struct BinExpr BinExpr;

typedef enum {
    BIN_EXPR,
    NODE_EXPR,
} BindExprRecKind;

typedef struct BindExprRec {
    BindExprRecKind type;
    union {
        BinExpr* bin_expr;
        NodeExpr* node_expr;
    } as;
} BindExprRec;

struct BinExprAdd { BindExprRec lhs; BindExprRec rhs; };
struct BinExprMinus { BindExprRec lhs; BindExprRec rhs; };
struct BinExprMulti { BindExprRec lhs; BindExprRec rhs; };
struct BinExprDivide { BindExprRec lhs; BindExprRec rhs; };
struct BinExprBinOp { BindExprRec lhs; BindExprRec rhs; };

typedef enum {
    BIN_EXPR_ADD,
    BIN_EXPR_MULTI,
    BIN_EXPR_MINUS,
    BIN_EXPR_DIVIDE,
    BIN_EXPR_EQ,
    BIN_EXPR_NEQ,
    BIN_EXPR_LT,
    BIN_EXPR_LTE,
    BIN_EXPR_MR,
    BIN_EXPR_MRE,
    BIN_EXPR_AND,
    BIN_EXPR_OR,
} BinExprKind;

struct BinExpr {
    BinExprKind kind;
    union {
        BinExprAdd add;
        BinExprMulti multi;
        BinExprMinus minus;
        BinExprDivide divide;
        BinExprBinOp binop;
    } as;
};

typedef enum {
    NODE_EXPR_INT_LIT,
    NODE_EXPR_IDENT,
    NODE_EXPR_BIN,
    NODE_EXPR_EMPTY
} NodeExprKind;

struct NodeExpr {
    NodeExprKind kind;
    union {
        NodeExprIntLit int_lit;
        NodeExprIdent ident;
        BinExpr* bin;
    } as;
};

typedef struct NodeStmtExit {
    NodeExpr expr;
} NodeStmtExit;

typedef struct NodeStmtLet {
    Token ident;
    NodeExpr expr;
} NodeStmtLet;


typedef struct NodeStmtVchange {
    Token ident;
    NodeExpr expr;
} NodeStmtVchange;

typedef enum {
    NODE_STMT_EXIT,
    NODE_STMT_LET,
    NODE_STMT_VCHANGE,
    NODE_STMT_IF,
    NODE_STMT_ELSE,
    NODE_STMT_WHILE,
} NodeStmtKind;

typedef struct NodeStmtIf {
    NodeExpr cond;
    NodeStmtArray body;
} NodeStmtIf;

typedef struct NodeStmtElse {
    NodeStmtArray body;
} NodeStmtElse;

typedef struct NodeStmtWhile {
    NodeExpr cond;
    NodeStmtArray body;
} NodeStmtWhile;

typedef struct NodeStmt {
    NodeStmtKind kind;
    union {
        NodeStmtLet let;
        NodeStmtExit exit_;
        NodeStmtIf if_;
        NodeStmtElse else_;
        NodeStmtWhile while_;
        NodeStmtVchange vchange;
    } as;
} NodeStmt;

typedef struct NodeProg {
    NodeStmtArray stmt;
} NodeProg;

typedef struct OptionalNodeExpr {
    int has_value;
    NodeExpr value;
} OptionalNodeExpr;

typedef struct OptionalBinExpr {
    int has_value;
    BinExpr value;
} OptionalBinExpr;

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

typedef struct NodeIf {
    NodeExpr* lhs;
    Token operator;
    NodeExpr* rhs;
} NodeIf;

Parser_data* init_parser(TokenArray src);

OptionalToken parser_peek(Parser_data* p, int offset);
Token parser_consume(Parser_data* p);

void print_bin_expr(BinExpr* node, int depth);

OptionalBinExpr parse_bin_stmt(Parser_data* p);
OptionalNodeExpr parse_expr(Parser_data* p);
OptionalNodeStmt parse_stmt(Parser_data* p);
OptionalNodeProg parse_prog(Parser_data* p);

OptionalNodeExpr parse_expr_to_terminator(Parser_data* p);

BindExprRec parse_bin_stmt_rec(Parser_data* p, BinExpr* top, int ptr, const int ptr_max);
