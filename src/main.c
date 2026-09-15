#include "../include/kinetra.h"
#include "../include/hpc_math.h"

void print_usage() {
    printf("Kinetra Language v%s\n", KINETRA_VERSION);
    printf("Usage: kinetra <source_file.knt>\n");
    printf("Flags:\n");
    printf("  --hpc      Enable hardware acceleration flags\n");
    printf("  --version  Print version\n");
}

char* read_file(const char* filename) {
    // IMPORTANT: binary mode. Text mode translates CRLF -> LF on Windows,
    // which makes fread() return fewer bytes than ftell() reports.
    FILE* file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "[Kinetra Error] Could not open file: %s\n", filename);
        exit(1);
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "[Kinetra Error] Could not seek in file: %s\n", filename);
        fclose(file);
        exit(1);
    }

    long length = ftell(file);

    if (length < 0) {
        fprintf(stderr, "[Kinetra Error] Could not determine file size: %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "[Kinetra Error] Could not rewind file: %s\n", filename);
        fclose(file);
        exit(1);
    }

    char* buffer = malloc((size_t)length + 1);

    if (!buffer) {
        fprintf(stderr, "[Kinetra Error] Out of memory while reading: %s\n", filename);
        fclose(file);
        exit(1);
    }

    // Use the ACTUAL number of bytes read, not the ftell() length.
    size_t read_len = fread(buffer, 1, (size_t)length, file);

    if (ferror(file)) {
        fprintf(stderr, "[Kinetra Error] Failed while reading file: %s\n", filename);
        free(buffer);
        fclose(file);
        exit(1);
    }

    // Terminate exactly after the bytes we actually read.
    buffer[read_len] = '\0';

    fclose(file);

    return buffer;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("Kinetra v%s\n", KINETRA_VERSION);
        return 0;
    }

    printf("--- Kinetra Compiler/Runtime v%s ---\n", KINETRA_VERSION);

    // Initialize HPC/Simulation subsystems
    init_hpc_subsystem();

    // 1. Read Source Code
    const char* filename = argv[1];
    char* source = read_file(filename);
    printf("[1/3] Source loaded: %s\n", filename);

    // 2. Lexical Analysis
    int token_count = 0;
    Token* tokens = lex(source, &token_count);
    printf("[2/3] Lexing complete. Generated %d tokens.\n", token_count);

    // 3. Parsing
    ASTNode* ast = parse(tokens, token_count);
    printf("[3/3] Parsing complete. AST generated.\n");

    // 4. Execution (VM)
    printf("\n--- Executing Simulation ---\n");
    execute(ast);
    printf("\n--- Simulation Finished ---\n");

    // Cleanup
    free_ast(ast);
    free(tokens);
    free(source);

    return 0;
}