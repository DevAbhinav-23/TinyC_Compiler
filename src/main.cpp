#include "ast.h"
#include "codegen.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>

extern FILE* yyin;
extern int yyparse();
extern Program* g_program;

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.tiny>\n", argv[0]);
        return 1;
    }

    const char* inputFile = argv[1];
    yyin = fopen(inputFile, "r");
    if (!yyin) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", inputFile);
        return 1;
    }

    if (yyparse() != 0) {
        fprintf(stderr, "Error: Parsing failed\n");
        fclose(yyin);
        return 1;
    }
    fclose(yyin);

    if (!g_program) {
        fprintf(stderr, "Error: No program parsed\n");
        return 1;
    }

    CodeGen codegen;
    codegen.generate(*g_program);

    // Print IR to stderr
    std::string irStr;
    llvm::raw_string_ostream irStream(irStr);
    codegen.TheModule->print(irStream, nullptr);
    fprintf(stderr, "%s", irStr.c_str());

    // Initialize targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    llvm::Triple targetTriple(llvm::sys::getDefaultTargetTriple());
    codegen.TheModule->setTargetTriple(targetTriple);

    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple.getTriple(), error);
    if (!target) {
        fprintf(stderr, "Error: %s\n", error.c_str());
        return 1;
    }

    llvm::TargetOptions options;
    std::optional<llvm::Reloc::Model> rm;
    llvm::TargetMachine* targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", options, rm);

    std::string objFile = std::string(inputFile) + ".o";
    std::error_code ec;
    llvm::raw_fd_ostream objStream(objFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        fprintf(stderr, "Error: Could not open %s: %s\n", objFile.c_str(), ec.message().c_str());
        return 1;
    }

    llvm::legacy::PassManager passManager;
    if (targetMachine->addPassesToEmitFile(passManager, objStream, nullptr,
            llvm::CodeGenFileType::ObjectFile)) {
        fprintf(stderr, "Error: TargetMachine can't emit object file\n");
        return 1;
    }
    passManager.run(*codegen.TheModule);
    objStream.flush();
    fprintf(stderr, "Object file written to %s\n", objFile.c_str());

    std::string outExe = std::string(inputFile) + ".out";
    std::string linkCmd = "gcc -o " + outExe + " " + objFile + " -no-pie";
    fprintf(stderr, "Linking: %s\n", linkCmd.c_str());
    int ret = system(linkCmd.c_str());
    if (ret != 0) {
        fprintf(stderr, "Error: Linking failed\n");
        return 1;
    }

    fprintf(stderr, "Successfully compiled to %s\n", outExe.c_str());
    return 0;
}
