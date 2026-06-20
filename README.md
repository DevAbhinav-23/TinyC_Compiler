<h1 align="center">TinyC Compiler</h1>

<p align="center">
  <em>A from-scratch compiler for a minimal C-like language, powered by LLVM, Flex, and Bison</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus" alt="C++17">
  <img src="https://img.shields.io/badge/LLVM-22-purple?logo=llvm" alt="LLVM 22">
  <img src="https://img.shields.io/badge/Flex-Bison-red" alt="Flex/Bison">
  <img src="https://img.shields.io/badge/License-MIT-green" alt="MIT License">
</p>

---

**Author:** Abhinav Venkata K

## What is TinyC?

TinyC is a compact, C-inspired programming language that supports the core building blocks of imperative programming: integer variables, arithmetic, functions, conditionals, and loops. The TinyC Compiler takes `.tiny` source files and compiles them down to **native machine executables** through LLVM's optimization and code generation backend.

The project demonstrates how a real compiler is structured -- from tokenizing raw text, to parsing grammar rules, to building an abstract syntax tree, and finally generating LLVM IR that gets optimized and emitted as native object code. Everything is built from the ground up using established compiler infrastructure (Flex for lexing, Bison for parsing, LLVM for codegen).

## Compiler Pipeline

The compiler processes source code through a clean sequence of stages:

```
  .tiny Source File
        |
        v
  ┌─────────────┐
  │    Lexer     │   Flex-generated tokenizer
  │              │   Breaks raw text into keywords, identifiers,
  │              │   numbers, operators, and punctuation
  └──────┬──────┘
         | tokens
         v
  ┌─────────────┐
  │    Parser    │   Bison-generated LALR(1) parser
  │              │   Consumes tokens and builds a typed
  │              │   Abstract Syntax Tree (AST)
  └──────┬──────┘
         | AST
         v
  ┌─────────────┐
  │  Code Gen    │   Walks AST nodes and emits LLVM IR
  │              │   using LLVM's IRBuilder API
  │              │   Handles scoping, short-circuit eval,
  │              │   and runtime safety checks
  └──────┬──────┘
         | LLVM IR
         v
  ┌─────────────┐
  │ LLVM Backend │   TargetMachine optimizes IR and emits
  │              │   native object code for the host CPU
  └──────┬──────┘
         | .o file
         v
  ┌─────────────┐
  │   Linker     │   gcc links the object file with libc
  │              │   via fork()/execvp() (no shell injection)
  └──────┬──────┘
         v
    Native Executable (.out)
```

## Key Features

- **Full LLVM codegen** -- Compiles down to real native executables, not interpreted. The compiler produces `.o` object files via LLVM's `TargetMachine` and links them with GCC.
- **Short-circuit evaluation** -- Logical `&&` and `||` operators skip the right-hand side when the result is already determined, implemented with LLVM basic blocks and PHI nodes.
- **Proper scope nesting** -- Variables declared inside `if`, `while`, or `for` blocks are cleaned up when the block exits. Inner scope shadowing works correctly.
- **Runtime safety checks** -- Division and modulo by zero are caught at runtime with a clear error message and `abort()`, rather than causing a segfault.
- **Custom output names** -- Use the `-o` flag to control the output executable name instead of always defaulting to `input.tiny.out`.
- **Safe external linking** -- Uses `fork()`/`execvp()` to invoke the linker, avoiding shell injection vulnerabilities that `system()` would introduce.
- **Comment support** -- Single-line (`//`) and block (`/* */`) comments are properly recognized and ignored by the lexer.

## Language Specification

The TinyC language supports the following constructs:

| Category | Syntax | Example |
|----------|--------|---------|
| Types | `int` (32-bit signed) | `int x = 42;` |
| Functions | `int name(int p1, int p2) { ... }` | `int add(int a, int b) { return a + b; }` |
| Arithmetic | `+`, `-`, `*`, `/`, `%` | `x + y * 2` |
| Comparison | `==`, `!=`, `<`, `>`, `<=`, `>=` | `if (x < 10) { ... }` |
| Boolean | `&&`, `\|\|`, `!` (short-circuit) | `if (a > 0 && b > 0) { ... }` |
| Control Flow | `if / else`, `while`, `for` | `for (int i = 0; i < 10; i = i + 1) { ... }` |
| Return | `return expr;` | `return result;` |
| Print | `print(expr);` | `print(x + y);` |
| Comments | `// line` and `/* block */` | `// this is a comment` |

### Example: Hello World

```c
int main() {
    print(42);
    return 0;
}
```

### Example: Fibonacci Sequence

```c
int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int i = 0;
    while (i < 10) {
        print(fib(i));
        i = i + 1;
    }
    return 0;
}
```

### Example: Factorial with For Loop

```c
int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; i = i + 1) {
        result = result * i;
    }
    return result;
}

int main() {
    print(factorial(5));
    return 0;
}
```

## Project Structure

```
TinyC_Compiler/
├── CMakeLists.txt              Build configuration (CMake 3.20+, links LLVM)
├── README.md                   This file
├── ISSUES_FIXED.md             Code review: resolved blocker issues
├── FUTURE_IMPROVEMENTS.md      Code review: suggestions and roadmap
├── src/
│   ├── lexer.l                 Flex lexer specification
│   │                           Tokenizes source into keywords, operators,
│   │                           identifiers, numbers, and punctuation
│   ├── parser.y                Bison LALR(1) grammar
│   │                           Parses token streams into a typed AST with
│   │                           proper operator precedence
│   ├── ast.h                   AST node definitions
│   │                           Expression nodes (Number, Variable, Binary,
│   │                           Unary, Call) and statement nodes (VarDecl,
│   │                           Assign, If, While, For, Return, Print)
│   ├── codegen.h               LLVM code generation header
│   │                           CodeGen class with IRBuilder, scope stack,
│   │                           and function registry
│   ├── codegen.cpp             LLVM IR code generation implementation
│   │                           Walks the AST and emits LLVM IR with scope
│   │                           management, short-circuit eval, and safety checks
│   └── main.cpp                Compiler driver
│                               Parses CLI args, orchestrates the full pipeline,
│                               emits object files, and invokes the linker
├── tests/
│   ├── test1.tiny              Basic print
│   ├── test2.tiny              Variables and arithmetic
│   ├── test3.tiny              Function calls
│   ├── test4.tiny              While loop
│   ├── test5.tiny              For loop and if/else
│   └── run_tests.sh            Automated test runner
└── build/                      Build output (generated)
```

## Implementation Details

### Lexer (`src/lexer.l`)

The Flex-based lexer converts raw source text into a stream of typed tokens. It recognizes:
- **Keywords:** `int`, `return`, `if`, `else`, `while`, `for`, `print`
- **Operators:** arithmetic (`+`, `-`, `*`, `/`, `%`), comparison (`==`, `!=`, `<`, `>`, `<=`, `>=`), logical (`&&`, `||`, `!`), assignment (`=`)
- **Literals:** integer constants
- **Identifiers:** variable and function names (`[a-zA-Z_][a-zA-Z0-9_]*`)
- **Whitespace and comments:** spaces, tabs, newlines, `//` line comments, and `/* */` block comments

### Parser (`src/parser.y`)

The Bison-generated LALR(1) grammar builds a typed Abstract Syntax Tree from the token stream. It manages operator precedence through `%left`/`%right` declarations and handles the dangling-else ambiguity with `%prec LOWER_THAN_ELSE`. Ownership of AST nodes is transferred from raw pointers to `std::unique_ptr` for safe memory management.

### AST (`src/ast.h`)

The tree representation of a parsed program, with each node implementing a virtual `codegen()` method:
- **Expressions:** `NumberExpr`, `VariableExpr`, `BinaryExpr`, `UnaryExpr`, `CallExpr`
- **Statements:** `ExprStmt`, `VarDeclStmt`, `AssignStmt`, `IfStmt`, `WhileStmt`, `ForStmt`, `ReturnStmt`, `PrintStmt`
- **Structure:** `FunctionAST` (name, params, body), `Program` (list of functions)

### Code Generation (`src/codegen.cpp`)

The `CodeGen` class walks the AST and emits LLVM IR using LLVM's `IRBuilder`. Key aspects:
- **Scope management:** A `NamedValuesStack` saves/restores the symbol table on block entry/exit for proper variable scoping
- **Short-circuit evaluation:** `&&` and `||` use separate LLVM basic blocks with PHI nodes to skip the RHS when the result is already determined
- **Runtime checks:** Division and modulo by zero generate conditional branches that call `abort()` on detection
- **External functions:** `printf`, `fflush`, and `abort` are declared as external LLVM functions for `print` statements

## Building

### Prerequisites

| Dependency | Version | Purpose |
|------------|---------|---------|
| CMake | 3.20+ | Build system |
| LLVM development libs | 22 | IR generation, optimization, and native code emission |
| Flex | any | Lexer generation from `.l` specification |
| Bison | any | Parser generation from `.y` grammar |
| GCC or Clang | any | C++ compilation and final linking |

### Build Steps

```bash
cd TinyC_Compiler
mkdir build && cd build
cmake ..
make
```

This produces the `tinyc` executable in the `build/` directory.

## Usage

### Basic Compilation

```bash
# Compile a .tiny source file (prints LLVM IR to stderr)
./build/tinyc tests/test2.tiny

# Run the compiled executable
./tests/test2.tiny.out
# Output: 30
```

### Custom Output Name

```bash
# Specify the output executable name with -o
./build/tinyc -o my_program tests/test3.tiny

./my_program
# Output: 7
```

### Run All Tests

```bash
chmod +x tests/run_tests.sh
./tests/run_tests.sh

# Expected output:
# === TinyC Compiler Test Suite ===
# Running test1      ... PASS  (output: 42)
# Running test2      ... PASS  (output: 30)
# Running test3      ... PASS  (output: 7)
# Running test4      ... PASS  (output: 0 1 2 3 4)
# Running test5      ... PASS  (output: 0 1 2 3 4 1)
# Results: 5 passed, 0 failed out of 5 tests
```

## Safety Features

| Feature | What It Does |
|---------|-------------|
| Division by zero check | Generates a runtime check that prints a clear error and calls `abort()` on `x / 0` or `x % 0` |
| Scope nesting | Variables declared in `if`/`while`/`for` blocks don't leak into outer scopes |
| Short-circuit evaluation | `&&` and `||` skip the RHS operand when the result is already determined |
| Safe linking | Uses `fork()`/`execvp()` instead of `system()` to prevent command injection |
| Terminators check | Stops codegen if a basic block already has a terminator (after `return`) to produce valid LLVM IR |

## Comparison with TinyC_MLIR

This project uses a **direct AST-to-LLVM-IR** approach. For an alternative implementation that uses MLIR as an intermediate layer with its own optimization passes, see [TinyC_MLIR](../TinyC_MLIR).

| Aspect | TinyC_Compiler | TinyC_MLIR |
|--------|---------------|------------|
| Pipeline | AST -> LLVM IR directly | AST -> MLIR -> LLVM IR |
| Optimization | LLVM backend only | MLIR passes + LLVM backend |
| Variable scoping | Manual scope stack | MLIR's memref model |
| Control flow | Manual basic blocks | SCF dialect (structured) |
| Extensibility | Single-level IR | Multi-level (add new dialects) |
| Pass infrastructure | `llvm::legacy::PassManager` | `mlir::PassManager` |

## License

MIT License
