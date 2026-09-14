#ifndef KINETRA_H
#define KINETRA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define KINETRA_VERSION "0.0.1a05"
#define MAX_TOKENS 4096
#define MAX_AST_NODES 2048

// --- Token Definitions ---
typedef enum {
	TOKEN_EOF,
	TOKEN_NUMBER,
	TOKEN_IDENTIFIER,

	TOKEN_LET,
	TOKEN_PRINT,

	TOKEN_SIM,
	TOKEN_STEP,

	TOKEN_SIM_KEYWORD,
	TOKEN_MATH_KEYWORD,
	
	TOKEN_ASSIGN,

	TOKEN_OP_ADD,
	TOKEN_OP_SUB,
	TOKEN_OP_MUL,
	TOKEN_OP_DIV,

	TOKEN_LPAREN,
	TOKEN_RPAREN,

	TOKEN_LBRACE,
	TOKEN_RBRACE,

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

	NODE_PRINT,
	NODE_LET,
	NODE_ASSIGN,

	NODE_STEP,
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

	// Used by NODE_PROGRAM and NODE_SIMULATION_BLOCK
	struct ASTNode** statements;
	int statement_count;
} ASTNode;

// --- Function Prototypes ---
Token* lex(const char* source, int* token_count);
ASTNode* parse(Token* tokens, int token_count);
void free_ast(ASTNode* node);
void execute(ASTNode* ast);

#endif