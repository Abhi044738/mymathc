#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "ast.h"
#include "semantic.h"
#include "ir.h"
#include "opt.h"
#include "codegen.h"
#include "parser.tab.h"
#include <math.h>

/* ---- token & statement storage ---- */
typedef struct {
    cJSON *tokens;
    ASTNode *ast;
} Stmt;
/* Flex buffer API (no header provided by Flex, so declare it yourself) */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void           yy_delete_buffer(YY_BUFFER_STATE buffer);


static Stmt *stmts = NULL;
static int stmt_count = 0;

static void generate_ir_for_statement(ASTNode *ast, cJSON *ir_array) {
    init_ir();
    if (!ir_has_error()) {
        gen_ir(ast);
        cJSON *ir_json = get_ir_json();
        cJSON_AddItemToArray(ir_array, ir_json);
    } else {
        cJSON_AddItemToArray(ir_array, cJSON_CreateArray());
    }
}


/* collect tokens per statement */
void add_token(const char *type, const char *text) {
    if (stmt_count == 0) {
        // Create the first statement entry when the first token is encountered
        stmt_count++;
        stmts = realloc(stmts, stmt_count * sizeof(*stmts));
        stmts[0].tokens = cJSON_CreateArray();
        stmts[0].ast = NULL;
    }
    cJSON *tok = cJSON_CreateObject();
    cJSON_AddStringToObject(tok, "type", type);
    cJSON_AddStringToObject(tok, "value", text);
    cJSON_AddItemToArray(stmts[stmt_count - 1].tokens, tok);
}

/* collect AST per statement */
void add_statement(ASTNode *n) {
    if (stmt_count > 0) {
        // Assign the AST to the current statement
        stmts[stmt_count - 1].ast = n;
    }
    // Prepare for the next statement
    stmt_count++;
    stmts = realloc(stmts, stmt_count * sizeof(*stmts));
    stmts[stmt_count - 1].tokens = cJSON_CreateArray();
    stmts[stmt_count - 1].ast = NULL;
}

/* ---- utility to convert AST to JSON ---- */
static cJSON* ast_to_json(ASTNode *n) {
    if (!n) return NULL;
    cJSON *o = cJSON_CreateObject();
    switch(n->type) {
        case NODE_NUM:
            cJSON_AddStringToObject(o, "type", "NUMBER");
            cJSON_AddNumberToObject(o, "value", n->value);
            break;
        case NODE_ADD: 
        case NODE_SUB: 
        case NODE_MUL: 
        case NODE_DIV: 
        case NODE_POW: 
            cJSON_AddStringToObject(o, "type", (n->type == NODE_ADD) ? "ADD" :
                                              (n->type == NODE_SUB) ? "SUB" :
                                              (n->type == NODE_MUL) ? "MUL" :
                                              (n->type == NODE_DIV) ? "DIV" : "POW");
            cJSON_AddItemToObject(o, "left", ast_to_json(n->left));
            cJSON_AddItemToObject(o, "right", ast_to_json(n->right));
            break;
        case NODE_NEG: 
        case NODE_SIN: 
        case NODE_COS: 
        case NODE_TAN: 
        case NODE_LOG: 
        case NODE_EXP: 
        case NODE_SQRT: 
            cJSON_AddStringToObject(o, "type", (n->type == NODE_NEG) ? "NEG" :
                                              (n->type == NODE_SIN) ? "SIN" :
                                              (n->type == NODE_COS) ? "COS" :
                                              (n->type == NODE_TAN) ? "TAN" :
                                              (n->type == NODE_LOG) ? "LOG" :
                                              (n->type == NODE_EXP) ? "EXP" : "SQRT");
            cJSON_AddItemToObject(o, "operand", ast_to_json(n->left));
            break;
        default: break;
    }
    return o;
}

int main(int argc, char **argv) {
    char input_buf[1024];
    char *input = NULL;
    if (argc > 1) {
        input = argv[1];
    } else {
        if (!fgets(input_buf, sizeof(input_buf), stdin)) {
            fprintf(stderr, "No input\n");
            return 1;
        }
        input = input_buf;
    }

    /* parse */
    stmts = NULL; stmt_count = 0;
    YY_BUFFER_STATE buf = yy_scan_string(input);
    yyparse();
    yy_delete_buffer(buf);

    /* build JSON */
    cJSON *root = cJSON_CreateObject();
    cJSON *j_tokens = cJSON_CreateArray();
    cJSON *j_asts   = cJSON_CreateArray();
    cJSON *j_sem    = cJSON_CreateArray();
    cJSON *j_ir     = cJSON_CreateArray();
    cJSON *j_opt    = cJSON_CreateArray();
    cJSON *j_code   = cJSON_CreateArray();
    cJSON *j_res    = cJSON_CreateArray();




    // After parsing, process each completed statement (stmt_count - 1)
    int num_statements = stmt_count > 0 ? stmt_count - 1 : 0;

    for (int i = 0; i < num_statements; i++) {
        /* tokens */
        cJSON_AddItemToArray(j_tokens, stmts[i].tokens);

        /* AST JSON */
        cJSON_AddItemToArray(j_asts, ast_to_json(stmts[i].ast));

        /* semantics */
        init_semantic();
        check_semantics(stmts[i].ast);
        cJSON *semantic_errors =cJSON_Duplicate( get_semantic_json(),1);
        cJSON_AddItemToArray(j_sem, semantic_errors);

        /* evaluation */
        double val; 
        if (cJSON_GetArraySize(semantic_errors) > 0) {
            val = NAN;
        } else {
            val = eval(stmts[i].ast);
        }

        // Represent NaN as null in JSON

        if (isnan(val)) {
            cJSON_AddItemToArray(j_res, cJSON_CreateNull());
        } else {
            cJSON_AddItemToArray(j_res, cJSON_CreateNumber(val));
        }

        /* IR */
        cJSON *ir_stmt = cJSON_CreateArray();
        generate_ir_for_statement(stmts[i].ast, ir_stmt);
        cJSON_AddItemToArray(j_ir, ir_stmt);



        /* optimize */
        init_opt();
        cJSON_AddItemToArray(j_opt, get_opt_json());

        /* codegen */
        init_codegen();
        cJSON_AddItemToArray(j_code, get_code_json());
        generate_assembly();


        free_ast(stmts[i].ast);
    }
    
    cJSON_AddItemToObject(root, "tokens",      j_tokens);
    cJSON_AddItemToObject(root, "asts",        j_asts);
    cJSON_AddItemToObject(root, "semantic",    j_sem);
    cJSON_AddItemToObject(root, "ir",          j_ir);
    cJSON_AddItemToObject(root, "opt_ir",      j_opt);
    cJSON_AddItemToObject(root, "asm",         j_code);
    cJSON_AddItemToObject(root, "results",     j_res);

    char *out = cJSON_Print(root);
    puts(out);
    cJSON_Delete(root);
    return 0;
}
