# C& Compiler Internals (`candc`)

## Compilation Pipeline Details

1. **Lexical Analysis (`cand-lexer`):** Reads UTF-8 source files, producing line/column spans and token streams.
2. **Parsing (`cand-parser`):** Builds an immutable AST representation.
3. **Semantic Analysis (`cand-semantic`):** Validates types and scope resolution.
4. **Code Generation (`cand-codegen`):** Generates SSA-form LLVM IR targeting native architectures.
5. **Native Linker (`cand-driver`):** Calls `clang` backend to link object files with standard system C libraries (`libc` / MSVC UCRT).
