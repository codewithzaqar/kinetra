#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

// Process exit codes
#define KINETRA_EXIT_OK 0
#define KINETRA_EXIT_LEX 1
#define KINETRA_EXIT_PARSE 2
#define KINETRA_EXIT_RUNTIME 3
#define KINETRA_EXIT_IO 4
#define KINETRA_EXIT_USAGE 64
#define KINETRA_EXIT_CODEGEN 5

typedef enum {
	DIAG_LEX,
	DIAG_PARSE,
	DIAG_RUNTIME,
	DIAG_CODEGEN
} DiagStage;

// Render the current source file for snippet rendering
void diag_set_source(const char* filename, const char* source);

// Report an error with snippet + caret, then exit with the stage's code
_Noreturn void diag_error(DiagStage stage, int line, int column, const char* message);

#endif