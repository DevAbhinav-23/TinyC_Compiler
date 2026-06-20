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
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

extern FILE* yyin;
extern int yyparse();
extern Program* g_program;

void printUsage(const char* progName) {
    fprintf(stderr, "Usage: %s [-o output] <input.tiny>\n", progName);
}

int main(int argc, char** argv) {
    const char* outputFile = nullptr;
    const char* inputFile = nullptr;

    // Parse -o flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
            outputFile = argv[++i];
        } else if (argv[i][0] != '-') {
            inputFile = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }

    if (!inputFile) {
        printUsage(argv[0]);
        return 1;
    }

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
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeAsmParser();
    llvm::InitializeNativeAsmPrinter();

    llvm::Triple targetTriple(llvm::sys::getDefaultTargetTriple());
    codegen.TheModule->setTargetTriple(targetTriple);

    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
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

    // Determine output executable name
    std::string outExe;
    if (outputFile) {
        outExe = outputFile;
    } else {
        outExe = std::string(inputFile) + ".out";
    }

    // Link using fork/execvp (safe against shell injection)
    fprintf(stderr, "Linking: gcc -o %s %s -no-pie\n", outExe.c_str(), objFile.c_str());

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Error: fork failed\n");
        return 1;
    }

    if (pid == 0) {
        // Child process: exec gcc
        execlp("gcc", "gcc", "-o", outExe.c_str(), objFile.c_str(), "-no-pie", nullptr);
        // If exec fails
        fprintf(stderr, "Error: exec gcc failed\n");
        _exit(1);
    }

    // Parent process: wait for gcc
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Error: Linking failed\n");
        return 1;
    }

    fprintf(stderr, "Successfully compiled to %s\n", outExe.c_str());
    return 0;
}
