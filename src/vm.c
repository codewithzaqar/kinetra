#include "../include/kinetra.h"
#include "../include/hpc_math.h"

static double evaluate(ASTNode* node) {
    if (!node) return 0.0;

    switch (node->type) {
        case NODE_NUMBER_LITERAL:
            return node->token.value;

        case NODE_BINARY_OP: {
            double left_val = evaluate(node->left);
            double right_val = evaluate(node->right);
            
            switch (node->token.type) {
                case TOKEN_OP_ADD: return left_val + right_val;
                case TOKEN_OP_SUB: return left_val - right_val;
                case TOKEN_OP_MUL: return left_val * right_val;
                case TOKEN_OP_DIV: 
                    if (right_val == 0.0) {
                        fprintf(stderr, "[VM Error] Division by zero in simulation!\n");
                        exit(1);
                    }
                    return left_val / right_val;
                default: break;
            }
        }
        
        case NODE_VARIABLE:
            // v0.0.1a01: Variables are placeholders returning 0.0
            printf("[VM] Resolving variable: %s\n", node->token.lexeme);
            return 0.0;

        default:
            break;
    }
    return 0.0;
}

void execute(ASTNode* ast) {
    if (ast->type == NODE_PROGRAM && ast->left) {
        double result = evaluate(ast->left);
        printf("[VM] Computation Result: %f\n", result);
        
        // Example: Triggering HPC math subsystem from VM
        Vec3 pos = {0.0, 0.0, 0.0};
        Vec3 vel = {1.0, 2.0, 0.0};
        double dt = 0.016; // 60 FPS simulation step
        
        Vec3 new_pos = sim_integrate_euler(pos, vel, dt);
        printf("[VM] Sim Step -> Pos: (%.3f, %.3f, %.3f)\n", new_pos.x, new_pos.y, new_pos.z);
    }
}