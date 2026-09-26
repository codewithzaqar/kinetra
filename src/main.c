#include "../include/kinetra.h"
#include "../include/hpc_math.h"
#include "../include/diagnostics.h"
#include "../include/bytecode.h"
#include "../include/gc.h"
#include <time.h>

void print_usage() {
    printf("Kinetra Language v%s\n", KINETRA_VERSION);
    printf("Usage: kinetra [flags] <source_file.knt>\n");
    printf("Flags:\n");
    printf("  --bench    Run 100 silent iterations and report timing\n");
    printf("  --bc       Execute using the prototype bytecode VM\n");
    printf("  --tokens   Dump the token stream and exit\n");
    printf("  --ast      Dump the AST and exit\n");
    printf("  --repl     Start an interactive REPL\n");
    printf("  --folds    Report how many constant folds the parser performed\n");
    printf("  --hpc      Enable hardware acceleration flags\n");
    printf("  --raw      Suppress banner/stage output (for testing)\n");
    printf("  --version  Print version\n");
    printf("  --help     Show this help messgae\n\n");
    printf("  --gc       Print GC statistics after execution\n");
    printf("Exit codes: 0=ok, 1=lex, 2=parse, 3=runtime, 4=io, 5=codegen, 64=usage\n");
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "[Kinetra Error] Could not open file: %s\n", path);
        exit(KINETRA_EXIT_IO);
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "[Kinetra Error] Could not seek in file: %s\n", path);
        fclose(file);
        exit(KINETRA_EXIT_IO);
    }

    long length = ftell(file);

    if (length < 0) {
        fprintf(stderr, "[Kinetra Error] Could not determine file size: %s\n", path);
        fclose(file);
        exit(KINETRA_EXIT_IO);
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "[Kinetra Error] Could not rewind file: %s\n", path);
        fclose(file);
        exit(KINETRA_EXIT_IO);
    }

    char* buffer = malloc((size_t)length + 1);

    if (!buffer) {
        fprintf(stderr, "[Kinetra Error] Out of memory while reading: %s\n", path);
        fclose(file);
        exit(KINETRA_EXIT_IO);
    }

    size_t read_len = fread(buffer, 1, (size_t)length, file);

    if (ferror(file)) {
        fprintf(stderr, "[Kinetra Error] Failed while reading file: %s\n", path);
        free(buffer);
        fclose(file);
        exit(KINETRA_EXIT_IO);
    }

    buffer[read_len] = '\0';

    fclose(file);

    return buffer;
}

static bool is_expression_node(ASTNode* n) {
    switch (n->type) {
        case NODE_NUMBER_LITERAL:
        case NODE_BOOLEAN_LITERAL:
        case NODE_VARIABLE:
        case NODE_VEC3:
        case NODE_MAT4:
        case NODE_PARTICLE:
        case NODE_ARRAY_LITERAL:
        case NODE_INDEX:
        case NODE_MEMBER:
        case NODE_BINARY_OP:
        case NODE_UNARY_OP:
        case NODE_CALL:
        case NODE_STRING_LITERAL:
            return true;
        default:
            return false;
    }
}

static void run_repl(void) {
    printf("Kinetra v%s REPL - type 'exit' to quit\n", KINETRA_VERSION);

    char line[4096];
    char buffer[16384];
    buffer[0] = '\0';

    jmp_buf repl_jmp;

    // volatile so the values survive longjmp
    volatile ASTNode* repl_ast = NULL;
    volatile Token* repl_tokens = NULL;
    volatile char* repl_source = NULL;

    for (;;) {
        printf(buffer[0] ? "... " : "> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        size_t len = strlen(line);

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0) {
            buffer[0] = '\0';
            continue;
        }

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            break;
        }

        if (buffer[0]) {
            if (strlen(buffer) + len + 2 < sizeof(buffer)) {
                strcat(buffer, "\n");
                strcat(buffer, line);
            } else {
                printf("[REPL] input too large, cleared\n");
                buffer[0] = '\0';
                continue;
            }
        } else {
            strncpy(buffer, line, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
        }

        int balance = 0;

        for (const char* p = buffer; *p; p++) {
            if (*p == '(' || *p == '[' || *p == '{') balance++;
            else if (*p == ')' || *p == ']' || *p == '}') balance--;
        }

        if (balance > 0) {
            continue;
        }

        if (setjmp(repl_jmp) != 0) {
            // Error path: release whatever the failed submission allocated
            if (repl_ast) {
                free_ast((ASTNode*)repl_ast);
                repl_ast = NULL;
            }

            if (repl_tokens) {
                free((void*)repl_tokens);
                repl_tokens = NULL;
            }

            if (repl_source) {
                free((void*)repl_source);
                repl_source = NULL;
            }

            diag_disable_recovery();
            vm_reset_flow();
            buffer[0] = '\0';
            continue;
        }

        diag_enable_recovery(&repl_jmp);

        char* source = malloc(strlen(buffer) + 1);
        strcpy(source, buffer);
        repl_source = source;

        diag_set_source("<repl>", source);

        int token_count = 0;
        Token* tokens = lex(source, &token_count);
        repl_tokens = tokens;

        ASTNode* ast = parse(tokens, token_count);
        repl_ast = ast;

        ASTNode* last = ast->statement_count > 0
            ? ast->statements[ast->statement_count - 1]
            : NULL;

        bool echo = last && is_expression_node(last);

        execute(ast);

        if (echo) {
            vm_eval_and_print(last);
        }

        free_ast(ast);
        repl_ast = NULL;

        free(tokens);
        repl_tokens = NULL;

        free(source);
        repl_source = NULL;

        diag_disable_recovery();
        vm_reset_flow();
        buffer[0] = '\0';
    }
}
int main(int argc, char** argv) {
    bool bench = false;
    bool use_bc = false;
    bool show_folds = false;
    bool dump_tokens = false;
    bool dump_ast = false;
    bool repl = false;
    bool raw = false;
    bool gc_stats = false;
    const char* filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("Kinetra v%s\n", KINETRA_VERSION);
            return KINETRA_EXIT_OK;
        } else if (strcmp(argv[i], "--bench") == 0) {
            bench = true;
        } else if (strcmp(argv[i], "--bc") == 0) {
            use_bc = true;
        } else if (strcmp(argv[i], "--folds") == 0) {
            show_folds = true;
        } else if (strcmp(argv[i], "--tokens") == 0) {
            dump_tokens = true;
        } else if (strcmp(argv[i], "--ast") == 0) {
            dump_ast = true;
        } else if (strcmp(argv[i], "--repl") == 0) {
            repl = true;
        } else if (strcmp(argv[i], "--raw") == 0) {
            raw = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return KINETRA_EXIT_OK;
        } else if (strcmp(argv[i], "--gc") == 0) {
            gc_stats = true;
        } else {
            filename = argv[i];
        }
    }

    if (repl) {
        init_hpc_subsystem();
        run_repl();
        return KINETRA_EXIT_OK;
    }

    if (!filename) {
        print_usage();
        return KINETRA_EXIT_USAGE;
    }

    if (!raw) {
        printf("--- Kinetra Compiler/Runtime v%s ---\n", KINETRA_VERSION);
        init_hpc_subsystem();
        gc_init();
    }

    // 1. Read Source Code
    char* source = read_file(filename);
    diag_set_source(filename, source);
    if (!raw) printf("[1/3] Source loaded: %s\n", filename);

    // 2. Lexical Analysis
    int token_count = 0;
    Token* tokens = lex(source, &token_count);
    if (!raw) printf("[2/3] Lexing complete. Generated %d tokens.\n", token_count);

    if (dump_tokens) {
        for (int i = 0; i < token_count; i++) {
            Token* t = &tokens[i];

            printf(
                "%04d  %3d:%-3d  %-14s %s",
                i,
                t->line,
                t->column,
                token_type_name(t->type),
                t->lexeme
            );

            if (t->type == TOKEN_NUMBER) {
                printf("  value=%g", t->value);
            }

            printf("\n");
        }

        free(tokens);
        free(source);
        return KINETRA_EXIT_OK;
    }

    // 3. Parsing
    ASTNode* ast = parse(tokens, token_count);
    if (!raw) printf("[3/3] Parsing complete. AST generated.\n");

    if (show_folds) {
        printf("[Parser] Constant folds: %d\n", parser_fold_count());
    }

    if (dump_ast) {
        ast_dump(ast);

        free_ast(ast);
        free(tokens);
        free(source);
        return KINETRA_EXIT_OK;
    }

    // 4. Execution
    if (bench) {
        const int iterations = 100;

        printf(
            "[Bench] Running %d iterations (%s)...\n",
            iterations,
            use_bc ? "bytecode VM" : "tree-walk VM"
        );

        vm_set_quiet(true);

        clock_t start = clock();

        for (int i = 0; i < iterations; i++) {
            if (use_bc) {
                bc_run_program(ast, true);
            } else {
                vm_reset();
                execute(ast);
            }
        }

        clock_t end = clock();

        double total_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;

        printf(
            "[Bench] %d iterations: total %.3f ms, avg %.4f ms/iter\n",
            iterations,
            total_ms,
            total_ms / iterations
        );

        vm_set_quiet(false);
    } else if (use_bc) {
        if (!raw) printf("\n--- Executing on Bytecode VM ---\n");
        bc_run_program(ast, false);
        if (!raw) printf("\n--- Simulation Finished ---\n");
    } else {
        if (!raw) printf("\n--- Executing Simulation ---\n");
        execute(ast);
        if (!raw) printf("\n--- Simulation Finished ---\n");
    }

    if (gc_stats) {
        printf(
            "[GC] bytes allocated: %zu, collections: %d\n",
            gc_bytes_allocated(),
            gc_collections()
        );
    }

    // Cleanup
    free_ast(ast);
    free(tokens);
    free(source);

    gc_free_all();

    return KINETRA_EXIT_OK;
}