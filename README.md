# TinyC Compiler

A compiler for the **TinyC** language built from scratch using **LLVM**, **Flex**, and **Bison**.

## Author

**Abhinav V**

## Project Structure

```
TinyC_Compiler/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── src/
│   ├── ast.h               # Abstract Syntax Tree node definitions
│   ├── lexer.l             # Flex lexer specification
│   ├── parser.y            # Bison parser grammar
│   ├── codegen.h           # LLVM code generation header
│   ├── codegen.cpp         # LLVM code generation implementation
│   └── main.cpp            # Compiler driver (main entry point)
└── tests/
    ├── test1.tiny           # Basic print test
    ├── test2.tiny           # Variables and arithmetic
    ├── test3.tiny           # Function calls
    ├── test4.tiny           # While loop
    ├── test5.tiny           # For loop and if/else
    └── run_tests.sh         # Test runner script
```

## TinyC Language Specification

### Supported Features

| Category     | Syntax                                  |
|-------------|-----------------------------------------|
| Types       | `int`                                   |
| Functions   | `int name(int param1, int param2) { }` |
| Variables   | `int x = 10;`                           |
| Arithmetic  | `+`, `-`, `*`, `/`, `%`                |
| Comparison  | `==`, `!=`, `<`, `>`, `<=`, `>=`       |
| Boolean     | `&&`, `\|\|`, `!`                       |
| Control     | `if / else`, `while`, `for`            |
| Return      | `return expr;`                          |
| Print       | `print(expr);`                          |
| Comments    | `// line comment`, `/* block */`        |

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

## Building

### Prerequisites

- CMake 3.20+
- LLVM development libraries (tested with LLVM 22)
- Flex
- Bison
- GCC or Clang

### Build Steps

```bash
cd TinyC_Compiler
mkdir build && cd build
cmake ..
make
```

This produces the `tinyc` executable in the `build/` directory.

## Usage

### Compile and Run

```bash
# Compile a TinyC source file
./build/tinyc tests/test2.tiny

# This will:
# 1. Generate LLVM IR (test2.tiny.ll)
# 2. Generate object file (test2.tiny.o)
# 3. Link into executable (test2.tiny.out)

# Run the compiled program
./tests/test2.tiny.out
```

### Run All Tests

```bash
chmod +x tests/run_tests.sh
./tests/run_tests.sh
```

## Compiler Pipeline

```
Source Code (.tiny)
    │
    ▼
┌─────────┐
│  Lexer  │  (Flex) - Tokenization
└────┬────┘
     │ Tokens
     ▼
┌─────────┐
│ Parser  │  (Bison) - Parse tokens into AST
└────┬────┘
     │ AST
     ▼
┌─────────────┐
│ Code Gen    │  (LLVM API) - Generate LLVM IR
└────┬────────┘
     │ LLVM IR (.ll)
     ▼
┌─────────────┐
│ LLVM        │  Optimization, Object code emission
│ Backend     │
└────┬────────┘
     │ Object file (.o)
     ▼
┌─────────────┐
│ Linker      │  (gcc) - Link into executable
└────┬────────┘
     │
     ▼
 Executable (.out)
```

## Implementation Details

### Lexer (lexer.l)
- Converts source text into tokens (keywords, identifiers, numbers, operators)
- Handles whitespace, line comments (`//`), and block comments (`/* */`)

### Parser (parser.y)
- LALR(1) grammar using Bison
- Builds an AST using raw pointers with manual ownership via `std::unique_ptr`
- Operator precedence is handled with Bison precedence rules

### AST (ast.h)
- Expression nodes: `NumberExpr`, `VariableExpr`, `BinaryExpr`, `UnaryExpr`, `CallExpr`
- Statement nodes: `ExprStmt`, `VarDeclStmt`, `AssignStmt`, `IfStmt`, `WhileStmt`, `ForStmt`, `ReturnStmt`, `PrintStmt`
- Function/Program nodes: `FunctionAST`, `Program`
- Each node has a virtual `codegen()` method for LLVM IR generation

### Code Generation (codegen.cpp)
- Uses LLVM's `IRBuilder` for creating instructions
- Symbol table maps variable names to `alloca` instructions
- Supports all TinyC language constructs
- Calls C's `printf` for the `print` statement
- Emits object files via LLVM's `TargetMachine`

### Compiler Driver (main.cpp)
- Reads source file, runs the lexer/parser pipeline
- Generates LLVM IR and prints it to a `.ll` file
- Emits a native object file using LLVM's backend
- Links with `gcc` to produce a final executable
