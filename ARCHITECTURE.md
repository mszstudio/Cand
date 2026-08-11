# C& Compiler Architecture

```text
C& Source (.cand)
    │
    ▼
[ cand-lexer ] ─────► Stream of Tokens
    │
    ▼
[ cand-parser ] ────► Abstract Syntax Tree (AST)
    │
    ▼
[ cand-semantic ] ──► Type Checker & Name Resolution
    │
    ▼
[ cand-codegen ] ───► LLVM IR (.ll)
    │
    ▼
[ LLVM / Clang ] ───► Native Binary Executable (.exe / ELF)
```

## Modular Crates Structure

- `cand-lexer`: Tokenization and lexical analysis.
- `cand-ast`: AST node definitions.
- `cand-parser`: Recursive descent parser.
- `cand-diagnostics`: Rich compiler diagnostic reporting with source spans.
- `cand-semantic`: Type checker and symbol table analyzer.
- `cand-types`: C& type representation and type checking logic.
- `cand-ir`: High-level C& Intermediate Representation.
- `cand-codegen`: Translates AST to structured LLVM IR.
- `cand-driver`: CLI runner (`cand build`, `cand run`, `cand doctor`, etc.).
