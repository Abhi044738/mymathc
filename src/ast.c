#include <stdlib.h>
#include "ast.h"
#include <math.h>

ASTNode* make_num(double v) {
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = NODE_NUM;
    n->value = v;
    n->left = n->right = NULL;
    return n;
}
double eval(ASTNode *n) {
    if (!n) return 0.0;
    switch (n->type) {
      case NODE_NUM:  return n->value;
      case NODE_ADD:  return eval(n->left) + eval(n->right);
      case NODE_SUB:  return eval(n->left) - eval(n->right);
      case NODE_MUL:  return eval(n->left) * eval(n->right);
      case NODE_DIV:  return eval(n->left) / eval(n->right);
      case NODE_POW:  return pow(eval(n->left), eval(n->right));
      case NODE_NEG:  return -eval(n->left);
      case NODE_SIN:  return sin(eval(n->left));
      case NODE_COS:  return cos(eval(n->left));
      case NODE_TAN:  return tan(eval(n->left));
      case NODE_LOG:  return log(eval(n->left));
      case NODE_EXP:  return exp(eval(n->left));
      case NODE_SQRT: return sqrt(eval(n->left));
    }
    return 0.0;  /* should never reach */
}

ASTNode* make_bin(NodeType t, ASTNode *l, ASTNode *r) {
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = t;
    n->left = l;
    n->right = r;
    return n;
}

ASTNode* make_unary(NodeType t, ASTNode *c) {
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = t;
    n->left = c;
    n->right = NULL;
    return n;
}

void free_ast(ASTNode *n) {
    if (!n) return;
    free_ast(n->left);
    free_ast(n->right);
    free(n);
}
