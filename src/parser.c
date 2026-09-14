#include "../include/kinetra.h"

static int current = 0;
static Token* current_tokens;
static int total_tokens;

static Token peek() { 
    return current_tokens[current]; 
}

static Token advance() { 
    return current_tokens[current++]; 
}

static ASTNode* create_node(ASTNodeType type, Token token) {
    ASTNode* node = malloc(sizeof(ASTNode));

    if (!node) {
        fprintf(stderr, "[Parser Error] Out of memory\n");
        exit(1);
    }

    node->type = type;
    node->token = token;

    node->left = NULL;
    node->right = NULL;

    node->statements = NULL;
    node->statement_count = 0;

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
            fprintf(
                stderr, 
                "[Parser Error] Expected ')' at line %d\n", 
                t.line
            );
            exit(1);
        }

        advance(); // consume ')'

        return expr;
    }

    fprintf(
        stderr, 
        "[Parser Error] Unexpected token '%s'\n", 
        t.lexeme,
        t.line
    );

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

        ASTNode* node = create_node(NODE_PRINT, print_token);
        node->left = expr;

        return node;
    }

    // let identifier = expression;
    if (peek().type == TOKEN_LET) {
        advance(); // consume let

        if (peek().type != TOKEN_IDENTIFIER) {
            fprintf(
                stderr,
                "[Parser Error] Expected variable name after 'let' at line %d\n",
                peek().line
            );
            exit(1);
        }

        Token id = advance();

        if (peek().type != TOKEN_ASSIGN) {
            fprintf(
                stderr,
                "[Parser Error] Expected '=' after variable name '%s' at line %d\n",
                id.lexeme,
                id.line
            );
            exit(1);
        }

        advance(); // consume '='

        ASTNode* expr = expression();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        ASTNode* node = create_node(NODE_LET, id);
        node->left = expr;

        return node;
    }

    // identifier = expression;
    if (
        peek().type == TOKEN_IDENTIFIER &&
        current + 1 < total_tokens &&
        current_tokens[current + 1].type == TOKEN_ASSIGN
    ) {
        Token id = advance();

        advance(); // consume '='

        ASTNode* expr = expression();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        ASTNode* node = create_node(NODE_ASSIGN, id);
        node->left = expr;

        return node;
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
    
    while (peek().type != TOKEN_EOF) {
        // Allow empty statements: ;;
        if (peek().type == TOKEN_SEMICOLON) {
            advance();
            continue;
        }

        ASTNode* stmt = statement();

        ASTNode** tmp = realloc(
            root->statements,
            sizeof(ASTNode*) * (root->statement_count + 1)
        );

        if (!tmp) {
            fprintf(stderr, "[Parser Error] Out of memory\n");
            exit(1);
        }

        root->statements = tmp;
        root->statements[root->statement_count++] = stmt;
    }

    return root;
}

void free_ast(ASTNode* node) {
    if (!node) return;

    if (node->type == NODE_PROGRAM && node->statements) {
        for (int i = 0; i < node->statement_count; i++) {
            free_ast(node->statements[i]);
        }

        free(node->statements);
    } else {
        free_ast(node->left);
        free_ast(node->right);
    }

    free(node);
}