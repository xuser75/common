# Common Programming Language

A minimalist, dependency-free compiler for a C-like language that targets x86-32 assembly.

## Overview

Common is a statically-typed systems programming language with:
- **No runtime dependencies** - compiles to standalone executables
- **Direct C interoperability** - call and be called by C code  
- **Predictable codegen** - straightforward mapping to assembly
- **Complete type system** - 8 integer types, pointers, arrays
- **Full control flow** - if/else, loops, switch, functions
- **Zero external dependencies** - just libc, gcc, and nasm

## Quick Start

### Build the Compiler

```bash
gcc -o common common.c
```

### Hello World

Create `hello.cm`:
```c
void puts(uint8 *s);

int32 main(void) {
    puts("Hello, World!");
    return 0;
}
```

Compile and run:
```bash
./common hello.cm hello.asm
nasm -f elf32 hello.asm -o hello.o
gcc -m32 hello.o -o hello
./hello
```

Or use the Makefile:
```bash
make                    # Build compiler and test suite
make test               # Run all tests
make hello              # Build hello example
make examples           # Build all examples
make run-examples       # Build and run all examples
```

## Documentation

### For Users

- **[Quick Reference](QUICKREF.md)** - One-page cheat sheet for syntax and operators
- **[Reference Manual](MANUAL.md)** - Complete language specification (80+ pages)
- **[Troubleshooting Guide](TROUBLESHOOTING.md)** - Solutions to common problems

### For Developers

- **[Test Suite README](README_TESTS.md)** - How to run and write tests
- **[Source Code](common.c)** - Well-commented compiler implementation

## Language Features

### Types

```c
// Integers
int8  int16  int32  int64      // Signed
uint8 uint16 uint32 uint64     // Unsigned

// Pointers and arrays
int32 *ptr;                    // Pointer
int32 arr[10];                 // Array
uint8 *str = "text";           // String
```

### Control Flow

```c
if (x > 0) { ... }
while (x < 100) { ... }
for (int32 i = 0; i < n; i++) { ... }
switch (x) { case 1: ... break; }
```

### Operators

```c
// Arithmetic: + - * / %
// Comparison: == != < <= > >=
// Logical: && || !
// Bitwise: & | ^ ~ << >>
// Pointers: & *
// Increment: ++ --
```

### Functions

```c
int32 add(int32 a, int32 b) {
    return a + b;
}

int32 factorial(int32 n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

## Example Programs

All examples are in the `examples/` directory:

| Program | Description |
|---------|-------------|
| **hello.cm** | Hello World |
| **fibonacci.cm** | Recursive Fibonacci |
| **arrays.cm** | Array operations |
| **pointers.cm** | Pointer manipulation |
| **bubblesort.cm** | Bubble sort algorithm |
| **bitwise.cm** | Bitwise operations |
| **types.cm** | Type casting examples |
| **switch.cm** | Switch statements |
| **primes.cm** | Prime number calculator |
| **strings.cm** | String functions |
| **calculator.cm** | Expression evaluator |
| **linkedlist.cm** | Linked list (simulated) |

Build any example:
```bash
make fibonacci && ./fibonacci
make bubblesort && ./bubblesort
```

## Test Suite

The test suite includes 60+ automated tests covering:
- Arithmetic and operators
- Variables and arrays
- Control flow
- Functions and recursion
- Pointers and type casting
- All integer types

Run tests:
```bash
make test
# or
./run_tests.sh
```

## Compilation Pipeline

```
source.cm → [common compiler] → output.asm → [nasm] → output.o → [gcc] → executable
```

1. **Common compiler**: Parses source, generates NASM assembly
2. **NASM**: Assembles to ELF32 object file
3. **GCC**: Links with C runtime library

## Requirements

- **GCC** with 32-bit support (gcc-multilib)
- **NASM** assembler
- **Linux** or compatible environment (WSL works)

Installation:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential gcc-multilib nasm

# Fedora/RHEL  
sudo dnf install gcc glibc-devel.i686 nasm

# Arch
sudo pacman -S gcc lib32-gcc-libs nasm
```

## Language Limitations

- **Single file compilation** - no modules or includes
- **No structs/unions** - use arrays for structured data
- **No floating point** - integers only
- **No preprocessor** - no #define, #include
- **1D arrays only** - simulate 2D with manual indexing
- **Partial 64-bit support** - types exist but ops truncate to 32-bit

See [MANUAL.md](MANUAL.md) for complete details and workarounds.

## Implementation Details

**Target**: x86-32 (IA-32) ELF  
**Calling convention**: cdecl  
**Stack alignment**: 16-byte (System V ABI)  
**Registers**:
- `eax`: return values, expressions
- `ecx`: left operand  
- `edx`: scratch
- `ebp`: frame pointer
- `esp`: stack pointer

**Code sections**:
- `.text`: executable code
- `.data`: initialized globals, strings
- `.bss`: zero-initialized globals

## Architecture

The compiler is a single-pass implementation in C99:

```
┌─────────────┐
│   Lexer     │ Tokenize source
├─────────────┤
│   Parser    │ Build AST
├─────────────┤
│  Type Check │ Infer expression types
├─────────────┤
│  Code Gen   │ Emit NASM assembly
└─────────────┘
```

Key components:
- **Lexer** (150 LOC): Tokenization with lookahead
- **Parser** (400 LOC): Recursive descent parser
- **Type System** (200 LOC): Type inference for pointer arithmetic
- **Code Generator** (800 LOC): Assembly emission

Total: ~2000 lines of C99

## C Interoperability

Common can call C functions:

```c
// Declare C functions
void printf(uint8 *fmt, ...);
void *malloc(uint32 size);
void free(void *ptr);

int32 main(void) {
    printf("Allocated %d bytes\n", 100);
    void *mem = malloc(100);
    free(mem);
    return 0;
}
```

C can call Common functions:
```c
// common.cm
int32 compute(int32 x) {
    return x * x;
}

// main.c
extern int compute(int);
int main() {
    printf("%d\n", compute(10));
}
```

Compile:
```bash
./common common.cm common.asm
nasm -f elf32 common.asm -o common.o
gcc -m32 main.c common.o -o program
```

## Comparison to C

### Similar to C
- Syntax and semantics
- Type system (with fewer types)
- Pointer arithmetic
- Control flow
- Function calls (cdecl)

### Different from C  
- No preprocessor
- No structs/unions
- No enums
- No static/extern keywords
- No goto
- Single file only
- Simpler type system

### Simpler than C
- No type qualifiers (const, volatile)
- No storage classes (auto, register)
- No function pointers (can cast to void*)
- No variadic function definitions
- No bitfields
- No flexible array members

## Project Structure

```
.
├── common.c              # Compiler source (2000 LOC)
├── Makefile              # Build automation
├── run_tests.sh          # Quick test script
│
├── MANUAL.md             # Complete language reference
├── QUICKREF.md           # One-page cheat sheet
├── TROUBLESHOOTING.md    # Problem solutions
├── README_TESTS.md       # Test suite documentation
│
├── test_runner.c         # Automated test harness
│
└── examples/             # Example programs
    ├── hello.cm
    ├── fibonacci.cm
    ├── arrays.cm
    ├── pointers.cm
    ├── bubblesort.cm
    ├── bitwise.cm
    ├── types.cm
    ├── switch.cm
    ├── primes.cm
    ├── strings.cm
    ├── calculator.cm
    └── linkedlist.cm
```

## License

Public domain / CC0. Use freely for any purpose.

## Credits

Inspired by:
- **C** - Dennis Ritchie and Brian Kernighan
- **chibicc** - Rui Ueyama's educational C compiler
- **8cc** - Rui Ueyama's C compiler
- **tcc** - Fabrice Bellard's Tiny C Compiler

Built for programmers who value:
- Simplicity over features
- Control over convenience
- Learning over abstraction

---

Start with the [Quick Reference](QUICKREF.md) or dive into the [Manual](MANUAL.md).
