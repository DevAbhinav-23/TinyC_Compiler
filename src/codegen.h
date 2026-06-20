#pragma once

#include "ast.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include <map>
#include <vector>
#include <string>
#include <memory>

class CodeGen {
public:
    llvm::LLVMContext Context;
    std::unique_ptr<llvm::Module> TheModule;
    llvm::IRBuilder<> Builder;
    std::map<std::string, llvm::Value*> NamedValues;
    std::vector<std::map<std::string, llvm::Value*>> NamedValuesStack;
    std::map<std::string, llvm::Function*> Functions;

    CodeGen();

    void generate(Program& program);
    llvm::Function* getFunction(const std::string& name);
    void putVariable(const std::string& name, llvm::Value* val);
    llvm::Value* getVariable(const std::string& name);
    void pushScope();
    void popScope();
};
