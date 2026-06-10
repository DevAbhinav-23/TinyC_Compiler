#pragma once

#include <string>
#include <vector>
#include <memory>

namespace llvm { class Value; }
class CodeGen;

// ==================== Expressions ====================

struct Expr {
    virtual ~Expr() = default;
    virtual llvm::Value* codegen(CodeGen& cg) = 0;
};

struct NumberExpr : Expr {
    int value;
    NumberExpr(int value) : value(value) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct VariableExpr : Expr {
    std::string name;
    VariableExpr(const std::string& name) : name(name) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct BinaryExpr : Expr {
    char op;
    std::unique_ptr<Expr> lhs, rhs;
    BinaryExpr(char op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct UnaryExpr : Expr {
    char op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(char op, std::unique_ptr<Expr> operand)
        : op(op), operand(std::move(operand)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(const std::string& callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(callee), args(std::move(args)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

// ==================== Statements ====================

struct Stmt {
    virtual ~Stmt() = default;
    virtual llvm::Value* codegen(CodeGen& cg) = 0;
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    ExprStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct VarDeclStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> init;
    VarDeclStmt(const std::string& name, std::unique_ptr<Expr> init)
        : name(name), init(std::move(init)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignStmt(const std::string& name, std::unique_ptr<Expr> value)
        : name(name), value(std::move(value)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::vector<std::unique_ptr<Stmt>> thenBody;
    std::vector<std::unique_ptr<Stmt>> elseBody;
    llvm::Value* codegen(CodeGen& cg) override;
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::vector<std::unique_ptr<Stmt>> body;
    llvm::Value* codegen(CodeGen& cg) override;
};

struct ForStmt : Stmt {
    std::unique_ptr<Stmt> init;
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> incr;
    std::vector<std::unique_ptr<Stmt>> body;
    llvm::Value* codegen(CodeGen& cg) override;
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    ReturnStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> value;
    PrintStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}
    llvm::Value* codegen(CodeGen& cg) override;
};

// ==================== Helpers for Bison ====================

struct Param {
    std::string type;
    std::string name;
};

struct ParamList {
    std::vector<Param> params;
};

struct StmtList {
    std::vector<Stmt*> stmts;
};

// ==================== Function and Program ====================

struct FunctionAST {
    std::string name;
    std::vector<Param> params;
    std::vector<std::unique_ptr<Stmt>> body;
};

struct Program {
    std::vector<std::unique_ptr<FunctionAST>> functions;
};
