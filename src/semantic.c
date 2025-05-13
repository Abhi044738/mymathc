#include "semantic.h"
#include "ast.h"
#include "cJSON.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <errno.h>
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923


static cJSON *errors_arr = NULL;

static double eval_for_check(ASTNode *n) {
    if (!n) return 0.0;
    errno = 0;
    double result = 0.0;
    
    switch (n->type) {
        case NODE_NUM:  result = n->value; break;
        case NODE_ADD:  result = eval_for_check(n->left) + eval_for_check(n->right); break;
        case NODE_SUB:  result = eval_for_check(n->left) - eval_for_check(n->right); break;
        case NODE_MUL:  result = eval_for_check(n->left) * eval_for_check(n->right); break;
        case NODE_DIV:  result = eval_for_check(n->left) / eval_for_check(n->right); break;
        case NODE_POW:  result = pow(eval_for_check(n->left), eval_for_check(n->right)); break;
        case NODE_NEG:  result = -eval_for_check(n->left); break;
        case NODE_SIN:  result = sin(eval_for_check(n->left)); break;
        case NODE_COS:  result = cos(eval_for_check(n->left)); break;
        case NODE_TAN:  {
            double angle = eval_for_check(n->left);
            if (fabs(fmod(angle + M_PI_2, M_PI)) < 1e-6) {
                cJSON_AddItemToArray(errors_arr, cJSON_CreateString("Tangent asymptotic behavior"));
            }
            result = tan(angle);
            break;
        }
        case NODE_LOG:  result = log(eval_for_check(n->left)); break;
        case NODE_EXP:  {
            double exponent = eval_for_check(n->left);
            if (exponent > 700) {  // exp(709) overflows double
                cJSON_AddItemToArray(errors_arr, cJSON_CreateString("Exponential overflow"));
            }
            result = exp(exponent);
            break;
        }
        case NODE_SQRT: result = sqrt(eval_for_check(n->left)); break;
        default:        result = 0.0; break;
    }

    // Check for math library errors
    if (errno == ERANGE || errno == EDOM) {
        cJSON_AddItemToArray(errors_arr, cJSON_CreateString("Domain/range error in math function"));
        errno = 0;
    }
    
    return result;
}

static void check_node(ASTNode *n) {
    if (!n) return;
    
    if (n->type == NODE_DIV) {
        double denom = eval_for_check(n->right);
        if (denom == 0.0) {
            cJSON_AddItemToArray(errors_arr, cJSON_CreateString("Division by zero"));
        }
    }
    
    if (n->type == NODE_SQRT) {
        double operand = eval_for_check(n->left);
        if (operand < 0.0) {
            cJSON_AddItemToArray(errors_arr, cJSON_CreateString("Square root of negative number"));
        }
    }
    
    if (n->type == NODE_LOG) {
        double operand = eval_for_check(n->left);
        if (operand <= 0.0) {
            cJSON_AddItemToArray(errors_arr, cJSON_CreateString("Logarithm of non-positive number"));
        }
    }
    
    if (n->type == NODE_POW) {
        double base = eval_for_check(n->left);
        double exponent = eval_for_check(n->right);
        if (base == 0.0 && exponent <= 0.0) {
            cJSON_AddItemToArray(errors_arr, cJSON_CreateString("Zero raised to non-positive power"));
        }
    }
    
    check_node(n->left);
    check_node(n->right);
}

void init_semantic() {
    if (errors_arr) cJSON_Delete(errors_arr);
    errors_arr = cJSON_CreateArray();
}

void check_semantics(ASTNode *root) {
    check_node(root);
    
    if (cJSON_GetArraySize(errors_arr) == 0) {
        double result = eval_for_check(root);
        
        if (isinf(result)) {
            cJSON_AddItemToArray(errors_arr, 
                cJSON_CreateString("Arithmetic overflow/underflow"));
        }
        else if (isnan(result)) {
            cJSON_AddItemToArray(errors_arr, 
                cJSON_CreateString("Undefined mathematical result"));
        }
        
        // Detect underflow (subnormal numbers)
        if (fpclassify(result) == FP_SUBNORMAL) {
            cJSON_AddItemToArray(errors_arr,
                cJSON_CreateString("Arithmetic underflow"));
        }
        
        if (fabs(result) > DBL_MAX) {
            cJSON_AddItemToArray(errors_arr,
                cJSON_CreateString("Result exceeds double precision limits"));
        }
    }
}

cJSON* get_semantic_json() {
    return cJSON_Duplicate(errors_arr, 1);
}