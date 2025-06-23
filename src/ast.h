#ifndef AST_H
#define AST_H

typedef enum {
    NODE_NUM, NODE_ADD, NODE_SUB, NODE_MUL, NODE_DIV, NODE_POW,
    NODE_NEG, NODE_SIN, NODE_COS, NODE_TAN, NODE_LOG, NODE_EXP, NODE_SQRT
} NodeType;

typedef struct ASTNode {
    NodeType type;
    double value;
    struct ASTNode *left, *right;
} ASTNode;

ASTNode* make_num(double v);
ASTNode* make_bin(NodeType t, ASTNode *l, ASTNode *r);
ASTNode* make_unary(NodeType t, ASTNode *c);
void free_ast(ASTNode *n);
double eval(ASTNode *n);
#endif // AST_H