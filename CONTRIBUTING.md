# Contributing to C&

We welcome contributions to the C& programming language compiler!

## Development Setup

1. Clone the repository.
2. Install Rust (`rustup`) and LLVM (`clang`).
3. Run compiler tests:
```bash
cargo test
```
4. Build a C& test program:
```bash
cargo run --bin cand -- run examples/hello-world/main.cand
```
