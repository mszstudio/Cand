# C& Programming Language

> **C& — The C you wish existed.**  
> Developed and Maintained by **MSZ Studio**.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()
[![Philosophy](https://img.shields.io/badge/philosophy-C%2C%20Reconsidered-orange.svg)](PHILOSOPHY.md)

---

## 📌 Overview & Core Philosophy

**C&** (pronounced *"C-and"*) is an independent, high-performance systems programming language. It takes the essence of C systems programming — control, simplicity, closeness to the system, and predictable performance — and rebuilds the developer experience around it with modern rules and unified tooling.

Read the full [C& Manifesto & Philosophy](PHILOSOPHY.md) (`PHILOSOPHY.md`).

> **Not C made bigger.**  
> **Not Rust made simpler.**  
> **Not C++ redesigned.**  
> **C, reconsidered.**

---

## ✨ Key Differentiators

- **C-Level Control + Modern Syntax:** Expressive syntax (`let`, `mut`, `fn`) with direct system-level memory control.
- **Rust/Cargo-Independent:** Self-contained native C compiler (`cand.exe`) written in pure C.
- **Predictable Performance:** No mandatory garbage collection or heavy runtime VM overhead.
- **Zero-Cost C Interoperability:** Uses standard C ABI for seamless integration with existing C libraries (SQLite, Raylib, OpenSSL).
- **Unified Tooling (`cand` CLI):** Integrated commands for build, run, doctor, formatting, linting, and diagnostics.

---

## 🚀 Quick Start

### 1. Build Compiler (`cand.exe`)
Requirements: Any standard C compiler (Clang, GCC, or MSVC).

```bash
# On Windows
build.bat

# On Linux / macOS
make
```

### 2. Check Toolchain Status
```bash
cand doctor
```
Output:
```text
=== C& Compiler Toolchain Doctor ===
  [✓] C& Native Compiler: v1.0.0 (Independent Native Compiler) (Independent)
  [✓] Language Autonomy: 100% Standalone (No Rust, No External Apps)
  [✓] Build Engine: Native Standalone Executable Generator

System is fully independent and ready for C& development!
```

### 3. Compile & Run C& Program
Create `main.cand`:
```c
fn main() {
    println("Hello, C&!");
}
```

Compile & Run with `cand`:
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
cand run main.cand       # Build and execute immediately
cand doctor              # Audit system environment & compiler toolchain
cand version             # Display compiler version details
cand clean               # Clean build artifacts
cand fmt                 # Format C& source code
cand lint                # Static analysis & checks
```

---

## 📁 Repository Structure

```text
c-and/
├── build.bat                     # Windows Native Batch build script
├── Makefile                      # Cross-platform Makefile
├── cand.toml                     # C& project manifest
├── LICENSE                       # Official MIT License (MSZ Studio)
├── README.md                     # Main documentation
├── PHILOSOPHY.md                 # Official C& Manifesto & 10 Principles
├── ARCHITECTURE.md               # Compiler architecture map
├── COMPILER.md                   # Compiler pipeline details
├── LANGUAGE.md                   # Language syntax guide
├── ROADMAP.md                    # Multi-phase project roadmap
├── src/                          # Pure C Compiler Source Code
│   ├── cand_compiler.h           # Unified compiler header
│   ├── cand_lexer.c              # Handcrafted Lexical Analyzer
│   ├── cand_parser.c             # Recursive Descent Parser (AST)
│   ├── cand_semantic.c           # Semantic Analyzer & Type Checker
│   ├── cand_codegen.c            # Code Generator & C-Backend
│   └── cand_driver.c             # `cand` CLI Driver binary entrypoint
└── examples/
    └── hello-world/
        └── main.cand             # Integration test program
```

---

## 📄 License & Copyright

Copyright © 2026 **MSZ Studio**. All rights reserved.

Distributed under the [MIT License](LICENSE).
