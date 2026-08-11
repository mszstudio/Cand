# C& Language Overview & Syntax Guide

C& brings modern ergonomics to low-level systems programming.

## Variables & Constants

```c
let age: int = 16;
let price: float = 15.5;
let active: bool = true;

// Type inference
let x = 100;
let name = "Mohammed";

// Mutable variables
let mut counter = 0;
counter += 1;

// Constants
const MAX_USERS: int = 1000;
```

## Functions

```c
fn add(a: int, b: int) -> int {
    return a + b;
}

fn main() {
    let msg = "Hello C&";
    println(msg);
}
```

## Structs and Implementation

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

## Memory Ownership & Unsafe Code

```c
// Safe pointers & references
let ptr: &int = &value;

// Unsafe raw pointers
unsafe {
    let raw_ptr: *int = &value;
    *raw_ptr = 42;
}
```

## C FFI Interoperability

```c
extern "C" {
    fn printf(format: *char, ...);
}
```
