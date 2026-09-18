#include "../include/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* g_filename = "<unknown>";
static const char* g_source = NULL;

void diag_set_source(const char* filename, const char* source) {
	g_filename = filename ? filename : "<unknown>";
	g_source = source;
}

static const char* stage_name(DiagStage stage) {
	switch (stage) {
		case DIAG_LEX:     return "lex";
		case DIAG_PARSE:   return "parse";
		case DIAG_RUNTIME: return "runtime";
		default:  return "unknown";
	}
}

static int exit_code_for(DiagStage stage) {
	switch (stage) {
		case DIAG_LEX:  return KINETRA_EXIT_LEX;
		case DIAG_PARSE: return KINETRA_EXIT_PARSE;
		case DIAG_RUNTIME: return KINETRA_EXIT_RUNTIME;
		default:  return KINETRA_EXIT_RUNTIME;
	}
}

static void print_snippet(int line, int column) {
	if (!g_source || line <= 0) {
		return;
	}

	// Locate the start of the requested line
	const char* p = g_source;
	int current = 1;

	while (current < line && *p) {
		if (*p == '\n') {
			current++;
		}
		p++;
	}

	if (current != line) {
		return; // line beyond end of line
	}

	// Copy the line text (without newline)
	char buf[1024];
	int n = 0;

	while (*p && *p != '\n' && *p != '\r' && n < 1023) {
		buf[n++] = *p++;
	}

	buf[n] = '\0';

	fprintf(stderr, "  %4d | %s\n", line, buf);

	// Caret line (only when the column is known)
	if (column > 0 && column <= n) {
		fprintf(stderr, "   |");

		for (int i = 1; i < column; i++) {
			fputc(0x20, stderr);
		}

		fprintf(stderr, "^\n");
	}
}

_Noreturn void diag_error(DiagStage stage, int line, int column, const char* message) {
	fprintf(stderr, "[Kinetra Error] (%s) %s\n", stage_name(stage), message);

	fprintf(stderr, " --> %s:%d", g_filename, line);

	if (column > 0) {
		fprintf(stderr, ":%d", column);
	}

	fprintf(stderr, "\n");

	print_snippet(line, column);

	exit(exit_code_for(stage));
}