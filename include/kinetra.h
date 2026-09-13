#ifndef KINETRA_H
#define KINETRA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define KINETRA_VERSION "0.0.1a02"
#define MAX_TOKENS 4096
#define MAX_AST_NODES 2048

// --- Token Definitions ---
typedef enum {
	TOKEN_EOF,
	TOKEN_NUMBER,
	TOKEN_IDENTIFIER,
	TOKEN_SIM_KEYWORD,
	TOKEN_MATH_KEYWORD,
	TOKEN_OP_ADD,
	TOKEN_OP_SUB,
	TOKEN_OP_MUL,
	TOKEN_OP_DIV,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_SEMICOLON,
	TOKEN_ERROR
} TokenType;

typedef struct {
	TokenType type;
	char lexeme[256];
	double value;
	int line;
} Token;

// --- AST Definitions ---
typedef enum {
	NODE_PROGRAM,
	NODE_SIMULATION_BLOCK,
	NODE_BINARY_OP,
	NODE_NUMBER_LITERAL,
	NODE_VARIABLE
} ASTNodeType;

typedef struct ASTNode {
	ASTNodeType type;
	Token token;
	struct ASTNode* left;
	struct ASTNode* right;
} ASTNode;

// --- Function Prototypes ---
// Lexer
Token* lex(const char* source, int* token_count);

// Parser
ASTNode* parse(Token* tokens, int token_count);
void free_ast(ASTNode* node);

// VM
void execute(ASTNode* ast);

#endif