#include "../include/kinetra.h"
#include "../include/hpc_math.h"
#include <math.h>

#define MAX_VARIABLES 1024

typedef enum {
    K_VALUE_NUMBER,
    K_VALUE_VEC3
} KValueType;

typedef struct {
    KValueType type;

    double number;

    double x;
    double y;
    double z;
} KValue;

typedef struct {
    char name[256];
    KValue value;
} Variable;

static Variable variables[MAX_VARIABLES];
static int variable_count = 0;

static void runtime_error(const char* message, int line) {
    fprintf(
        stderr,
        "[VM Error] %s at line %d\n",
        message,
        line
    );
    exit(1);
}

static KValue make_number(double value) {
    KValue v;
    v.type = K_VALUE_NUMBER;
    v.number = value;
    v.x = 0.0;
    v.y = 0.0;
    v.z = 0.0;
    return v;
}

static KValue make_vec3(double x, double y, double z) {
    KValue v;
    v.type = K_VALUE_VEC3;
    v.number = 0.0;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

static bool is_number(KValue value) {
    return value.type == K_VALUE_NUMBER;
}

static bool is_vec3(KValue value) {
    return value.type == K_VALUE_VEC3;
}

static KValue* find_variable(const char* name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i].value;
        }
    }

    return NULL;
}

static void set_variable(const char* name, KValue value) {
    KValue* existing = find_variable(name);

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

static void set_variable_number(const char* name, double value) {
    set_variable(name, make_number(value));
}

static void print_value(KValue value) {
    if (is_number(value)) {
        printf("[Kinetra] %g\n", value.number);
    } else if (is_vec3(value)) {
        printf(
            "[Kinetra] vec3(%g, %g, %g)\n",
            value.x,
            value.y,
            value.z
        );
    }
}

static Vec3 kvalue_to_vec3(KValue value) {
    Vec3 v;
    v.x = value.x;
    v.y = value.y;
    v.z = value.z;
    return v;
}

static KValue vec3_to_kvalue(Vec3 v) {
    return make_vec3(v.x, v.y, v.z);
}

static double require_number_arg(KValue value, const char* fn, int line) {
    if (!is_number(value)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s() expects number arguments", fn);
        runtime_error(msg, line);
    }

    return value.number;
}

static KValue evaluate(ASTNode* node);
static void execute_statement(ASTNode* node);

static KValue apply_binary_op(Token op, KValue left, KValue right) {
    switch (op.type) {
        case TOKEN_OP_ADD: {
            if (is_number(left) && is_number(right)) {
                return make_number(left.number + right.number);
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_vec3(
                    left.x + right.x,
                    left.y + right.y,
                    left.z + right.z
                );
            }

            runtime_error("Invalid operands to '+'", op.line);
            return make_number(0.0);
        } 

        case TOKEN_OP_SUB: {
            if (is_number(left) && is_number(right)) {
                return make_number(left.number - right.number);
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_vec3(
                    left.x - right.x,
                    left.y - right.y,
                    left.z - right.z
                );
            }

            runtime_error("Invalid operands to '-'", op.line);
            return make_number(0.0);
        }

        case TOKEN_OP_MUL: {
            if (is_number(left) && is_number(right)) {
                return make_number(left.number * right.number);
            }

            if (is_vec3(left) && is_number(right)) {
                return make_vec3(
                    left.x * right.number,
                    left.y * right.number,
                    left.z * right.number
                );
            }

            if (is_number(left) && is_vec3(right)) {
                return make_vec3(
                    right.x * left.number,
                    right.y * left.number,
                    right.z * left.number
                );
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_vec3(
                    left.x * right.x,
                    left.y * right.y,
                    left.z * right.z
                );
            }

            runtime_error("Invalid operands to '*'", op.line);
            return make_number(0.0);
        }

        case TOKEN_OP_DIV: {
            if (is_number(left) && is_number(right)) {
                if (right.number == 0.0) {
                    runtime_error("Division by zero", op.line);
                }

                return make_number(left.number / right.number);
            }

            if (is_vec3(left) && is_number(right)) {
                if (right.number == 0.0) {
                    runtime_error("Division by zero", op.line);
                }

                return make_vec3(
                    left.x / right.number,
                    left.y / right.number,
                    left.z / right.number
                );
            }

            runtime_error("Invalid operands to '/'", op.line);
            return make_number(0.0);
        }

        default: {
            runtime_error("Unknown operands", op.line);
            return make_number(0.0);
        }
    }
}

static KValue evaluate(ASTNode* node) {
    if (!node) return make_number(0.0);

    switch (node->type) {
        case NODE_NUMBER_LITERAL: {
            return make_number(node->token.value);
        }

        case NODE_VARIABLE: {
            KValue* value = find_variable(node->token.lexeme);

            if (!value) {
                runtime_error(
                    "Undefined variable",
                    node->token.line
                );
            }

            return *value;
        }

        case NODE_VEC3: {
            if (!node->left || !node->right || !node->third) {
                runtime_error(
                    "Invalid vec3 constructor",
                    node->token.line
                );
            }

            KValue x = evaluate(node->left);
            KValue y = evaluate(node->right);
            KValue z = evaluate(node->third);

            if (!is_number(x) || !is_number(y) || !is_number(z)) {
                runtime_error(
                    "vec3 arguments must be numbers",
                    node->token.line
                );
            }

            return make_vec3(x.number, y.number, z.number);
        }

        case NODE_CALL: {
            const char* name = node->token.lexeme;

            if (strcmp(name, "dot") == 0) {
                KValue a = evaluate(node->statements[0]);
                KValue b = evaluate(node->statements[1]);

                if (!is_vec3(a) || !is_vec3(b)) {
                    runtime_error("dot() expects vec3 arguments", node->token.line);
                }

                return make_number(
                    vec3_dot(kvalue_to_vec3(a), kvalue_to_vec3(b))
                );
            }

            if (strcmp(name, "cross") == 0) {
                KValue a = evaluate(node->statements[0]);
                KValue b = evaluate(node->statements[1]);

                if (!is_vec3(a) || !is_vec3(b)) {
                    runtime_error("cross() expects vec3 arguments", node->token.line);
                }

                return vec3_to_kvalue(
                    vec3_cross(kvalue_to_vec3(a), kvalue_to_vec3(b))
                );
            }

            if (strcmp(name, "length") == 0) {
                KValue a = evaluate(node->statements[0]);

                if (!is_vec3(a)) {
                    runtime_error("length() expects a vec3 arguments", node->token.line);
                }

                return make_number(vec3_length(kvalue_to_vec3(a)));
            }

            if (strcmp(name, "normalize") == 0) {
                KValue a = evaluate(node->statements[0]);

                if (!is_vec3(a)) {
                    runtime_error("normalize() expects a vec3 argument", node->token.line);
                }

                Vec3 v = kvalue_to_vec3(a);
                double len = vec3_length(v);

                if (len == 0.0) {
                    runtime_error("normalize() of zero-length vector", node->token.line);
                }

                return vec3_to_kvalue(vec3_normalize(v));
            }

            if (strcmp(name, "sqrt") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token.line
                );

                if (a < 0.0) {
                    runtime_error("sqrt() of negative number", node->token.line);
                }

                return make_number(sqrt(a));
            }

            if (strcmp(name, "abs") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token.line
                );

                return make_number(fabs(a));
            }

            if (strcmp(name, "min") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token.line
                );
                double b = require_number_arg(
                    evaluate(node->statements[1]), name, node->token.line
                );

                return make_number(a < b ? a : b);
            }

            if (strcmp(name, "max") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token.line
                );
                double b = require_number_arg(
                    evaluate(node->statements[1]), name, node->token.line
                );

                return make_number(a > b ? a : b);
            }

            if (strcmp(name, "sin") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token.line
                );

                return make_number(sin(a));
            }

            if (strcmp(name, "cos") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token.line
                );

                return make_number(cos(a));
            }

            if (strcmp(name, "tan") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token.line
                );

                return make_number(tan(a));
            }

            runtime_error("Unknown built-in function", node->token.line);
            return make_number(0.0);
        } 

        case NODE_BINARY_OP: {
            KValue left = evaluate(node->left);
            KValue right = evaluate(node->right);

            return apply_binary_op(node->token, left, right);
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

        case NODE_STEP: {
            return evaluate(node->left);
        }

        case NODE_SIMULATION_BLOCK: {
            return make_number(0.0);
        }

        default: {
            return make_number(0.0);
        }
    }
}

static void execute_sim(ASTNode* node) {
    double step_count = 0.0;

    // Form:
    // sim expression {...}
    if (node->left) {
        KValue count_value = evaluate(node->left);

        if (!is_number(count_value)) {
            runtime_error(
                "sim step count must be a number",
                node->token.line
            );
        }

        step_count = count_value.number;
    }

    // Form:
    // sim {step expression ...}
    else {
        for (int i = 0; i < node->statement_count; i++) {
            ASTNode* stmt = node->statements[i];

            if (stmt->type == NODE_STEP) {
                KValue count_value = evaluate(stmt->left);

                if (!is_number(count_value)) {
                    runtime_error(
                        "step count must be a number",
                        stmt->token.line
                    );
                }

                step_count = count_value.number;
                break;
            }
        }
    }

    long steps = (long)step_count;

    if (steps < 0) {
        steps = 0;
    }

    double dt_value = 0.0;

    if (node->right) {
        KValue dt_val = evaluate(node->right);

        if (!is_number(dt_val)) {
            runtime_error("sim dt value must be a number", node->token.line);
        }

        dt_value = dt_val.number;
    }

    else if (steps > 0) {
        dt_value = 1.0 / (double)steps;
    }

    for (long i = 0; i < steps; i++) {
        // Built-in loop index
        set_variable_number("step_index", (double)i);
        set_variable_number("dt", dt_value);

        for (int j = 0; j < node->statement_count; j++) {
            ASTNode* stmt = node->statements[j];

            // The step declaration controls the loop;
            // it is not executed as a normal body statement.
            if (stmt->type == NODE_STEP) {
                continue;
            }

            execute_statement(stmt);
        }
    }
}

static void execute_statement(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PRINT: {
            KValue value = evaluate(node->left);
            print_value(value);
            break;
        }

        case NODE_LET: {
            KValue value = evaluate(node->left);
            set_variable(node->token.lexeme, value);
            break;
        }

        case NODE_ASSIGN: {
            KValue value = evaluate(node->left);
            set_variable(node->token.lexeme, value);
            break;
        }

        case NODE_STEP: {
            // Step nodes are handles by execute_sim().
            break;
        }

        case NODE_SIMULATION_BLOCK: {
            execute_sim(node);
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