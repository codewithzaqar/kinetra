#include "../include/kinetra.h"
#include "../include/hpc_math.h"
#include "../include/diagnostics.h"
#include <time.h>

void print_usage() {
    printf("Kinetra Language v%s\n", KINETRA_VERSION);
    printf("Usage: kinetra [flags] <source_file.knt>\n");
    printf("Flags:\n");
    printf("  --bench    Run 100 silent iterations and report timing\n");
    printf("  --folds    Report how many constant folds the parser performed\n");
    printf("  --hpc      Enable hardware acceleration flags\n");
    printf("  --version  Print version\n");
}

char* read_file(const char* path) {
    // IMPORTANT: binary mode. Text mode translates CRLF -> LF on Windows,
    // which makes fread() return fewer bytes than ftell() reports.
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

    // Use the ACTUAL number of bytes read, not the ftell() length.
    size_t read_len = fread(buffer, 1, (size_t)length, file);

    if (ferror(file)) {
        fprintf(stderr, "[Kinetra Error] Failed while reading file: %s\n", path);
        free(buffer);
        fclose(file);
        exit(KINETRA_EXIT_IO);
    }

    // Terminate exactly after the bytes we actually read.
    buffer[read_len] = '\0';

    fclose(file);

    return buffer;
}

int main(int argc, char** argv) {
    bool bench = false;
    bool show_folds = false;
    const char* filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("Kinetra v%s\n", KINETRA_VERSION);
            return KINETRA_EXIT_OK;
        } else if (strcmp(argv[i], "--bench") == 0) {
            bench = true;
        } else if (strcmp(argv[i], "--folds") == 0) {
            show_folds = true;
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        print_usage();
        return KINETRA_EXIT_USAGE;
    }

    printf("--- Kinetra Compiler/Runtime v%s ---\n", KINETRA_VERSION);

    // Initialize HPC/Simulation subsystems
    init_hpc_subsystem();

    // 1. Read Source Code
    char* source = read_file(filename);
    diag_set_source(filename, source);
    printf("[1/3] Source loaded: %s\n", filename);

    // 2. Lexical Analysis
    int token_count = 0;
    Token* tokens = lex(source, &token_count);
    printf("[2/3] Lexing complete. Generated %d tokens.\n", token_count);

    // 3. Parsing
    ASTNode* ast = parse(tokens, token_count);
    printf("[3/3] Parsing complete. AST generated.\n");

    if (show_folds) {
        printf("[Parser] Constant folds: %d\n", parser_fold_count());
    }

    // 4. Execution (VM)
    if (bench) {
        const int iterations = 100;

        vm_set_quiet(true);
        printf("[Bench] Runtime %d iterations...\n", iterations);

        clock_t start = clock();

        for (int i = 0; i < iterations; i++) {
            vm_reset();
            execute(ast);
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
    } else {
        printf("\n--- Executing Simulation ---\n");
        execute(ast);
        printf("\n--- Simulation Finished ---\n");
    }


    // Cleanup
    free_ast(ast);
    free(tokens);
    free(source);

    return KINETRA_EXIT_OK;
}