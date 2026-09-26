#include "../include/kinetra.h"
#include "../include/diagnostics.h"

static int current = 0;
static Token* current_tokens;
static int total_tokens;

static Token peek() { 
    return current_tokens[current]; 
}

static Token advance() { 
    return current_tokens[current++]; 
}

static Token expect(TokenType type, const char* message) {
    if (peek().type != type) {
        diag_error(DIAG_PARSE, peek().line, peek().column, message);
    }

    return advance();
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
    node->third = NULL;

    node->statements = NULL;
    node->statement_count = 0;

    return node;
}

static void add_statement(ASTNode* node, ASTNode* stmt) {
    ASTNode** tmp = realloc(
        node->statements,
        sizeof(ASTNode*) * (node->statement_count + 1)
    );

    if (!tmp) {
        fprintf(stderr, "[Parser Error] Out of memory\n");
        exit(1);
    }

    node->statements = tmp;
    node->statements[node->statement_count++] = stmt;
}

static int fold_count = 0;

int parser_fold_count(void) {
    return fold_count;
}

static ASTNode* fold_binary(ASTNode* node) {
    if (node->type != NODE_BINARY_OP) return node;
    if (!node->left || !node->right) return node;

    // Logical ops on boolean literals
    if (node->token.type == TOKEN_AND || node->token.type == TOKEN_OR) {
        if (
            node->left->type == NODE_BOOLEAN_LITERAL &&
            node->right->type == NODE_BOOLEAN_LITERAL
        ) {
            bool l = node->left->token.type == TOKEN_TRUE;
            bool r = node->right->token.type == TOKEN_TRUE;
            bool res = (node->token.type == TOKEN_AND) ? (l && r) : (l || r);

            Token t = node->token;
            t.type = res ? TOKEN_TRUE : TOKEN_FALSE;
            snprintf(t.lexeme, sizeof(t.lexeme), "%s", res ? "true" : "false");

            ASTNode* lit = create_node(NODE_BOOLEAN_LITERAL, t);
            free_ast(node);
            fold_count++;
            return lit;
        }

        return node;
    }

    // Arithmetic / comparison on number literals
    if (
        node->left->type != NODE_NUMBER_LITERAL ||
        node->right->type != NODE_NUMBER_LITERAL
    ) {
        return node;
    }

    double l = node->left->token.value;
    double r = node->right->token.value;
    double result = 0.0;
    bool is_comparison = false;
    bool bool_result = false;

    switch (node->token.type) {
        case TOKEN_OP_ADD: result = l + r; break;
        case TOKEN_OP_SUB: result = l - r; break;
        case TOKEN_OP_MUL: result = l * r; break;

        case TOKEN_OP_DIV:
            // Never fold division by zero keep the runtime error
            if (r == 0.0) return node;
            result = l/r;
            break;

        case TOKEN_LT: is_comparison = true; bool_result = l < r; break;
        case TOKEN_GT: is_comparison = true; bool_result = l > r; break;
        case TOKEN_LE: is_comparison = true; bool_result = l <= r; break;
        case TOKEN_GE: is_comparison = true; bool_result = l >= r; break;
        case TOKEN_EQ: is_comparison = true; bool_result = l == r; break;
        case TOKEN_NE: is_comparison = true; bool_result = l != r; break;

        default:
            return node;
    }

    Token t = node->token;
    ASTNode* lit;

    if (is_comparison) {
        t.type = bool_result ? TOKEN_TRUE : TOKEN_FALSE;
        snprintf(t.lexeme, sizeof(t.lexeme), "%s", bool_result ? "true" : "false");
        lit = create_node(NODE_BOOLEAN_LITERAL, t);
    } else {
        t.type = TOKEN_NUMBER;
        t.value = result;
        snprintf(t.lexeme, sizeof(t.lexeme), "%g", result);
        lit = create_node(NODE_NUMBER_LITERAL, t);
    }

    free_ast(node);
    fold_count++;
    return lit;
}

static ASTNode* fold_unary(ASTNode* node) {
    if (node->type != NODE_UNARY_OP || node->token.type != TOKEN_NOT) return node;
    if (!node->left || node->left->type != NODE_BOOLEAN_LITERAL) return node;

    bool l = node->left->token.type == TOKEN_TRUE;

    Token t = node->token;
    t.type = l ? TOKEN_FALSE : TOKEN_TRUE;
    snprintf(t.lexeme, sizeof(t.lexeme), "%s", l ? "false" : "true");

    ASTNode* lit = create_node(NODE_BOOLEAN_LITERAL, t);
    free_ast(node);
    fold_count++;
    return lit;
}

// Forward declaration
static ASTNode* expression();
static ASTNode* statement();
static void parse_block_into(ASTNode* node);
static ASTNode* parse_block(void);

static ASTNode* primary() {
    Token t = peek();

    if (t.type == TOKEN_NUMBER) {
        advance();
        return create_node(NODE_NUMBER_LITERAL, t);
    }

    if(t.type == TOKEN_TRUE || t.type == TOKEN_FALSE) {
        advance();
        return create_node(NODE_BOOLEAN_LITERAL, t);
    }

    if (t.type == TOKEN_IDENTIFIER) {
        // User-defined function call: name(args)
        if (
            current + 1 < total_tokens &&
            current_tokens[current + 1].type == TOKEN_LPAREN
        ) {
            Token name_token = advance();

            advance(); // consume '('

            ASTNode* node = create_node(NODE_CALL, name_token);

            if (peek().type != TOKEN_RPAREN) {
                for (;;) {
                    ASTNode* arg = expression();
                    add_statement(node, arg);

                    if (peek().type == TOKEN_COMMA) {
                        advance();
                        continue;
                    } 

                    break;
                }
            }

            expect(TOKEN_RPAREN, "Expected ')' after function arguments");

            return node;
        }

        advance();
        return create_node(NODE_VARIABLE, t);
    }

    // Renderer the built-in dt variable
    if (t.type == TOKEN_DT) {
        advance();
        return create_node(NODE_VARIABLE, t);
    }

    // vec3(expression, expression, expression)
    if (t.type == TOKEN_VEC3) {
        Token vec_token = advance();

        expect(TOKEN_LPAREN, "Expected '(' after 'vec3'");

        ASTNode* x = expression();

        expect(TOKEN_COMMA, "Expected ',' in vec3 constructor");

        ASTNode* y = expression();

        expect(TOKEN_COMMA, "Expected ',' in vec3 constructor");

        ASTNode* z = expression();

        expect(TOKEN_RPAREN, "Expected ')' after vec3 constructor");

        ASTNode* node = create_node(NODE_VEC3, vec_token);
        node->left = x;
        node->right = y;
        node->third = z;

        return node;
    }

    // Built-in function call: name(arg, arg, ...)
    if (t.type == TOKEN_BUILTIN) {
        Token fn_token = advance();

        expect(TOKEN_LPAREN, "Expected '(' after built-in function name");

        ASTNode* node = create_node(NODE_CALL, fn_token);

        // Parse argument list
        if (peek().type != TOKEN_RPAREN) {
            for (;;) {
                ASTNode* arg = expression();
                add_statement(node, arg);

                if (peek().type == TOKEN_COMMA) {
                    advance();
                    continue;
                }

                break;
            }
        }

        expect(TOKEN_RPAREN, "Expected ')' after built-in function arguments");

        // Parse-time arity checking
        int arity = node->statement_count;
        const char* name = fn_token.lexeme;

        if (
            strcmp(name, "dot") == 0 || 
            strcmp(name, "cross") == 0 ||
            strcmp(name, "min") == 0 ||
            strcmp(name, "max") == 0 ||
            strcmp(name, "rotate") == 0 ||
            strcmp(name, "transform") == 0 ||
            strcmp(name, "apply_force") == 0 ||
            strcmp(name, "integrate") == 0 ||
            strcmp(name, "pow") == 0 ||
            strcmp(name, "reflect") == 0
        ) {
            if (arity != 2) {
                fprintf(
                    stderr,
                    "[Parser Error] '%s' expects 2 arguments, got %d at line %d\n",
                    name,
                    arity,
                    fn_token.line
                );
                exit(1);
            }
        } else if (
            strcmp(name, "translate") == 0 ||
            strcmp(name, "scale") == 0 ||
            strcmp(name, "clamp") == 0 ||
            strcmp(name, "lerp") == 0
        ) {
            if (arity != 3) {
                fprintf(
                    stderr,
                    "[Parser Error] '%s' expects 3 arguments, got %d at line %d\n",
                    name,
                    arity,
                    fn_token.line
                );
                exit(1);
            }
        } else if (
            strcmp(name, "length") == 0 || 
            strcmp(name, "normalize") == 0 ||
            strcmp(name, "sqrt") == 0 ||
            strcmp(name, "abs") == 0 ||
            strcmp(name, "sin") == 0 ||
            strcmp(name, "cos") == 0 ||
            strcmp(name, "tan") == 0 ||
            strcmp(name, "clear_force") == 0 ||
            strcmp(name, "exp") == 0 ||
            strcmp(name, "log") == 0 ||
            strcmp(name, "floor") == 0 ||
            strcmp(name, "ceil") == 0 ||
            strcmp(name, "round") == 0
        ) {
            if (arity != 1) {
                fprintf(
                    stderr,
                    "[Parser Error] '%s' expects 1 argument, got %d at line %d\n",
                    name,
                    arity,
                    fn_token.line
                );
                exit(1);
            }
        }

        return node;
    }

    // mat4() or mat4(m0 .. m15)
    if (t.type == TOKEN_MAT4) {
        Token mat_token = advance();

        expect(TOKEN_LPAREN, "Expected '(' after 'mat4'");

        ASTNode* node = create_node(NODE_MAT4, mat_token);

        if (peek().type != TOKEN_RPAREN) {
            for (;;) {
                ASTNode* arg = expression();
                add_statement(node, arg);

                if (peek().type == TOKEN_COMMA) {
                    advance();
                    continue;
                }

                break;
            }
        }

        expect(TOKEN_RPAREN, "Expected ')' after mat4 constructor");

        if (node->statement_count != 0 && node->statement_count != 16) {
            fprintf(
                stderr,
                "[Parser Error] mat4() expects 0 or 16 arguments, got %d at line %d\n",
                node->statement_count,
                mat_token.line
            );
            exit(1);
        }

        return node;
    }

    // [elem, elem, ...]
    if (t.type == TOKEN_LBRACKET) {
        Token bracket = advance();

        ASTNode* node = create_node(NODE_ARRAY_LITERAL, bracket);

        if (peek().type != TOKEN_RBRACKET) {
            for (;;) {
                ASTNode* elem = expression();
                add_statement(node, elem);

                if (peek().type == TOKEN_COMMA) {
                    advance();
                    continue;
                }

                break;
            }
        }

        expect(TOKEN_RBRACKET, "Expected ']' after array literal");

        return node;
    }

    // particle(pos, vel) or particle(pos, vel, mass)
    if (t.type == TOKEN_PARTICLE) {
        Token part_token = advance();

        expect(TOKEN_LPAREN, "Expected '(' after 'particle'");

        ASTNode* node = create_node(NODE_PARTICLE, part_token);

        for (;;) {
            ASTNode* arg = expression();
            add_statement(node, arg);

            if (peek().type == TOKEN_COMMA) {
                advance();
                continue;
            }

            break;
        }

        expect(TOKEN_RPAREN, "Expected ')' after particle constructor");

        if (node->statement_count != 2 && node->statement_count != 3) {
            fprintf(
                stderr,
                "[Parser Error] particle() expects 2 or 3 arguments, got %d at line %d\n",
                node->statement_count,
                part_token.line
            );
            exit(1);
        }

        return node;
    }

    if (t.type == TOKEN_STRING) {
        advance();
        return create_node(NODE_STRING_LITERAL, t);
    }

    if (t.type == TOKEN_LPAREN) {
        advance();

        ASTNode* expr = expression();

        expect(TOKEN_RPAREN, "Expected ')'");

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

static ASTNode* unary() {
    if (peek().type == TOKEN_NOT) {
        Token op = advance();

        ASTNode* node = create_node(NODE_UNARY_OP, op);
        node->left = unary();

        return fold_unary(node);
    }

    ASTNode* node = primary();

    // Postfix index chains: a[i], m[1][0] f()[2]
    for (;;) {
        if (peek().type == TOKEN_LBRACKET) {
            Token bracket = advance();

            ASTNode* start = NULL;
            ASTNode* end = NULL;
            bool is_slice = false;

            // Optional start bound
            if (peek().type != TOKEN_COLON) {
                start = expression();
            }

            // Colon turns indexing into slicing
            if (peek().type == TOKEN_COLON) {
                is_slice = true;
                advance();

                // Optional end bound
                if (peek().type != TOKEN_RBRACKET) {
                    end = expression();
                }
            }

            expect(TOKEN_RBRACKET, "Expected ']' after index or slice");

            if (is_slice) {
                ASTNode* slice_node = create_node(NODE_SLICE, bracket);
                slice_node->left = node;
                slice_node->right = start;
                slice_node->third = end;
                node = slice_node;
            } else {
                ASTNode* index_node = create_node(NODE_INDEX, bracket);
                index_node->left = node;
                index_node->right = start;
                node = index_node;
            }

            continue;
        }

        if (peek().type == TOKEN_DOT) {
            advance(); // consume '.'

            if (peek().type != TOKEN_IDENTIFIER) {
                fprintf(
                    stderr,
                    "[Parser Error] Expected name after '.' at line %d",
                    peek().line
                );
                exit(1);
            }

            Token member = advance();

            ASTNode* member_node = create_node(NODE_MEMBER, member);
            member_node->left = node;

            node = member_node;
            continue;
        }

        break;
    }

    return node;
}

static ASTNode* term() {
    ASTNode* node = unary();

    while (peek().type == TOKEN_OP_MUL || peek().type == TOKEN_OP_DIV) {
        Token op = advance();

        ASTNode* bin_op = create_node(NODE_BINARY_OP, op);
        bin_op->left = node;
        bin_op->right = unary();

        node = fold_unary(bin_op);
    }

    return node;
}

static ASTNode* additive() {
    ASTNode* node = term();

    while (peek().type == TOKEN_OP_ADD || peek().type == TOKEN_OP_SUB) {
        Token op = advance();

        ASTNode* bin_op = create_node(NODE_BINARY_OP, op);
        bin_op->left = node;
        bin_op->right = term();

        node = fold_unary(bin_op);
    }

    return node;
}

static bool is_comparison_token(TokenType type) {
    return
        type == TOKEN_LT ||
        type == TOKEN_GT ||
        type == TOKEN_LE ||
        type == TOKEN_GE ||
        type == TOKEN_EQ ||
        type == TOKEN_NE;
}

static ASTNode* comparison() {
    ASTNode* node = additive();

    while (is_comparison_token(peek().type)) {
        Token op = advance();

            ASTNode* bin_op = create_node(NODE_BINARY_OP, op);
            bin_op->left = node;
            bin_op->right = additive();

            node = fold_unary(bin_op);
    }

    return node;
}

static ASTNode* and_expr() {
    ASTNode* node = comparison();

    while (peek().type == TOKEN_AND) {
        Token op = advance();

        ASTNode* bin_op = create_node(NODE_BINARY_OP, op);
        bin_op->left = node;
        bin_op->right = comparison();

        node = fold_unary(bin_op);
    }

    return node;
}
    
// Entry point: logical OR
static ASTNode* expression() {
    ASTNode* node = and_expr();

    while (peek().type == TOKEN_OR) {
        Token op = advance();

        ASTNode* bin_op = create_node(NODE_BINARY_OP, op);
        bin_op->left = node;
        bin_op->right = and_expr();

        node = fold_binary(bin_op);
    }

    return node;
}

static void parse_block_into(ASTNode* node) {
    expect(TOKEN_LBRACE, "Expected '{'");

    while (peek().type != TOKEN_RBRACE && peek().type != TOKEN_EOF) {
        // Allow empty statements
        if (peek().type == TOKEN_SEMICOLON) {
            advance();
            continue;
        }

        ASTNode* stmt = statement();
        add_statement(node, stmt);
    }

    expect(TOKEN_RBRACE, "Expected '}'");
}

static ASTNode* parse_block(void) {
    ASTNode* block = create_node(NODE_BLOCK, peek());
    parse_block_into(block);
    return block;
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

    // const identifier = expression;
    if (peek().type == TOKEN_CONST) {
        advance(); // consume const

        if (peek().type != TOKEN_IDENTIFIER) {
            diag_error(
                DIAG_PARSE,
                peek().line,
                peek().column,
                "Expected variable name after 'const'"
            );
        }

        Token id = advance();

        expect(TOKEN_ASSIGN, "Expected '=' after constant name");

        ASTNode* expr = expression();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        ASTNode* node = create_node(NODE_CONST, id);
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

        expect(TOKEN_ASSIGN, "Expected '=' after variable name");

        ASTNode* expr = expression();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        ASTNode* node = create_node(NODE_LET, id);
        node->left = expr;

        return node;
    }

    // step expression;
    if (peek().type == TOKEN_STEP) {
        Token step_token = advance();

        ASTNode* expr = expression();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        ASTNode* node = create_node(NODE_STEP, step_token);
        node->left = expr;

        return node;
    }

    // sim [count] [dt value] {...}
    if (peek().type == TOKEN_SIM) {
        Token sim_token = advance();

        ASTNode* count_expr = NULL;
        ASTNode* dt_expr = NULL;

        // Optional step count: sim 100 ...
        if (peek().type != TOKEN_LBRACE && peek().type != TOKEN_DT) {
            count_expr = expression();
        }

        // Optional custom time step: sim 100 dt 0.016 {...}
        if (peek().type == TOKEN_DT) {
            advance(); // consume 'dt'
            dt_expr = expression();
        }

        ASTNode* node = create_node(NODE_SIMULATION_BLOCK, sim_token);
        node->left = count_expr;
        node->right = dt_expr;

        parse_block_into(node);

        return node;
    }

    // if condition {...} else {...}
    if (peek().type == TOKEN_IF) {
        Token if_token = advance();

        ASTNode* cond = expression();

        ASTNode* then_block = parse_block();

        ASTNode* else_block = NULL;

        if (peek().type == TOKEN_ELSE) {
            advance();

            // else if chaining
            if (peek().type == TOKEN_IF) {
                ASTNode* nested_if = statement();
                else_block = create_node(NODE_BLOCK, nested_if->token);
                add_statement(else_block, nested_if);
            } else {
                else_block = parse_block();
            }
        }

        ASTNode* node = create_node(NODE_IF, if_token);
        node->left = cond;
        node->right = then_block;
        node->third = else_block;

        return node;
    }

    // while condition {...}
    if (peek().type == TOKEN_WHILE) {
        Token while_token = advance();

        ASTNode* cond = expression();

        ASTNode* body = parse_block();

        ASTNode* node = create_node(NODE_WHILE, while_token);
        node->left = cond;
        node->right = body;

        return node;
    }

    // break;
    if (peek().type == TOKEN_BREAK) {
        Token break_token = advance();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        return create_node(NODE_BREAK, break_token);
    }

    // continue;
    if (peek().type == TOKEN_CONTINUE) {
        Token continue_token = advance();

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        return create_node(NODE_CONTINUE, continue_token);
    }

    // fn name(params) {...}
    if (peek().type == TOKEN_FN) {
        advance(); // consume fn

        if (peek().type != TOKEN_IDENTIFIER) {
            fprintf(
                stderr,
                "[Parser Error] Expected function name after 'fn' at line %d\n",
                peek().line
            );
            exit(1);
        }

        Token name_token = advance();

        expect(TOKEN_LPAREN, "Expected '(' after function name");

        ASTNode* node = create_node(NODE_FUNCTION, name_token);

        // Parameter list
        if (peek().type != TOKEN_RPAREN) {
            for (;;) {
                if (peek().type != TOKEN_IDENTIFIER) {
                    fprintf(
                        stderr,
                        "[Parser Error] Expected parameter name at line %d\n",
                        peek().line
                    );
                    exit(1);
                }

                Token param = advance();
                add_statement(node, create_node(NODE_VARIABLE, param));

                if (peek().type == TOKEN_COMMA) {
                    advance();
                    continue;
                }

                break;
            }
        }

        expect(TOKEN_RPAREN, "Expected ')' after parameter list");

        node->left = parse_block();

        return node;
    }

    // return [expression];
    if (peek().type == TOKEN_RETURN) {
        Token return_token = advance();

        ASTNode* expr = NULL;

        if (
            peek().type != TOKEN_SEMICOLON &&
            peek().type != TOKEN_RBRACE &&
            peek().type != TOKEN_EOF
        ) {
            expr = expression();
        }

        if (peek().type == TOKEN_SEMICOLON) {
            advance();
        }

        ASTNode* node = create_node(NODE_RETURN, return_token);
        node->left = expr;

        return node;
    }

    // parallel for <var> in <count> {...}
    if (peek().type == TOKEN_PARALLEL) {
        Token par_token = advance();

        if (peek().type != TOKEN_FOR) {
            diag_error(
                DIAG_PARSE,
                peek().line,
                peek().column,
                "Expected 'for' after 'parallel'"
            );
        }

        advance(); // consume for

        if (peek().type != TOKEN_IDENTIFIER) {
            diag_error(
                DIAG_PARSE,
                peek().line,
                peek().column,
                "Expected loop variable name after 'parallel for'"
            );
        }

        Token id = advance();

        if (peek().type != TOKEN_IN) {
            diag_error(
                DIAG_PARSE,
                peek().line,
                peek().column,
                "Expected 'in' after parallel loop variable"
            );
        }

        advance(); // consume in

        ASTNode* count = expression();

        ASTNode* body = parse_block();

        ASTNode* node = create_node(NODE_PARALLEL_FOR, id);
        node->left = count;
        node->right = body;

        (void)par_token;

        return node;
    }

    // identifier = expression;
    // identifier[index] = expression;
    if (
        peek().type == TOKEN_IDENTIFIER &&
        current + 1 < total_tokens &&
        (
            current_tokens[current + 1].type == TOKEN_ASSIGN ||
            current_tokens[current + 1].type == TOKEN_LBRACKET
        )
    ) {
        Token id = advance();

        // Indexed assignment: name[i] = value;
        if (peek().type == TOKEN_LBRACKET) {
            advance(); // consume '['

            ASTNode* idx = expression();

            expect(TOKEN_RBRACKET, "Expected ']' after index");
            expect(TOKEN_ASSIGN, "Expected '=' after indexed target");

            ASTNode* value = expression();

            if (peek().type == TOKEN_SEMICOLON) {
                advance();
            }

            ASTNode* node = create_node(NODE_INDEX_ASSIGN, id);
            node->left = idx;
            node->right = value;

            return node;
        } 

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
        add_statement(root, stmt);
    }

    return root;
}

const char* ast_node_name(ASTNodeType type) {
    switch (type) {
        case NODE_PROGRAM: return "PROGRAM";
        case NODE_PRINT: return "PRINT";
        case NODE_LET: return "LET";
        case NODE_ASSIGN: return "ASSIGN";
        case NODE_STEP: return "STEP";
        case NODE_SIMULATION_BLOCK: return "SIM";
        case NODE_VEC3: return "VEC3";
        case NODE_MAT4: return "MAT4";
        case NODE_PARTICLE: return "PARTICLE";
        case NODE_ARRAY_LITERAL: return "ARRAY";
        case NODE_INDEX: return "INDEX";
        case NODE_INDEX_ASSIGN: return "INDEX_ASSIGN";
        case NODE_MEMBER: return "MEMBER";
        case NODE_CALL: return "CALL";
        case NODE_IF: return "IF";
        case NODE_BLOCK: return "BLOCK";
        case NODE_WHILE: return "WHILE";
        case NODE_BREAK: return "BREAK";
        case NODE_CONTINUE: return "CONTINUE";
        case NODE_FUNCTION: return "FN";
        case NODE_RETURN: return "RETURN";
        case NODE_BINARY_OP: return "BINARY";
        case NODE_UNARY_OP: return "UNARY";
        case NODE_NUMBER_LITERAL: return "NUMBER";
        case NODE_BOOLEAN_LITERAL: return "BOOLEAN";
        case NODE_VARIABLE: return "VARIABLE";
        case NODE_SLICE: return "SLICE";
        default: return "UNKNOWN";
    }
}

static bool ast_node_owns_list(ASTNodeType type) {
    return
        type == NODE_PROGRAM ||
        type == NODE_SIMULATION_BLOCK ||
        type == NODE_CALL ||
        type == NODE_BLOCK ||
        type == NODE_FUNCTION ||
        type == NODE_MAT4 ||
        type == NODE_ARRAY_LITERAL ||
        type == NODE_PARTICLE;
}

static void ast_dump_node(ASTNode* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) {
        printf(" ");
    }

    printf("%s", ast_node_name(node->type));

    if (node->token.lexeme[0]) {
        printf("'%s'", node->token.lexeme);
    }

    if (node->type == NODE_NUMBER_LITERAL) {
        printf("value=%g", node->token.value);
    }

    printf("(%d:%d)\n", node->token.line, node->token.column);

    if (ast_node_owns_list(node->type)) {
        for (int i = 0; i < node->statement_count; i++) {
            ast_dump_node(node->statements[i], depth + 1);
        }
    }

    ast_dump_node(node->left, depth + 1);
    ast_dump_node(node->right, depth + 1);
    ast_dump_node(node->third, depth + 1);
}

void ast_dump(ASTNode* node) {
    ast_dump_node(node, 0);
}

void free_ast(ASTNode* node) {
    if (!node) return;

    // Nodes that own statement lists
    if (
        node->type == NODE_PROGRAM || 
        node->type == NODE_SIMULATION_BLOCK ||
        node->type == NODE_CALL ||
        node->type == NODE_BLOCK ||
        node->type == NODE_FUNCTION ||
        node->type == NODE_MAT4 ||
        node->type == NODE_ARRAY_LITERAL ||
        node->type == NODE_PARTICLE
    ) {
        if (node->statements) {
            for (int i = 0; i < node->statement_count; i++) {
                free_ast(node->statements[i]);
            }

            free(node->statements);
        }

        free_ast(node->left);
        free_ast(node->right);
        free_ast(node->third);
        free(node);

        return;
    }

    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->third);
    free(node);
}