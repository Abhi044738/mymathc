%{
#include "parser.tab.h"
#include <stdlib.h>
#include <string.h>

extern void add_token(const char *type, const char *text);
%}

%option noyywrap

%%

[ \t\r\n]+                      { /* skip whitespace */ }
";"                             { add_token("SEMICOLON", yytext); return ';'; }
"sin"                           { add_token("SIN", yytext);   return SIN; }
"cos"                           { add_token("COS", yytext);   return COS; }
"tan"                           { add_token("TAN", yytext);   return TAN; }
"log"                           { add_token("LOG", yytext);   return LOG; }
"exp"                           { add_token("EXP", yytext);   return EXP; }
"sqrt"                          { add_token("SQRT", yytext);  return SQRT; }
"^"                             { add_token("POW", yytext);   return '^'; }
"+"                             { add_token("PLUS", yytext);  return '+'; }
"-"                             { add_token("MINUS", yytext); return '-'; }
"*"                             { add_token("MULT", yytext);  return '*'; }
"/"                             { add_token("DIV", yytext);   return '/'; }
"("                             { add_token("LPAREN", yytext);return '('; }
")"                             { add_token("RPAREN", yytext);return ')'; }
[0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)? {
                                  add_token("NUMBER", yytext);
                                  yylval.dval = atof(yytext);
                                  return NUMBER;
                                }
.                               { /* ignore unknown */ }

%%


