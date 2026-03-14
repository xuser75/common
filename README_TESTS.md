# Common Compiler - Test Suite and Examples

This directory contains a comprehensive test suite and example programs for the Common programming language compiler.

## Building the Compiler

First, build the compiler:

```bash
gcc -o common common.c
```

## Test Suite

### Building and Running Tests

The test suite is a dependency-free C99 program that automatically compiles, assembles, links, and runs test programs.

```bash
# Build the test runner
gcc -std=c99 -o test_runner test_runner.c

# Run all tests
./test_runner
```

The test runner will:
1. Compile each test program with the Common compiler
2. Assemble with NASM
3. Link with GCC
4. Execute and verify the results
5. Report pass/fail status

### Test Coverage

The test suite includes over 60 tests covering:

- **Arithmetic**: add, subtract, multiply, divide, modulo
- **Variables**: local and global variables with initialization
- **Control Flow**: if/else, while, for, switch/case, break, continue
- **Operators**: 
  - Comparison: ==, !=, <, <=, >, >=
  - Logical: &&, ||, !
  - Bitwise: &, |, ^, ~, <<, >>
  - Increment/Decrement: ++, --
  - Compound Assignment: +=, -=, *=, /=, etc.
- **Functions**: calls, recursion, multiple parameters
- **Arrays**: declaration, initialization, indexing
- **Pointers**: address-of, dereference, pointer arithmetic
- **Type Casting**: explicit casts between integer types
- **Integer Types**: uint8, uint16, uint32, uint64, int8, int16, int32, int64
- **Ternary Operator**: ? :

## Example Programs

The `examples/` directory contains practical programs demonstrating Common language features.

### Compiling Examples

All examples follow this pattern:

```bash
./common examples/hello.cm hello.asm
nasm -f elf32 hello.asm -o hello.o
gcc -m32 hello.o -o hello
./hello
```

Or use this one-liner:

```bash
./common examples/hello.cm hello.asm && nasm -f elf32 hello.asm && gcc -m32 hello.o -o hello && ./hello
```

### Available Examples

#### hello.cm
Basic "Hello, World!" program.

```bash
./common examples/hello.cm hello.asm && nasm -f elf32 hello.asm && gcc -m32 hello.o -o hello
./hello
```

#### fibonacci.cm
Recursive Fibonacci number calculator. Demonstrates:
- Function recursion
- Conditionals
- Loops

```bash
./common examples/fibonacci.cm fib.asm && nasm -f elf32 fib.asm && gcc -m32 fib.o -o fib
./fib
```

#### arrays.cm
Array manipulation with sum and reverse operations. Demonstrates:
- Array initialization
- Array traversal
- Function parameters with arrays
- Array modification

```bash
./common examples/arrays.cm arrays.asm && nasm -f elf32 arrays.asm && gcc -m32 arrays.o -o arrays
./arrays
```

#### pointers.cm
Pointer operations and pointer arithmetic. Demonstrates:
- Pointer declaration and dereferencing
- Address-of operator
- Pointer arithmetic
- Pointer to pointer
- Pass-by-reference with pointers

```bash
./common examples/pointers.cm ptrs.asm && nasm -f elf32 ptrs.asm && gcc -m32 ptrs.o -o ptrs
./ptrs
```

#### bubblesort.cm
Bubble sort implementation. Demonstrates:
- Nested loops
- Array sorting
- Swap algorithm

```bash
./common examples/bubblesort.cm sort.asm && nasm -f elf32 sort.asm && gcc -m32 sort.o -o sort
./sort
```

#### bitwise.cm
Comprehensive bitwise operations. Demonstrates:
- Bitwise AND, OR, XOR, NOT
- Bit shifts
- Bit counting
- Bit manipulation algorithms

```bash
./common examples/bitwise.cm bits.asm && nasm -f elf32 bits.asm && gcc -m32 bits.o -o bits
./bits
```

#### types.cm
Different integer types and type casting. Demonstrates:
- uint8, uint16, uint32
- int8, int16, int32
- Type casting
- Sign extension
- Truncation behavior

```bash
./common examples/types.cm types.asm && nasm -f elf32 types.asm && gcc -m32 types.o -o types
./types
```

#### switch.cm
Switch/case statement usage. Demonstrates:
- Switch statements
- Case labels
- Default case
- Fall-through behavior

```bash
./common examples/switch.cm switch.asm && nasm -f elf32 switch.asm && gcc -m32 switch.o -o switch
./switch
```

#### primes.cm
Prime number calculator. Demonstrates:
- Mathematical algorithms
- Optimized loop conditions
- Complex conditionals

```bash
./common examples/primes.cm primes.asm && nasm -f elf32 primes.asm && gcc -m32 primes.o -o primes
./primes
```

#### strings.cm
String manipulation functions. Demonstrates:
- String literals
- Character arrays
- String length, copy, compare
- String reversal

```bash
./common examples/strings.cm strings.asm && nasm -f elf32 strings.asm && gcc -m32 strings.o -o strings
./strings
```

#### calculator.cm
Expression calculator with global state. Demonstrates:
- Global variables
- Multiple function definitions
- Function composition
- Error handling
- State tracking

```bash
./common examples/calculator.cm calc.asm && nasm -f elf32 calc.asm && gcc -m32 calc.o -o calc
./calc
```

## Common Language Quick Reference

### Types
```c
uint8, uint16, uint32, uint64
int8, int16, int32, int64
void
```

### Variables
```c
int32 x;           // Declaration
int32 y = 42;      // Declaration with initialization
int32 arr[10];     // Array declaration
int32 *ptr;        // Pointer declaration
```

### Control Flow
```c
if (condition) { ... }
if (condition) { ... } else { ... }
while (condition) { ... }
for (init; condition; increment) { ... }
switch (expr) {
    case value: ... break;
    default: ...
}
```

### Operators
```c
// Arithmetic: +, -, *, /, %
// Comparison: ==, !=, <, <=, >, >=
// Logical: &&, ||, !
// Bitwise: &, |, ^, ~, <<, >>
// Assignment: =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
// Increment/Decrement: ++, --
// Ternary: ? :
// Address: &, *
```

### Functions
```c
int32 function_name(int32 param1, uint8 *param2) {
    return value;
}
```

### Comments
```c
// Single line comment
/* Multi-line
   comment */
```

## Requirements

- GCC with 32-bit support (gcc-multilib)
- NASM assembler
- Linux (or compatible environment)

On Ubuntu/Debian:
```bash
sudo apt-get install gcc-multilib nasm
```

## Troubleshooting

**Error: "cannot find -lgcc_s"**
- Install 32-bit libraries: `sudo apt-get install gcc-multilib`

**Error: "nasm: command not found"**
- Install NASM: `sudo apt-get install nasm`

**Test failures**
- Ensure the compiler binary is in the current directory: `./common`
- Check that you have write access to `/tmp/`
- Verify 32-bit support is installed

## License

The test suite and examples are provided as-is for testing and demonstration purposes.
