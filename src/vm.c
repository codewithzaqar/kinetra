#include "../include/kinetra.h"
#include "../include/hpc_math.h"

#define MAX_VARIABLES 1024

typedef struct {
    char name[256];
    double value;
} Variable;

static Variable variables[MAX_VARIABLES];
static int variable_count = 0;

static double* find_variable(const char* name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i].value;
        }
    }

    return NULL;
}

static void set_variable(const char* name, double value) {
    double* existing = find_variable(name);

    if (existing) {
        *existing = value;
        return;
    }

    if (variable_count >= MAX_VARIABLES) {
        fprintf(
            stderr,
            "[VM Error] Variable limit reached\n"
        );
        exit(1);
    }

    strncpy(variables[variable_count].name, name, 255);
    variables[variable_count].name[255] = '\0';
    variables[variable_count].value = value;

    variable_count++;
}

static double evaluate(ASTNode* node) {
    if (!node) return 0.0;

    switch (node->type) {
        case NODE_NUMBER_LITERAL: {
            return node->token.value;
        }

        case NODE_VARIABLE: {
            double* value = find_variable(node->token.lexeme);

            if (!value) {
                fprintf(
                    stderr,
                    "[VM Error] Undefined variables '%s' at line %d\n",
                    node->token.lexeme,
                    node->token.line
                );

                return 0.0;
            }

            return *value;
        }

        case NODE_BINARY_OP: {
            double left_value = evaluate(node->left);
            double right_value = evaluate(node->right);

            switch (node->token.type) {
                case TOKEN_OP_ADD:
                    return left_value + right_value;

                case TOKEN_OP_SUB:
                    return left_value - right_value;

                case TOKEN_OP_MUL:
                    return left_value * right_value;

                case TOKEN_OP_DIV:
                    if (right_value == 0.0) {
                        fprintf(
                            stderr,
                            "[VM Error] Division by zero at line %d\n",
                            node->token.line
                        );
                        exit(1);
                    }

                    return left_value / right_value;

                default:
                    break;
            }

            return 0.0;
        }

        case NODE_PRINT: {
            return evaluate(node->left);
        } 

        case NODE_LET: {
            return evaluate(node->left);
        }

        case NODE_ASSIGN: {
            return evaluate(node->left);
        }

        default: {
            return 0.0;
        }
    }
}

static void execute_statement(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PRINT: {
            double value = evaluate(node->left);
            printf("[Kinetra] %g\n", value);
            break;
        }

        case NODE_LET: {
            double value = evaluate(node->left);
            set_variable(node->token.lexeme, value);
            break;
        }

        case NODE_ASSIGN: {
            double value = evaluate(node->left);
            set_variable(node->token.lexeme, value);
            break;
        }

        default: {
            evaluate(node);
            break;
        }
    }
}

void execute(ASTNode* ast) {
    if (!ast || ast->type != NODE_PROGRAM) {
        return;
    }

    for (int i = 0; i < ast->statement_count; i++) {
        execute_statement(ast->statements[i]);
    }
}