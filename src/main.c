#include "../include/kinetra.h"
#include "../include/hpc_math.h"

void print_usage() {
	printf("Kinetra Language v%s\n", KINETRA_VERSION);
	printf("Usgae: kinetra <source_file.knt>\n");
	printf("Flags:\n");
	printf(" --hpc     Enable hardware acceleration flags\n");
	printf(" --version Print version\n");
}

char* read_file(const char* filename) {
	FILE* file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "[Kinetra Error] Could not open file: %s\n", filename);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = malloc(length + 1);
	fread(buffer, 1, length, file);
	buffer[length] = '\0';
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

	// Read Source Code
	const char* filename = argv[1];
	char* source = read_file(filename);
	printf("[1/3] Source loaded: %s\n", filename);

	// Lexical Analysis
	int token_count = 0;
	Token* tokens = lex(source, &token_count);
	printf("[2/3] Lexing complete, Generated %d tokens. \n", token_count);

	// Parsing
	ASTNode* ast = parse(tokens, token_count);
	printf("[3/3] Parsing complete. AST generated.\n");

	// Execution (VM)
	printf("\n--- Executing Simulation ---\n");
	execute(ast);
	printf("\n--- Simulation Finished ---\n");

	// Cleanup
	free_ast(ast);
	free(tokens);
	free(source);

	return 0;
}