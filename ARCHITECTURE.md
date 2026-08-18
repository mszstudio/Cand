# C& Compiler Architecture Map

> **Native C Implementation — Independent Toolchain**

```text
C& Source Code (.cand)
         │
         ▼
 ┌──────────────┐
 │  cand_lexer  │  Lexical Analyzer (Tokens)
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │  cand_parser │  Recursive Descent Parser (AST Nodes)
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │cand_semantic │  Semantic Analyzer & Type Validation
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │ cand_codegen │  Optimized C Representation & C-Backend
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │ System C     │  Clang / GCC / MSVC System Compiler
 └──────┬───────┘
        │
        ▼
Native Binary (.exe)
```

## Core Compiler Subsystems (`src/`)

1. **`cand_compiler.h`**: Primary header defining core structures (`Token`, `ASTNode`, `Lexer`, `Parser`).
2. **`cand_lexer.c`**: Efficient lexer parsing C& keywords (`fn`, `let`, `mut`, `if`, `return`, `println`).
3. **`cand_parser.c`**: Recursive descent parser emitting abstract syntax trees with zero memory leaks.
4. **`cand_semantic.c`**: Type validator ensuring expression and function signature consistency.
5. **`cand_codegen.c`**: Emits high-level C runtime code and invokes host compiler to generate native binaries.
6. **`cand_driver.c`**: CLI driver managing `cand build`, `run`, `doctor`, `version`, `clean`, `fmt`, and `lint`.
