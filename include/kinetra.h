#ifndef KINETRA_H
#define KINETRA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define KINETRA_VERSION "0.0.1a14"
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
	TOKEN_DT,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_TRUE,
	TOKEN_FALSE,

	TOKEN_LT,
	TOKEN_GT,
	TOKEN_LE,
	TOKEN_GE,
	TOKEN_EQ,
	TOKEN_NE,

	TOKEN_AND,
	TOKEN_OR,
	TOKEN_NOT,
	TOKEN_WHILE,
	TOKEN_BREAK,
	TOKEN_CONTINUE,
	TOKEN_FN,
	TOKEN_RETURN,

	TOKEN_VEC3,
	TOKEN_MAT4,
	TOKEN_PARTICLE,
	TOKEN_DOT,
	TOKEN_BUILTIN,

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
	TOKEN_COMMA,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,

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

	NODE_VEC3,
	NODE_MAT4,
	NODE_ARRAY_LITERAL,
	NODE_INDEX,
	NODE_INDEX_ASSIGN,
	NODE_PARTICLE,
	NODE_MEMBER,
	NODE_CALL,
	NODE_IF,
	NODE_BLOCK,
	NODE_UNARY_OP,
	NODE_BOOLEAN_LITERAL,
	NODE_WHILE,
	NODE_BREAK,
	NODE_CONTINUE,
	NODE_FUNCTION,
	NODE_RETURN,

	NODE_BINARY_OP,
	NODE_NUMBER_LITERAL,
	NODE_VARIABLE
} ASTNodeType;

typedef struct ASTNode {
	ASTNodeType type;
	Token token;

	struct ASTNode* left;
	struct ASTNode* right;

	// Used by NODE_VEC3 for the third component:
	// vec3(left, right, third)
	struct ASTNode* third;

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