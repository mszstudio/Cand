# C& Language Specification (v0.1)

**Language Name:** C& (pronounced "C-and")  
**File Extension:** `.cand`  
**Motto:** *C& — The C you wish existed.*

---

## 1. Overview & Design Philosophy

C& is a modern, high-performance systems programming language derived in philosophy from C, but designed to be safer, cleaner, more practical, and memory-safe by default without sacrificing performance or control.

### Core Principles
- **No Garbage Collector (GC):** Memory management uses explicit ownership semantics (`owned<T>`, `borrow<T>`, `shared<T>`) alongside raw pointers (`*T`) when in `unsafe` blocks.
- **C-Compatible ABI & FFI:** Zero-cost interoperability with existing C libraries via `extern "C"`.
- **Modern Error Handling:** `Result<T, E>` and `Option<T>` with the `?` error-propagation operator instead of exceptions.
- **Native LLVM Backend:** Compiles directly to native binary executables via LLVM IR.
- **Built-in Toolchain:** Package management (`cand.toml`), formatting (`cand fmt`), testing (`cand test`), and diagnostics built into a single executable (`cand`).

---

## 2. Lexical Structure

### Source Encoding
C& source code is encoded in UTF-8.

### Comments
- **Line comment:** `// comment`
- **Block comment:** `/* comment */`

### Keywords
```text
fn        let       const     return    if        else
while     for       loop      break     continue  struct
enum      impl      trait     import    export    pub
unsafe    async     await     spawn     match     in
as        extern    comptime  test      mut       ref
true      false     null      self      type
```

### Identifiers
Identifiers start with a letter `[a-zA-Z]` or underscore `_`, followed by letters, digits `[0-9]`, or underscores.

---

## 3. Type System

### Primitive Types

| Type | Description | Size |
|---|---|---|
| `int` / `i32` | 32-bit signed integer | 4 bytes |
| `i8`, `i16`, `i32`, `i64` | Signed integers | 1, 2, 4, 8 bytes |
| `u8`, `u16`, `u32`, `u64` | Unsigned integers | 1, 2, 4, 8 bytes |
| `float` / `f64` | 64-bit IEEE 754 floating point | 8 bytes |
| `f32` | 32-bit floating point | 4 bytes |
| `bool` | Boolean (`true` / `false`) | 1 byte |
| `char` | Unicode scalar value | 4 bytes |
| `string` | UTF-8 string slice | 16 bytes |
| `void` | Empty / return unit type | 0 bytes |
| `usize` / `isize` | Pointer-sized integers | Architecture dependent |

---

## 4. Syntax & Basic Statements

### Variable Declarations
```c
let age: int = 16;
let price: float = 15.5;
let active: bool = true;

// Type inference
let x = 100;
let name = "Mohammed";

// Mutable variables
let mut count = 0;
count += 1;
```

### Constants
```c
const MAX_USERS: int = 1000;
```

### Functions
```c
fn add(a: int, b: int) -> int {
    return a + b;
}

fn main() {
    let message: string = "Hello C&";
    println(message);
}
```

---

## 5. Structs and Methods

```c
struct User {
    id: int;
    name: string;
    active: bool;
}

impl User {
    fn is_active(self) -> bool {
        return self.active;
    }
}
```

---

## 6. Memory Ownership Model

C& avoids garbage collection while guaranteeing memory safety through explicit ownership wrappers:

- `owned<T>`: Single-owner heap allocation (freed automatically when out of scope).
- `borrow<T>`: Non-owning borrowed reference.
- `shared<T>`: Reference-counted shared ownership.
- `weak<T>`: Weak pointer to break circular reference cycles.
- `*T` / `*mut T`: Raw C-style pointers allowed inside `unsafe { ... }`.

### Unsafe Code Block
```c
unsafe {
    let ptr: *int = &value;
    *ptr = 42;
}
```

---

## 7. Error Handling

C& avoids C++ / Java exceptions. Functions return `Result<T, E>` or `Option<T>`.

```c
fn read_file(path: string) -> Result<string, Error> {
    // ...
}

fn process() -> Result<void, Error> {
    let data = read_file("test.txt")?;
    println(data);
    return Ok();
}
```

---

## 8. Pattern Matching

```c
match result {
    Ok(value) => println(value),
    Err(error) => println(error),
}
```

---

## 9. C Interoperability (FFI)

C& provides direct foreign function interface with zero overhead:

```c
extern "C" {
    fn printf(format: *char, ...);
    fn malloc(size: usize) -> *void;
    fn free(ptr: *void);
}
```

---

## 10. Native Compilation Toolchain

Commands:
- `cand build <file.cand>`: Compiles to native binary executable via LLVM.
- `cand run <file.cand>`: Compiles and immediately executes.
- `cand test`: Runs built-in test blocks (`test "name" { ... }`).
- `cand doctor`: Audits LLVM toolchain, compiler health, and system environment.
