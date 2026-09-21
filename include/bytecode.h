#ifndef BYTECODE_H
#define BYTECODE_H

#include "kinetra.h"

// Prototype bytecode backend (v0.0.1a18)
// Compiles the scalar-core subset of the AST and executes it on a
// Stack machine. Unsupported constructs raise DIAG_CODEGEN errors
// (exit code 5)
void bc_run_program(ASTNode* ast, bool quiet);

#endif