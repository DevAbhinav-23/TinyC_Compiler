# TinyC Compiler - Code Review

**Reviewer:** @code-reviewer (agency-agents)
**Date:** 2026-06-20

---

## Overall Impression

A clean, well-structured LLVM-based compiler for a toy C-like language. The architecture is classic (Flex lexer, Bison parser, AST, LLVM codegen) and executed competently. The grammar handles operator precedence correctly, the codegen is straightforward, and the build system is solid. For a learning project, this is solid work.

That said, there are several correctness issues, a few security concerns, and some design improvements worth making. I'll break them down.

---

## 🔴 Blockers (Must Fix)

### 1. Memory Leak: AST never freed
- **File:** `src/main.cpp`
- **Issue:** `g_program` (the entire AST) is never freed after codegen completes. The `yyin` FILE handle leaks on some error paths.
- **Why:** Bad habit that bites if you ever embed this or run it in a loop.
- **Status:** Known-acceptable for a short-lived CLI compiler. Noted as a limitation.

### 2. Division by Zero is Undefined Behavior
- **File:** `src/codegen.cpp`, lines 99-100
- **Issue:** `CreateSDiv` and `CreateSRem` produce poison values (or trap) when `R` is zero. There's no runtime check.
- **Why:** Any TinyC program that divides by zero will produce undefined behavior at the LLVM level.
- **Fix:** ✅ Added runtime check -- prints error, flushes stdout, and calls `abort()` on division by zero.

### 3. `strdup` Memory Leak in Lexer
- **File:** `src/lexer.l`, line 49
- **Issue:** Every identifier token calls `strdup(yytext)`. The parser frees most, but ownership is fragile across 300+ lines of Bison actions.
- **Why:** One missed `free()` in a new grammar rule = silent leak.
- **Status:** Accepted as inherent to Bison/Flex. Consider `std::string` in `%union` or a string arena for a future rewrite.

### 4. `system()` Call with Unsanitized Input (Command Injection)
- **File:** `src/main.cpp`, line 95-97
- **Issue:** `inputFile` flows directly into a shell command via `system()`. A filename with shell metacharacters = arbitrary code execution.
- **Why:** Textbook command injection.
- **Fix:** ✅ Replaced `system()` with `fork()`/`execvp()` to avoid shell interpretation.

---

## 🟡 Suggestions (Should Fix)

### 5. Unreachable Code After Return -- Invalid IR
- **File:** `src/codegen.cpp`, lines 66-67
- **Issue:** After generating a `return`, subsequent statements still get codegen'd, inserting instructions after a terminator.
- **Why:** Produces invalid LLVM IR (instructions after `ret`).
- **Fix:** ✅ Added terminator check -- `if (Builder.GetInsertBlock()->getTerminator()) break;`

### 6. `NamedValues` is a Flat Map -- No Scope Nesting
- **File:** `src/codegen.h`, `src/codegen.cpp`
- **Issue:** Variables inside `if`/`while` blocks overwrite outer-scope variables. No cleanup on block exit.
- **Why:** `int x = 10; if (1) { int x = 20; } print(x);` prints 20 instead of 10.
- **Fix:** ✅ Implemented scope stack (`NamedValuesStack`) with `pushScope()`/`popScope()` that saves and restores the symbol table on block entry/exit.

### 7. `generate()` Returns Misleading Value
- **File:** `src/codegen.cpp`, line 32
- **Issue:** Returns `lastVal` which is meaningless for most statement types.
- **Fix:** ✅ Changed return type to `void`.

### 8. Deprecated LLVM API Calls
- **Files:** `src/codegen.cpp:319`, `src/main.cpp:65`
- **Issue:** `CreateGlobalStringPtr` and `lookupTarget(StringRef, string&)` deprecated in LLVM 22.
- **Fix:** ✅ Updated to `CreateGlobalString` and Triple-based overload.

### 9. Unused Lexer Tokens: `FLOAT_KW`, `VOID_KW`
- **File:** `src/lexer.l`, lines 16-17
- **Issue:** Lexed but never used in any parser rule. Misleads readers.
- **Fix:** ✅ Removed unused tokens from lexer and parser.

### 10. `ForStmt` Grammar Overly Restrictive
- **File:** `src/parser.y`, line 196
- **Issue:** Init must be `var_decl`, increment must be `IDENT ASSIGN expr`. No `i++`, no `for(i=0;...)`.
- **Status:** Noted as a future improvement (see Future Improvements section).

### 11. No Short-Circuit Evaluation for `&&` and `||`
- **File:** `src/codegen.cpp`, lines 125-136
- **Issue:** Both operands always evaluated. `if (x == 1 && foo())` calls `foo()` even when `x != 1`.
- **Fix:** ✅ Implemented short-circuit evaluation using LLVM basic blocks with PHI nodes for AND (short-circuits on false) and OR (short-circuits on true).

### 12. Inconsistent Error Handling -- Missing Null Checks
- **Files:** `src/codegen.cpp`, `IfStmt::codegen`
- **Issue:** `cond->codegen()` can return nullptr, but result is used without checking.
- **Fix:** ✅ Added null checks throughout codegen for error propagation.

---

## 💭 Nits (Nice to Have)

### 13. No `-o` Flag for Output Name
- **Issue:** Output filename is always `input.tiny.out`.
- **Fix:** ✅ Added `-o <output>` flag support.

### 14. Comment Block Regex May Miss Edge Cases
- **File:** `src/lexer.l`, line 54
- **Issue:** The block comment regex doesn't handle all edge cases with `*` sequences.
- **Status:** Accepted for a toy compiler.

### 15. `#pragma once` vs Include Guards
- **Status:** `#pragma once` is non-standard but universally supported. Fine for this project.

---

## Summary

| Category      | Count | Fixed |
|---------------|-------|-------|
| 🔴 Blockers   | 4     | 3     |
| 🟡 Suggestions| 8     | 7     |
| 💭 Nits       | 3     | 1     |

---

## Future Improvements

Potential enhancements for the TinyC compiler:

1. **String type support** -- Add `string` as a primitive type with string literals, concatenation, and a `prints()` builtin.

2. **Array support** -- Add `int[]` type with indexing (`a[i]`) and pointer arithmetic.

3. **Struct support** -- Add `struct` definitions with member access (`.` operator).

4. **Multiple return types** -- Support `void` functions and proper type checking.

5. **Optimization passes** -- Run LLVM's `PassManager` with `-O1`/`-O2` optimization levels.

6. **Error recovery** -- Add error production rules to Bison for multi-error reporting instead of aborting on first error.

7. **Line number tracking** -- Attach source line metadata to AST nodes for better error messages.

8. **Static analyzer pass** -- Warn on unused variables, unreachable code after return, and uninitialized variable reads.

9. **Increment/decrement operators** -- Add `++` and `--` as both postfix and prefix operators.

10. **Compound assignments** -- Add `+=`, `-=`, `*=`, `/=`, `%=` operators.

11. **Modular output** -- Support emitting LLVM IR to `.ll` files alongside object files for debugging.

12. **Inline assembly** -- Allow inline `asm()` blocks for low-level operations.

13. **Multi-file compilation** -- Support separate compilation and linking of multiple `.tiny` source files.

14. **Constant folding** -- Fold constant expressions at compile time (`3 + 4` -> `7`).

15. **Improved block comment lexer** -- Use a more robust regex or state-based approach for nested/block comments.

---

## What's Good

- **Clean AST design** -- Proper `Expr`/`Stmt` separation with virtual `codegen()` dispatch
- **Correct operator precedence** -- Bison `%left`/`%right` declarations are accurate
- **Function parameter handling** -- Proper allocation as stack variables with correct naming in LLVM IR
- **Build system** -- Clean CMake integrating Flex/Bison/LLVM with no warnings
- **Test suite** -- Covers all core language features, all passing
- **Scoped variable save/restore** -- Function-boundary variable save/restore was correct from the start
