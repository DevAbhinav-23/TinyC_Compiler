#include "codegen.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Instructions.h"
#include <cstdio>
#include <vector>

using namespace llvm;

CodeGen::CodeGen() : TheModule(std::make_unique<Module>("tinyc", Context)), Builder(Context) {}

void CodeGen::putVariable(const std::string& name, Value* val) {
    NamedValues[name] = val;
}

Value* CodeGen::getVariable(const std::string& name) {
    auto it = NamedValues.find(name);
    if (it != NamedValues.end()) return it->second;
    fprintf(stderr, "Error: Unknown variable '%s'\n", name.c_str());
    return nullptr;
}

Function* CodeGen::getFunction(const std::string& name) {
    auto it = Functions.find(name);
    if (it != Functions.end()) return it->second;
    return TheModule->getFunction(name);
}

Value* CodeGen::generate(Program& program) {
    // Declare printf
    std::vector<Type*> printfArgs({PointerType::getUnqual(Context)});
    FunctionType* printfType = FunctionType::get(Type::getInt32Ty(Context), printfArgs, true);
    Function::Create(printfType, Function::ExternalLinkage, "printf", TheModule.get());

    Value* lastVal = nullptr;
    for (auto& func : program.functions) {
        std::vector<Type*> paramTypes;
        for (auto& p : func->params)
            paramTypes.push_back(Type::getInt32Ty(Context));

        FunctionType* funcType = FunctionType::get(Type::getInt32Ty(Context), paramTypes, false);
        Function* function = Function::Create(funcType, Function::ExternalLinkage, func->name, TheModule.get());

        unsigned idx = 0;
        for (auto& p : func->params)
            function->getArg(idx++)->setName(p.name);

        BasicBlock* entryBB = BasicBlock::Create(Context, "entry", function);
        Builder.SetInsertPoint(entryBB);

        auto oldNamedValues = NamedValues;
        NamedValues.clear();
        idx = 0;
        for (auto& p : func->params) {
            Value* arg = function->getArg(idx++);
            AllocaInst* alloca = Builder.CreateAlloca(Type::getInt32Ty(Context), nullptr, p.name);
            Builder.CreateStore(arg, alloca);
            putVariable(p.name, alloca);
        }

        Functions[func->name] = function;

        for (auto& stmt : func->body)
            lastVal = stmt->codegen(*this);

        if (!Builder.GetInsertBlock()->getTerminator())
            Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(Context), 0));

        NamedValues = oldNamedValues;
        verifyFunction(*function);
    }
    return lastVal;
}

// ==================== Expr codegen ====================

Value* NumberExpr::codegen(CodeGen& cg) {
    return ConstantInt::get(Type::getInt32Ty(cg.Context), value);
}

Value* VariableExpr::codegen(CodeGen& cg) {
    Value* v = cg.getVariable(name);
    if (!v) return nullptr;
    return cg.Builder.CreateLoad(Type::getInt32Ty(cg.Context), v, name.c_str());
}

Value* BinaryExpr::codegen(CodeGen& cg) {
    Value* L = lhs->codegen(cg);
    Value* R = rhs->codegen(cg);
    if (!L || !R) return nullptr;

    switch (op) {
        case '+': return cg.Builder.CreateAdd(L, R, "addtmp");
        case '-': return cg.Builder.CreateSub(L, R, "subtmp");
        case '*': return cg.Builder.CreateMul(L, R, "multmp");
        case '/': return cg.Builder.CreateSDiv(L, R, "divtmp");
        case '%': return cg.Builder.CreateSRem(L, R, "modtmp");
        case '<': {
            Value* cmp = cg.Builder.CreateICmpSLT(L, R, "cmplt");
            return cg.Builder.CreateZExt(cmp, Type::getInt32Ty(cg.Context), "booltmp");
        }
        case '>': {
            Value* cmp = cg.Builder.CreateICmpSGT(L, R, "cmpgt");
            return cg.Builder.CreateZExt(cmp, Type::getInt32Ty(cg.Context), "booltmp");
        }
        case '=': {
            Value* cmp = cg.Builder.CreateICmpEQ(L, R, "cmpeq");
            return cg.Builder.CreateZExt(cmp, Type::getInt32Ty(cg.Context), "booltmp");
        }
        case '!': {
            Value* cmp = cg.Builder.CreateICmpNE(L, R, "cmpne");
            return cg.Builder.CreateZExt(cmp, Type::getInt32Ty(cg.Context), "booltmp");
        }
        case 'l': {
            Value* cmp = cg.Builder.CreateICmpSLE(L, R, "cmple");
            return cg.Builder.CreateZExt(cmp, Type::getInt32Ty(cg.Context), "booltmp");
        }
        case 'g': {
            Value* cmp = cg.Builder.CreateICmpSGE(L, R, "cmpge");
            return cg.Builder.CreateZExt(cmp, Type::getInt32Ty(cg.Context), "booltmp");
        }
        case 'a': {
            L = cg.Builder.CreateICmpNE(L, ConstantInt::get(Type::getInt32Ty(cg.Context), 0), "tobool1");
            R = cg.Builder.CreateICmpNE(R, ConstantInt::get(Type::getInt32Ty(cg.Context), 0), "tobool2");
            Value* andVal = cg.Builder.CreateAnd(L, R, "andtmp");
            return cg.Builder.CreateZExt(andVal, Type::getInt32Ty(cg.Context), "booltmp");
        }
        case 'o': {
            L = cg.Builder.CreateICmpNE(L, ConstantInt::get(Type::getInt32Ty(cg.Context), 0), "tobool1");
            R = cg.Builder.CreateICmpNE(R, ConstantInt::get(Type::getInt32Ty(cg.Context), 0), "tobool2");
            Value* orVal = cg.Builder.CreateOr(L, R, "ortmp");
            return cg.Builder.CreateZExt(orVal, Type::getInt32Ty(cg.Context), "booltmp");
        }
        default:
            fprintf(stderr, "Error: Unknown binary operator '%c'\n", op);
            return nullptr;
    }
}

Value* UnaryExpr::codegen(CodeGen& cg) {
    Value* V = operand->codegen(cg);
    if (!V) return nullptr;

    switch (op) {
        case '-': return cg.Builder.CreateNeg(V, "negtmp");
        case '!': {
            Value* zero = ConstantInt::get(Type::getInt32Ty(cg.Context), 0);
            Value* cmp = cg.Builder.CreateICmpEQ(V, zero, "nottmp");
            return cg.Builder.CreateZExt(cmp, Type::getInt32Ty(cg.Context), "booltmp");
        }
        default:
            fprintf(stderr, "Error: Unknown unary operator '%c'\n", op);
            return nullptr;
    }
}

Value* CallExpr::codegen(CodeGen& cg) {
    Function* calleeFn = cg.getFunction(callee);
    if (!calleeFn) {
        fprintf(stderr, "Error: Unknown function '%s'\n", callee.c_str());
        return nullptr;
    }

    if (calleeFn->arg_size() != args.size()) {
        fprintf(stderr, "Error: Function '%s' expects %zu args, got %zu\n",
                callee.c_str(), calleeFn->arg_size(), args.size());
        return nullptr;
    }

    std::vector<Value*> argsV;
    for (auto& arg : args) {
        Value* v = arg->codegen(cg);
        if (!v) return nullptr;
        argsV.push_back(v);
    }

    return cg.Builder.CreateCall(calleeFn, argsV, "calltmp");
}

// ==================== Stmt codegen ====================

Value* ExprStmt::codegen(CodeGen& cg) {
    return expr ? expr->codegen(cg) : nullptr;
}

Value* VarDeclStmt::codegen(CodeGen& cg) {
    AllocaInst* alloca = cg.Builder.CreateAlloca(Type::getInt32Ty(cg.Context), nullptr, name);
    if (init) {
        Value* val = init->codegen(cg);
        if (!val) return nullptr;
        cg.Builder.CreateStore(val, alloca);
    }
    cg.putVariable(name, alloca);
    return alloca;
}

Value* AssignStmt::codegen(CodeGen& cg) {
    Value* var = cg.getVariable(name);
    if (!var) return nullptr;
    Value* val = value->codegen(cg);
    if (!val) return nullptr;
    cg.Builder.CreateStore(val, var);
    return val;
}

Value* IfStmt::codegen(CodeGen& cg) {
    Function* fn = cg.Builder.GetInsertBlock()->getParent();

    Value* condVal = cond->codegen(cg);
    if (!condVal) return nullptr;

    Value* condBool = cg.Builder.CreateICmpNE(
        condVal, ConstantInt::get(Type::getInt32Ty(cg.Context), 0), "ifcond");

    BasicBlock* thenBB = BasicBlock::Create(cg.Context, "then", fn);
    BasicBlock* elseBB = BasicBlock::Create(cg.Context, "else", fn);
    BasicBlock* mergeBB = BasicBlock::Create(cg.Context, "ifcont", fn);

    cg.Builder.CreateCondBr(condBool, thenBB, elseBB);

    cg.Builder.SetInsertPoint(thenBB);
    for (auto& s : thenBody)
        s->codegen(cg);
    if (!cg.Builder.GetInsertBlock()->getTerminator())
        cg.Builder.CreateBr(mergeBB);

    cg.Builder.SetInsertPoint(elseBB);
    for (auto& s : elseBody)
        s->codegen(cg);
    if (!cg.Builder.GetInsertBlock()->getTerminator())
        cg.Builder.CreateBr(mergeBB);

    cg.Builder.SetInsertPoint(mergeBB);
    return ConstantInt::get(Type::getInt32Ty(cg.Context), 0);
}

Value* WhileStmt::codegen(CodeGen& cg) {
    Function* fn = cg.Builder.GetInsertBlock()->getParent();

    BasicBlock* loopCond = BasicBlock::Create(cg.Context, "whilecond", fn);
    BasicBlock* loopBody = BasicBlock::Create(cg.Context, "whilebody", fn);
    BasicBlock* loopEnd = BasicBlock::Create(cg.Context, "whileend", fn);

    cg.Builder.CreateBr(loopCond);

    cg.Builder.SetInsertPoint(loopCond);
    Value* condVal = cond->codegen(cg);
    if (!condVal) return nullptr;
    Value* condBool = cg.Builder.CreateICmpNE(
        condVal, ConstantInt::get(Type::getInt32Ty(cg.Context), 0), "whilecond");
    cg.Builder.CreateCondBr(condBool, loopBody, loopEnd);

    cg.Builder.SetInsertPoint(loopBody);
    for (auto& s : body)
        s->codegen(cg);
    if (!cg.Builder.GetInsertBlock()->getTerminator())
        cg.Builder.CreateBr(loopCond);

    cg.Builder.SetInsertPoint(loopEnd);
    return ConstantInt::get(Type::getInt32Ty(cg.Context), 0);
}

Value* ForStmt::codegen(CodeGen& cg) {
    Function* fn = cg.Builder.GetInsertBlock()->getParent();

    BasicBlock* loopCond = BasicBlock::Create(cg.Context, "forcond", fn);
    BasicBlock* loopBody = BasicBlock::Create(cg.Context, "forbody", fn);
    BasicBlock* loopEnd = BasicBlock::Create(cg.Context, "forend", fn);

    if (init)
        init->codegen(cg);

    cg.Builder.CreateBr(loopCond);

    cg.Builder.SetInsertPoint(loopCond);
    if (cond) {
        Value* condVal = cond->codegen(cg);
        if (!condVal) return nullptr;
        Value* condBool = cg.Builder.CreateICmpNE(
            condVal, ConstantInt::get(Type::getInt32Ty(cg.Context), 0), "forcond");
        cg.Builder.CreateCondBr(condBool, loopBody, loopEnd);
    } else {
        cg.Builder.CreateBr(loopBody);
    }

    cg.Builder.SetInsertPoint(loopBody);
    for (auto& s : body)
        s->codegen(cg);

    if (incr)
        incr->codegen(cg);

    if (!cg.Builder.GetInsertBlock()->getTerminator())
        cg.Builder.CreateBr(loopCond);

    cg.Builder.SetInsertPoint(loopEnd);
    return ConstantInt::get(Type::getInt32Ty(cg.Context), 0);
}

Value* ReturnStmt::codegen(CodeGen& cg) {
    if (value) {
        Value* val = value->codegen(cg);
        if (!val) return nullptr;
        return cg.Builder.CreateRet(val);
    }
    return cg.Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(cg.Context), 0));
}

Value* PrintStmt::codegen(CodeGen& cg) {
    Function* printfFn = cg.TheModule->getFunction("printf");
    if (!printfFn) return nullptr;

    Value* val = value->codegen(cg);
    if (!val) return nullptr;

    Value* fmtStr = cg.Builder.CreateGlobalStringPtr("%d\n", "fmt");
    return cg.Builder.CreateCall(printfFn, {fmtStr, val}, "printtmp");
}
