# TinyC Compiler - Future Improvements

**Reviewer:** @code-reviewer (agency-agents)
**Date:** 2026-06-20

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
- **Status:** Noted as a future improvement (see Enhancement Ideas section).

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
| 🟡 Suggestions| 8     | 7     |
| 💭 Nits       | 3     | 1     |

---

## Enhancement Ideas

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
