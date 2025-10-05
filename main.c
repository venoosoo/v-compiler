#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "tokenizer/tokenizer.h"
#include "parser/parser.h"
#include "generation/generation.h"



typedef struct {
    char *data;
    size_t size;
} StringView;

int load_file(const char *filename, StringView *out) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    if (st.st_size == 0) {
        fprintf(stderr, "File is empty: %s\n", filename);
        close(fd);
        return EXIT_FAILURE;
    }

    char *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    // fill result
    out->data = mapped;
    out->size = (size_t)st.st_size;

    close(fd); // safe to close after mmap
    return EXIT_SUCCESS;
}

void print_tokens(TokenArray* tokens) {
    for (size_t i = 0; i < tokens->n; i++) {
        Token tok = tokens->a[i];
        printf("Token %zu: type = ", i);
        switch (tok.type) {
            case token_type_exit_kw:       printf("exit_kw"); break;
            case token_type_int_lit:       printf("int_lit"); break;
            case token_type_semi:          printf("semi"); break;
            case token_type_open_paren:    printf("open_paren"); break;
            case token_type_close_paren:   printf("close_paren"); break;
            case token_type_ident:         printf("ident"); break;
            case token_type_let_kw:        printf("let_kw"); break;
            case token_type_eq_kw:         printf("eq_kw"); break;
            case token_type_plus:          printf("plus"); break;
            case token_type_multi:         printf("multi"); break;
            case token_type_minus:         printf("minus"); break;
            case token_type_divide:        printf("divide"); break;
            case token_type_if:            printf("if"); break;
            case token_type_else:          printf("else"); break;
            case token_type_open_braces:   printf("open_braces"); break;
            case token_type_close_braces:  printf("close_braces"); break;
            case token_type_cmp:           printf("cmp"); break;
            case token_type_less:          printf("less"); break;
            case token_type_less_eq:       printf("less_eq"); break;
            case token_type_more:          printf("more"); break;
            case token_type_more_eq:       printf("more_eq"); break;
            case token_type_not_eq:        printf("not_eq"); break;
            case token_type_and:           printf("and"); break;
            case token_type_or:            printf("or"); break;
            case token_type_while:            printf("while"); break;
            case token_type_for:            printf("for"); break;
            case token_empty:              printf("empty"); break;
            default:                       printf("unknown"); break;
        }

        if (tok.value) {
            printf(", value = '%s'\n", tok.value);
        } else {
            printf(", value = NULL\n");
        }
    }
}



void print_optional_prog(OptionalNodeProg* prog) {
    if (!prog->has_value) {
        printf("Program is empty\n");
        return;
    }

    NodeProg* p = &prog->value;
    size_t n = kv_size(p->stmt);

    for (size_t i = 0; i < n; i++) {
        NodeStmt* stmt = &kv_A(p->stmt, i);

        switch (stmt->kind) {
            case NODE_STMT_EXIT:
                printf("Statement %zu: EXIT\n", i);
                switch (stmt->as.exit_.expr.kind) {
                    case NODE_EXPR_INT_LIT:
                        printf("  Expr: INT_LIT\n");
                        break;
                    case NODE_EXPR_IDENT:
                        printf("  Expr: IDENT\n");
                        break;
                    case NODE_EXPR_BIN:
                        printf("  Expr: BIN\n");
                        break;
                    case NODE_EXPR_EMPTY:
                        printf("  Expr: EMPTY\n");
                        break;
                }
                break;

            case NODE_STMT_LET:
                printf("Statement %zu: LET\n", i);
                switch (stmt->as.let.expr.kind) {
                    case NODE_EXPR_INT_LIT:
                        printf("  Expr: INT_LIT\n");
                        break;
                    case NODE_EXPR_IDENT:
                        printf("  Expr: IDENT\n");
                        break;
                    case NODE_EXPR_BIN:
                        printf("  Expr: BIN\n");
                        break;
                    case NODE_EXPR_EMPTY:
                        printf("  Expr: EMPTY\n");
                        break;
                }
                break;
            case NODE_STMT_IF:
                printf("Statement %zu: IF\n",i);
                switch (stmt->as.if_.cond.kind) {
                    case NODE_EXPR_INT_LIT:
                        printf("    Expr: INT_LIT\n");
                        break;
                    case NODE_EXPR_IDENT:
                        printf("    Expr: IDENT\n");
                        break;
                    case NODE_EXPR_BIN:
                        printf("    Expr: BIN\n");
                        break;
                }
                break;
            case NODE_STMT_ELSE:
                printf("Statement %zu: ELSE\n",i);
                break;
            case NODE_STMT_WHILE:
                printf("Statement %zu: WHILE\n",i);
                switch (stmt->as.while_.cond.kind) {
                    case NODE_EXPR_INT_LIT:
                        printf("    Expr: INT_LIT\n");
                        break;
                    case NODE_EXPR_IDENT:
                        printf("    Expr: IDENT\n");
                        break;
                    case NODE_EXPR_BIN:
                        printf("    Expr: BIN\n");
                        break;
                }
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "No file provided\n");
        fprintf(stderr, "Usage: %s <input.v>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // file reading
    StringView content;
    if (load_file(argv[1], &content) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    Token_data* t_data = tokenizer_create(content.data);
    TokenArray t_result = tokenize(t_data);
    printf("TOKENIZER RESULT:\n");
    print_tokens(&t_result);

    Parser_data* p_data = init_parser(t_result);
    OptionalNodeProg p_result = parse_prog(p_data);

    free(t_result.a);


    if (!p_result.has_value) {
        fprintf(stderr, "You nigger\n");
    }


    printf("PARSER RESULT:\n");
    print_optional_prog(&p_result);




    
    gen_data* g_data = generate_gen_data(&p_result.value);
    
    FILE* f = fopen("main.asm", "w");
    if (!f) {
        printf("error in file\n");
        return 1;
    }

    
    fprintf(f, gen_prog(g_data)); 
    
    fclose(f);



    printf("compiled\n");

    system("nasm -felf64 main.asm");
    system("ld -o v main.o");



    munmap(content.data, content.size);
    return EXIT_SUCCESS;
}


