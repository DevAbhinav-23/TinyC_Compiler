# TinyC Compiler

A compiler for the **TinyC** language built from scratch using **LLVM**, **Flex**, and **Bison**.

**Author:** Abhinav V

---

## Overview

TinyC is a minimal C-like language with a clean, modern compiler pipeline. The compiler reads `.tiny` source files and produces native executables through LLVM's backend, with safety features like runtime division-by-zero checks and proper scope nesting.

### Key Highlights

- Full LLVM IR codegen with native object file emission
- Flex/Bison lexer and parser with LALR(1) grammar
- Short-circuit evaluation for `&&` and `||`
- Runtime division-by-zero detection with clear error messages
- Proper variable scope nesting in blocks (`if`, `while`, `for`)
- Custom output filename support via `-o` flag
- Safe linking via `fork()`/`exec()` (no shell injection)

---

## Project Structure

```
TinyC_Compiler/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── ISSUES_FIXED.md             # Code review: blocker issues (resolved)
├── FUTURE_IMPROVEMENTS.md      # Code review: suggestions & enhancements
├── src/
│   ├── ast.h                   # Abstract Syntax Tree node definitions
│   ├── lexer.l                 # Flex lexer specification
│   ├── parser.y                # Bison parser grammar
│   ├── codegen.h               # LLVM code generation header
│   ├── codegen.cpp             # LLVM code generation implementation
│   └── main.cpp                # Compiler driver (main entry point)
├── tests/
│   ├── test1.tiny              # Basic print test
│   ├── test2.tiny              # Variables and arithmetic
│   ├── test3.tiny              # Function calls
│   ├── test4.tiny              # While loop
│   ├── test5.tiny              # For loop and if/else
│   └── run_tests.sh            # Test runner script
└── build/                      # Build output (generated)
```

---

## Language Specification

### Supported Features

| Category   | Syntax                                     |
|-----------|--------------------------------------------|
| Types     | `int`                                      |
| Functions | `int name(int param1, int param2) { ... }` |
| Variables | `int x = 10;`                              |
| Arithmetic | `+`, `-`, `*`, `/`, `%`                   |
| Comparison | `==`, `!=`, `<`, `>`, `<=`, `>=`          |
| Boolean   | `&&`, `\|\|`, `!` (short-circuit evaluated) |
| Control   | `if / else`, `while`, `for`               |
| Return    | `return expr;`                             |
| Print     | `print(expr);`                             |
| Comments  | `// line comment`, `/* block comment */`   |

### Example Programs

**Hello World:**
```c
int main() {
    print(42);
    return 0;
}
```

**Fibonacci:**
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

**Factorial with For Loop:**
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

---

## Building

### Prerequisites

| Dependency             | Version  | Purpose                        |
|-----------------------|----------|--------------------------------|
| CMake                 | 3.20+    | Build system                   |
| LLVM development libs | 22       | IR generation, optimization, code emission |
| Flex                  | any      | Lexer generation               |
| Bison                 | any      | Parser generation              |
| GCC or Clang          | any      | C++ compilation, final linking |

### Build Steps

```bash
cd TinyC_Compiler
mkdir build && cd build
cmake ..
make
```

This produces the `tinyc` executable in the `build/` directory.

---

## Usage

### Basic Compilation

```bash
# Compile a TinyC source file
./build/tinyc tests/test2.tiny

# This will:
# 1. Parse the source into an AST
# 2. Generate LLVM IR (printed to stderr)
# 3. Emit a native object file (test2.tiny.o)
# 4. Link with gcc into an executable (test2.tiny.out)

# Run the compiled program
./tests/test2.tiny.out
# Output: 30
```

### Custom Output Name

```bash
# Compile with a custom output filename
./build/tinyc -o my_program tests/test3.tiny

# Run the custom-named executable
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

---

## Compiler Pipeline

```
Source Code (.tiny)
       |
       v
+-----------+
|   Lexer   |  Flex    Tokenizes source into keywords, identifiers,
+-----+-----+          numbers, operators, and punctuation
      |
      v
+-----------+
|   Parser  |  Bison   Parses token stream into an Abstract Syntax
+-----+-----+          Tree (AST) with LALR(1) grammar
      |
      v
+-----------+
|  Code Gen |  LLVM    Walks AST nodes and emits LLVM IR using
+-----+-----+          IRBuilder. Handles scope, short-circuit eval,
      |                 and runtime safety checks.
      v
+-----------+
|   LLVM    |  Backend Optimizes IR and emits native object code
+-----+-----+          for the target architecture
      |
      v
+-----------+
|  Linker   |  gcc     Links object file with libc to produce
+-----+-----+          a native executable
      |
      v
Executable (.out)
```

---

## Implementation Details

### Lexer (`src/lexer.l`)

Converts raw source text into a stream of tokens. Handles:
- Keywords: `int`, `return`, `if`, `else`, `while`, `for`, `print`
- Operators: arithmetic, comparison, logical
- Literals: integers
- Identifiers: variable and function names
- Whitespace, line comments (`//`), and block comments (`/* */`)

### Parser (`src/parser.y`)

LALR(1) grammar built with Bison that:
- Parses token streams into a typed AST
- Manages operator precedence via `%left`/`%right` declarations
- Builds the AST using raw pointers with ownership transferred to `std::unique_ptr`
- Reports syntax errors with line numbers

### AST (`src/ast.h`)

Tree representation of the parsed program:
- **Expressions:** `NumberExpr`, `VariableExpr`, `BinaryExpr`, `UnaryExpr`, `CallExpr`
- **Statements:** `ExprStmt`, `VarDeclStmt`, `AssignStmt`, `IfStmt`, `WhileStmt`, `ForStmt`, `ReturnStmt`, `PrintStmt`
- **Structure:** `FunctionAST`, `Program`
- Each node implements a virtual `codegen()` method that emits LLVM IR

### Code Generation (`src/codegen.cpp`)

Translates the AST into executable LLVM IR:
- Uses LLVM `IRBuilder` for instruction creation
- Maintains a scope stack (`NamedValuesStack`) for proper variable scoping
- Generates runtime checks for division/modulo by zero
- Implements short-circuit evaluation for `&&` and `||` using basic blocks
- Calls C's `printf` for the `print` statement
- Emits object files via LLVM's `TargetMachine`

### Compiler Driver (`src/main.cpp`)

The entry point that orchestrates the full pipeline:
- Parses command-line arguments (`-o` for custom output name)
- Runs the lexer/parser to build the AST
- Triggers LLVM IR generation
- Emits a native object file using LLVM's backend
- Links with `gcc` via `fork()`/`execvp()` (safe against shell injection)

---

## Safety Features

| Feature | Description |
|---------|-------------|
| Division by zero check | Runtime check that prints an error and aborts on `x / 0` or `x % 0` |
| Scope nesting | Variables declared in `if`/`while`/`for` blocks don't leak into outer scopes |
| Short-circuit evaluation | `&&` and `||` skip the RHS operand when the result is already determined |
| Safe linking | Uses `fork()`/`execvp()` instead of `system()` to prevent command injection |

---

## License

MIT License
