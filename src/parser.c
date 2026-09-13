#include "../include/kinetra.h"

static int current = 0;
static Token* current_tokens;
static int total_tokens;

static Token peek() { return current_tokens[current]; }
static Token advance() { return current_tokens[current++]; }

static ASTNode* create_node(ASTNodeType type, Token token) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->token = token;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Forward declaration for recursive descent
static ASTNode* expression();

static ASTNode* primary() {
    Token t = peek();
    if (t.type == TOKEN_NUMBER) {
        advance();
        return create_node(NODE_NUMBER_LITERAL, t);
    }
    if (t.type == TOKEN_IDENTIFIER) {
        advance();
        return create_node(NODE_VARIABLE, t);
    }
    if (t.type == TOKEN_LPAREN) {
        advance();
        ASTNode* expr = expression();
        if (peek().type != TOKEN_RPAREN) {
            fprintf(stderr, "[Parser Error] Expected ')' at line %d\n", t.line);
        }
        advance(); // consume ')'
        return expr;
    }
    fprintf(stderr, "[Parser Error] Unexpected token '%s'\n", t.lexeme);
    exit(1);
}

static ASTNode* term() {
    ASTNode* node = primary();
    while (peek().type == TOKEN_OP_MUL || peek().type == TOKEN_OP_DIV) {
        Token op = advance();
        ASTNode* bin_op = create_node(NODE_BINARY_OP, op);
        bin_op->left = node;
        bin_op->right = primary();
        node = bin_op;
    }
    return node;
}

static ASTNode* expression() {
    ASTNode* node = term();
    while (peek().type == TOKEN_OP_ADD || peek().type == TOKEN_OP_SUB) {
        Token op = advance();
        ASTNode* bin_op = create_node(NODE_BINARY_OP, op);
        bin_op->left = node;
        bin_op->right = term();
        node = bin_op;
    }
    return node;
}

static ASTNode* statement() {
    // print expression;
    if (peek().type == TOKEN_PRINT) {
        Token print_token = advance();

        ASTNode* expr = expression();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        ASTNode* print_node = create_node(NODE_PRINT, print_token);
        print_node->left = expr;

        return print_node;
    }

    // expression;
    ASTNode* expr = expression();

    if (peek().type == TOKEN_SEMICOLON) {
        advance();
    }

    return expr;
}

ASTNode* parse(Token* tokens, int token_count) {
    current_tokens = tokens;
    total_tokens = token_count;
    current = 0;

    ASTNode* root = create_node(NODE_PROGRAM, tokens[0]);
    
    // For v0.0.1a03, we just parse a single mathematical expression/sim step
    // expression;
    // print expression;
    if (peek().type != TOKEN_EOF) {
        root->left = statement();
    }

    return root;
}

void free_ast(ASTNode* node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}