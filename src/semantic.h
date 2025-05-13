#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "cJSON.h"

void init_semantic(void);
void check_semantics(ASTNode *root);
cJSON* get_semantic_json(void);

#endif // SEMANTIC_H