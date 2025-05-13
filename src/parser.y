%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern int yylex(void);
extern void yyerror(const char *);
extern void add_statement(ASTNode *n);
%}

%code requires {
  #include "ast.h"
}

%union {
    double dval;
    ASTNode *node;
}

%token <dval> NUMBER
%token SIN COS TAN LOG EXP SQRT
%left '+' '-'
%left '*' '/'
%left NEG
%right '^'
%type <node> expr

%%

input:
    /* empty */
  | input stmt
  ;

stmt:
    expr ';'    { add_statement($1); }
  ;

expr:
    NUMBER               { $$ = make_num($1); }
  | expr '+' expr       { $$ = make_bin(NODE_ADD, $1, $3); }
  | expr '-' expr       { $$ = make_bin(NODE_SUB, $1, $3); }
  | expr '*' expr       { $$ = make_bin(NODE_MUL, $1, $3); }
  | expr '/' expr       { $$ = make_bin(NODE_DIV, $1, $3); }
  | '-' expr   %prec NEG{ $$ = make_unary(NODE_NEG, $2); }
  | expr '^' expr       { $$ = make_bin(NODE_POW, $1, $3); }
  | '(' expr ')'        { $$ = $2; }
  | SIN '(' expr ')'    { $$ = make_unary(NODE_SIN, $3); }
  | COS '(' expr ')'    { $$ = make_unary(NODE_COS, $3); }
  | TAN '(' expr ')'    { $$ = make_unary(NODE_TAN, $3); }
  | LOG '(' expr ')'    { $$ = make_unary(NODE_LOG, $3); }
  | EXP '(' expr ')'    { $$ = make_unary(NODE_EXP, $3); }
  | SQRT '(' expr ')'   { $$ = make_unary(NODE_SQRT, $3); }
  ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
}
