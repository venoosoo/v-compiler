#pragma once
#include "../parser.h"

struct BindExprRec parse_bin_stmt_rec(Parser_data* p, BinExpr* top, int ptr,const int ptr_max);
