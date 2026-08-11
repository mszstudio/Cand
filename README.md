# C& Programming Language

> **C& — The C you wish existed.**  
> Developed and Maintained by **MSZ Studio**.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()

---

## 📌 Overview

C& (pronounced "C-and") is a modern, high-performance systems programming language derived in philosophy from C, but designed to be safer, cleaner, more practical, and memory-safe by default.

Developed by **MSZ Studio**, C& combines the raw speed of C with modern compiler diagnostics, clean syntax, explicit ownership semantics, and zero-cost LLVM native machine code generation.

---

## ✨ Key Features

- **Blazing Fast:** Compiles directly to native machine code via LLVM backend.
- **C-Like Syntax:** Familiar, clean, and concise syntax for systems programmers.
- **Memory Safety without GC:** Explicit ownership semantics (`owned<T>`, `borrow<T>`, `shared<T>`) and scoped pointers.
- **Zero-Cost C Interop (FFI):** Direct foreign function calls to existing C libraries via `extern "C"`.
- **Modern Error Handling:** `Result<T, E>` and `Option<T>` with `?` propagation operator.
- **Built-in Toolchain:** Package management (`cand.toml`), testing (`cand test`), linting (`cand lint`), formatting (`cand fmt`), and diagnostics (`cand doctor`).

---

## 🚀 Quick Start

### 1. Prerequisites
- [Rust toolchain](https://rustup.rs/) (v1.75+)
- [LLVM / Clang](https://llvm.org/) (v17+)

### 2. Build Compiler
```bash
cargo build --release
```

### 3. Compile & Run C& Program
Create `main.cand`:
```c
fn main() {
    println("Hello, C&!");
}
```

Run with `cand`:
```bash
cand run main.cand
```

Output:
```text
   Compiling main.cand
    Finished main.exe
     Running main.exe

Hello, C&!
```

---

## 🛠️ CLI Toolchain (`cand`)

```bash
cand build main.cand     # Build native executable binary (.exe)
cand run main.cand       # Build and run immediately
cand test                # Run built-in unit tests
cand fmt                 # Format C& source code
cand lint                # Static analysis & security checks
cand clean               # Clean build artifacts
cand doctor              # Audit system environment & LLVM installation
cand version             # Display compiler version details
```

---

## 📁 Repository Structure

```text
c-and/
├── Cargo.toml                    # Workspace manifest
├── cand.toml                     # C& project manifest
├── LICENSE                       # Official MIT License (MSZ Studio)
├── README.md                     # Main documentation
├── ARCHITECTURE.md               # Compiler architecture map
├── COMPILER.md                   # Compiler pipeline details
├── LANGUAGE.md                   # Language syntax guide
├── ROADMAP.md                    # Multi-phase project roadmap
├── CONTRIBUTING.md               # Guidelines for contributors
├── docs/
│   └── language-spec.md          # Full C& Language Specification
├── crates/
│   ├── cand-lexer/               # Tokenizer & lexical analyzer
│   ├── cand-ast/                 # AST data structures
│   ├── cand-parser/              # Recursive descent parser
│   ├── cand-diagnostics/         # Rich source-spanned error reporter
│   ├── cand-semantic/            # Semantic analyzer & type checker
│   ├── cand-types/               # Type system definitions
│   ├── cand-ir/                  # High-level IR
│   ├── cand-codegen/             # LLVM IR code generator
│   └── cand-driver/              # `cand` CLI binary driver
└── examples/
    └── hello-world/
        └── main.cand             # Integration test program
```

---

## 📄 License & Copyright

Copyright © 2026 **MSZ Studio**. All rights reserved.

Distributed under the [MIT License](LICENSE).
