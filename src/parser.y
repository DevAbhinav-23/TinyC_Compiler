%{
#include "ast.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

extern int yylex();
extern int yylineno;
extern char* yytext;
void yyerror(const char* s);
extern FILE* yyin;

Program* g_program = nullptr;
%}

%union {
    int int_val;
    char* string_val;
    void* ptr;
}

%token <int_val> INT_LIT
%token <string_val> IDENT
%token INT_KW FLOAT_KW VOID_KW RETURN_KW IF_KW ELSE_KW WHILE_KW FOR_KW PRINT_KW
%token EQ NEQ LEQ GEQ AND OR NOT
%token PLUS MINUS STAR SLASH PCT
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA ASSIGN

%left OR
%left AND
%left EQ NEQ
%left '<' '>' LEQ GEQ
%left PLUS MINUS
%left STAR SLASH PCT
%right NOT UMINUS

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE_KW

%type <ptr> program function_list function
%type <ptr> param_list param_list_nonempty param
%type <ptr> block stmt_list stmt
%type <ptr> var_decl assign_stmt if_stmt while_stmt for_stmt return_stmt print_stmt expr_stmt
%type <ptr> expr arg_list arg_list_nonempty

%start program

%%

program
    : function_list { g_program = (Program*)$1; }
    ;

function_list
    : function { 
        auto* prog = new Program;
        prog->functions.push_back(std::unique_ptr<FunctionAST>((FunctionAST*)$1));
        $$ = prog;
    }
    | function_list function {
        auto* prog = (Program*)$1;
        prog->functions.push_back(std::unique_ptr<FunctionAST>((FunctionAST*)$2));
        $$ = prog;
    }
    ;

function
    : INT_KW IDENT LPAREN param_list RPAREN block {
        auto* func = new FunctionAST;
        func->name = std::string($2);
        free($2);
        ParamList* params = (ParamList*)$4;
        func->params = std::move(params->params);
        delete params;
        StmtList* body = (StmtList*)$6;
        for (auto* s : body->stmts)
            func->body.push_back(std::unique_ptr<Stmt>(s));
        delete body;
        $$ = func;
    }
    ;

param_list
    : param_list_nonempty { $$ = $1; }
    | /* empty */ { $$ = new ParamList; }
    ;

param_list_nonempty
    : param {
        auto* list = new ParamList;
        list->params.push_back(*(Param*)$1);
        delete (Param*)$1;
        $$ = list;
    }
    | param_list_nonempty COMMA param {
        auto* list = (ParamList*)$1;
        list->params.push_back(*(Param*)$3);
        delete (Param*)$3;
        $$ = list;
    }
    ;

param
    : INT_KW IDENT {
        auto* p = new Param;
        p->type = "int";
        p->name = std::string($2);
        free($2);
        $$ = p;
    }
    ;

block
    : LBRACE stmt_list RBRACE { $$ = $2; }
    ;

stmt_list
    : /* empty */ { $$ = new StmtList; }
    | stmt_list stmt {
        auto* list = (StmtList*)$1;
        list->stmts.push_back((Stmt*)$2);
        $$ = list;
    }
    ;

stmt
    : var_decl { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | expr_stmt { $$ = $1; }
    | if_stmt { $$ = $1; }
    | while_stmt { $$ = $1; }
    | for_stmt { $$ = $1; }
    | return_stmt { $$ = $1; }
    | print_stmt { $$ = $1; }
    ;

var_decl
    : INT_KW IDENT ASSIGN expr SEMICOLON {
        $$ = new VarDeclStmt(std::string($2), std::unique_ptr<Expr>((Expr*)$4));
        free($2);
    }
    ;

assign_stmt
    : IDENT ASSIGN expr SEMICOLON {
        $$ = new AssignStmt(std::string($1), std::unique_ptr<Expr>((Expr*)$3));
        free($1);
    }
    ;

expr_stmt
    : expr SEMICOLON {
        $$ = new ExprStmt(std::unique_ptr<Expr>((Expr*)$1));
    }
    ;

if_stmt
    : IF_KW LPAREN expr RPAREN block %prec LOWER_THAN_ELSE {
        auto* s = new IfStmt;
        s->cond = std::unique_ptr<Expr>((Expr*)$3);
        StmtList* then = (StmtList*)$5;
        for (auto* st : then->stmts)
            s->thenBody.push_back(std::unique_ptr<Stmt>(st));
        delete then;
        $$ = s;
    }
    | IF_KW LPAREN expr RPAREN block ELSE_KW block {
        auto* s = new IfStmt;
        s->cond = std::unique_ptr<Expr>((Expr*)$3);
        StmtList* then = (StmtList*)$5;
        for (auto* st : then->stmts)
            s->thenBody.push_back(std::unique_ptr<Stmt>(st));
        delete then;
        StmtList* els = (StmtList*)$7;
        for (auto* st : els->stmts)
            s->elseBody.push_back(std::unique_ptr<Stmt>(st));
        delete els;
        $$ = s;
    }
    ;

while_stmt
    : WHILE_KW LPAREN expr RPAREN block {
        auto* s = new WhileStmt;
        s->cond = std::unique_ptr<Expr>((Expr*)$3);
        StmtList* body = (StmtList*)$5;
        for (auto* st : body->stmts)
            s->body.push_back(std::unique_ptr<Stmt>(st));
        delete body;
        $$ = s;
    }
    ;

for_stmt
    : FOR_KW LPAREN var_decl expr SEMICOLON IDENT ASSIGN expr RPAREN block {
        auto* s = new ForStmt;
        s->init = std::unique_ptr<Stmt>((Stmt*)$3);
        s->cond = std::unique_ptr<Expr>((Expr*)$4);
        s->incr = std::unique_ptr<AssignStmt>(
            new AssignStmt(std::string($6), std::unique_ptr<Expr>((Expr*)$8)));
        free($6);
        StmtList* body = (StmtList*)$10;
        for (auto* st : body->stmts)
            s->body.push_back(std::unique_ptr<Stmt>(st));
        delete body;
        $$ = s;
    }
    ;

return_stmt
    : RETURN_KW expr SEMICOLON {
        $$ = new ReturnStmt(std::unique_ptr<Expr>((Expr*)$2));
    }
    | RETURN_KW SEMICOLON {
        $$ = new ReturnStmt(nullptr);
    }
    ;

print_stmt
    : PRINT_KW LPAREN expr RPAREN SEMICOLON {
        $$ = new PrintStmt(std::unique_ptr<Expr>((Expr*)$3));
    }
    ;

expr
    : INT_LIT {
        $$ = new NumberExpr($1);
    }
    | IDENT {
        $$ = new VariableExpr(std::string($1));
        free($1);
    }
    | IDENT LPAREN arg_list RPAREN {
        std::vector<std::unique_ptr<Expr>> args;
        auto* list = (std::vector<Expr*>*)$3;
        if (list) {
            for (auto* e : *list)
                args.push_back(std::unique_ptr<Expr>(e));
            delete list;
        }
        $$ = new CallExpr(std::string($1), std::move(args));
        free($1);
    }
    | LPAREN expr RPAREN {
        $$ = $2;
    }
    | expr PLUS expr {
        $$ = new BinaryExpr('+', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr MINUS expr {
        $$ = new BinaryExpr('-', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr STAR expr {
        $$ = new BinaryExpr('*', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr SLASH expr {
        $$ = new BinaryExpr('/', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr PCT expr {
        $$ = new BinaryExpr('%', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr EQ expr {
        $$ = new BinaryExpr('=', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr NEQ expr {
        $$ = new BinaryExpr('!', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr '<' expr {
        $$ = new BinaryExpr('<', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr '>' expr {
        $$ = new BinaryExpr('>', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr LEQ expr {
        $$ = new BinaryExpr('l', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr GEQ expr {
        $$ = new BinaryExpr('g', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr AND expr {
        $$ = new BinaryExpr('a', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | expr OR expr {
        $$ = new BinaryExpr('o', std::unique_ptr<Expr>((Expr*)$1), std::unique_ptr<Expr>((Expr*)$3));
    }
    | NOT expr {
        $$ = new UnaryExpr('!', std::unique_ptr<Expr>((Expr*)$2));
    }
    | MINUS expr %prec UMINUS {
        $$ = new UnaryExpr('-', std::unique_ptr<Expr>((Expr*)$2));
    }
    ;

arg_list
    : arg_list_nonempty { $$ = $1; }
    | /* empty */ { $$ = nullptr; }
    ;

arg_list_nonempty
    : expr {
        auto* list = new std::vector<Expr*>;
        list->push_back((Expr*)$1);
        $$ = list;
    }
    | arg_list_nonempty COMMA expr {
        auto* list = (std::vector<Expr*>*)$1;
        list->push_back((Expr*)$3);
        $$ = list;
    }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "Parse error at line %d near '%s': %s\n", yylineno, yytext, s);
}
