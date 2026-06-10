%{
#include "parser.tab.h"
#include <cstring>
#include <cstdlib>
%}

%option noyywrap
%option yylineno

DIGIT       [0-9]
ID          [a-zA-Z_][a-zA-Z0-9_]*

%%

"int"           { return INT_KW; }
"float"         { return FLOAT_KW; }
"void"          { return VOID_KW; }
"return"        { return RETURN_KW; }
"if"            { return IF_KW; }
"else"          { return ELSE_KW; }
"while"         { return WHILE_KW; }
"for"           { return FOR_KW; }
"print"         { return PRINT_KW; }

"=="            { return EQ; }
"!="            { return NEQ; }
"<="            { return LEQ; }
">="            { return GEQ; }
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }

"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return STAR; }
"/"             { return SLASH; }
"%"             { return PCT; }
"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
";"             { return SEMICOLON; }
","             { return COMMA; }
"="             { return ASSIGN; }
"<"             { return '<'; }
">"             { return '>'; }

{DIGIT}+        { yylval.int_val = atoi(yytext); return INT_LIT; }
{ID}            { yylval.string_val = strdup(yytext); return IDENT; }

[ \t\r]+        { /* skip whitespace */ }
\n              { /* skip newlines */ }
"//".*          { /* skip line comment */ }
"/*"([^*]|"*"+[^*/])*"*"+"/"  { /* skip block comment */ }

.               { fprintf(stderr, "Line %d: Unknown character '%s'\n", yylineno, yytext); }

%%
