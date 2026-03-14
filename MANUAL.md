# Common Language Reference Manual

**Version 1.0**  
**Target**: x86-32 (IA-32) Linux ELF  
**Calling Convention**: cdecl  
**Author**: Common Compiler Project

---

## Table of Contents

1. [Introduction](#introduction)
2. [Compiler Usage](#compiler-usage)
3. [Lexical Elements](#lexical-elements)
4. [Type System](#type-system)
5. [Declarations](#declarations)
6. [Expressions](#expressions)
7. [Statements](#statements)
8. [Functions](#functions)
9. [Scope and Linkage](#scope-and-linkage)
10. [Memory Model](#memory-model)
11. [Assembly Interface](#assembly-interface)
12. [Limitations](#limitations)
13. [Examples](#examples)

---

## 1. Introduction

Common is a statically-typed, imperative programming language that compiles to x86-32 assembly (NASM syntax). It provides a minimal yet complete set of features for systems programming:

- Integer types from 8 to 64 bits
- Pointers and arrays
- Functions with parameters
- Control flow (if, while, for, switch)
- Full operator set (arithmetic, logical, bitwise)
- Direct C library interoperability

### Design Philosophy

- **No runtime dependencies**: Compiled programs link only against libc
- **Explicit control**: No hidden allocations or implicit conversions
- **Predictable output**: Direct mapping to assembly
- **C compatibility**: Can call and be called by C code

---

## 2. Compiler Usage

### Building the Compiler

```bash
gcc -o common common.c
```

### Compiling Programs

```bash
# Compile Common source to NASM assembly
./common source.cm output.asm

# Assemble to object file
nasm -f elf32 output.asm -o output.o

# Link (requires 32-bit support)
gcc -m32 output.o -o executable
```

### One-Line Compilation

```bash
./common source.cm output.asm && nasm -f elf32 output.asm && gcc -m32 output.o -o program
```

### Compiler Output

The compiler writes NASM x86-32 assembly to stdout (or specified file) using:
- **ELF32** object format
- **cdecl** calling convention
- **Sections**: `.text`, `.data`, `.bss`

### Error Reporting

Errors are reported to stderr with line numbers:

```
line 42: syntax error near 'token'
line 15: Unknown char '~'
```

---

## 3. Lexical Elements

### Comments

```c
// Single-line comment (C++ style)

/* Multi-line comment
   spanning multiple lines */
```

Comments are stripped during lexical analysis.

### Keywords

```
if       else     while    for      switch   case     default
break    continue return
void     uint8    uint16   uint32   uint64
         int8     int16    int32    int64
```

### Identifiers

```
[a-zA-Z_][a-zA-Z0-9_]*
```

- Must start with letter or underscore
- Case-sensitive
- No length limit (internal buffer: 256 chars)

### Integer Literals

```c
42          // Decimal
0x2A        // Hexadecimal
052         // Octal
0b101010    // Binary (if supported by strtoul)
```

Literals are parsed by `strtoul()` with base 0 (auto-detect).

### String Literals

```c
"Hello, World!"
"Line 1\nLine 2"
"Tab\there"
```

Supported escape sequences:
- `\n` - newline
- `\t` - tab
- `\r` - carriage return
- `\0` - null character
- `\\` - backslash
- `\"` - quote
- Any other `\x` - literal `x`

String literals are null-terminated and stored in `.data` section.

### Operators and Punctuation

**Multi-character operators**:
```
==  !=  <=  >=  &&  ||  <<  >>  ++  --
+=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
```

**Single-character operators**:
```
+  -  *  /  %  &  |  ^  ~  !  <  >  =
```

**Punctuation**:
```
(  )  {  }  [  ]  ;  ,  :  ?
```

---

## 4. Type System

### Integer Types

| Type    | Size  | Range (Unsigned)      | Range (Signed)              |
|---------|-------|----------------------|-----------------------------|
| uint8   | 1 byte| 0 to 255             | -                           |
| int8    | 1 byte| -                    | -128 to 127                 |
| uint16  | 2 bytes| 0 to 65,535         | -                           |
| int16   | 2 bytes| -                   | -32,768 to 32,767           |
| uint32  | 4 bytes| 0 to 4,294,967,295  | -                           |
| int32   | 4 bytes| -                   | -2,147,483,648 to 2,147,483,647 |
| uint64  | 8 bytes| 0 to 2^64-1         | -                           |
| int64   | 8 bytes| -                   | -2^63 to 2^63-1             |

**Note**: 64-bit types are partially supported. They occupy 8 bytes in memory but arithmetic operations truncate to 32 bits on x86-32.

### Void Type

```c
void
```

- Used only for function return types
- Cannot declare variables of type void
- `void` in parameter list means "no parameters"

### Pointer Types

```c
int32 *ptr;        // Pointer to int32
uint8 **pptr;      // Pointer to pointer to uint8
void *generic;     // Generic pointer (4 bytes)
```

- All pointers are 4 bytes (32-bit addresses)
- Pointer arithmetic scales by pointee size
- Can be cast between types

### Array Types

```c
int32 arr[10];           // Array of 10 int32
uint8 matrix[5][5];      // Not supported (single dimension only)
```

Arrays:
- Decay to pointers when used in expressions
- Cannot be returned from functions
- Cannot be assigned (use element-wise copy)

### Type Qualifiers

Common has no type qualifiers (no `const`, `volatile`, `restrict`).

---

## 5. Declarations

### Variable Declarations

**Local variables**:
```c
int32 x;              // Uninitialized
int32 y = 42;         // Initialized
uint8 c = 'A';        // Character (just an int)
```

**Global variables**:
```c
int32 global_var;           // Zero-initialized (.bss)
int32 initialized = 100;    // Explicitly initialized (.data)
```

### Array Declarations

**Local arrays**:
```c
int32 arr[10];                          // Uninitialized
int32 nums[5] = { 1, 2, 3, 4, 5 };     // Initialized
uint8 partial[10] = { 1, 2 };          // Rest zero-filled
```

**Global arrays**:
```c
int32 global_arr[100];                  // Zero-initialized (.bss)
int32 data[3] = { 10, 20, 30 };        // Initialized (.data)
```

### Pointer Declarations

```c
int32 *ptr;              // Pointer to int32
uint8 *str;              // Pointer to uint8 (common for strings)
void *generic;           // Generic pointer
int32 **pptr;            // Pointer to pointer
```

### Type Syntax

```
type_specifier ::= base_type pointer_suffix
base_type      ::= "int8" | "int16" | "int32" | "int64"
                 | "uint8" | "uint16" | "uint32" | "uint64"
                 | "void"
pointer_suffix ::= ("*")*
```

Examples:
```c
int32 x;         // Base type: int32, no pointers
uint8 *s;        // Base type: uint8, 1 pointer level
void **pp;       // Base type: void, 2 pointer levels
```

---

## 6. Expressions

### Primary Expressions

```c
42              // Integer literal
"string"        // String literal
variable        // Identifier
(expression)    // Parenthesized expression
```

### Postfix Expressions

```c
array[index]           // Array subscript
function(args)         // Function call
expr++                 // Post-increment
expr--                 // Post-decrement
```

### Unary Expressions

```c
++expr          // Pre-increment
--expr          // Pre-decrement
-expr           // Negation
!expr           // Logical NOT
~expr           // Bitwise NOT
&expr           // Address-of
*expr           // Dereference
(type)expr      // Type cast
```

### Binary Expressions

**Arithmetic**:
```c
a + b           // Addition
a - b           // Subtraction
a * b           // Multiplication
a / b           // Division
a % b           // Modulo
```

**Bitwise**:
```c
a & b           // Bitwise AND
a | b           // Bitwise OR
a ^ b           // Bitwise XOR
a << b          // Left shift
a >> b          // Right shift (arithmetic for signed, logical for unsigned)
```

**Comparison**:
```c
a == b          // Equal
a != b          // Not equal
a < b           // Less than
a <= b          // Less than or equal
a > b           // Greater than
a >= b          // Greater than or equal
```

**Logical**:
```c
a && b          // Logical AND (short-circuit)
a || b          // Logical OR (short-circuit)
```

### Assignment Expressions

```c
a = b           // Assignment
a += b          // Add and assign
a -= b          // Subtract and assign
a *= b          // Multiply and assign
a /= b          // Divide and assign
a %= b          // Modulo and assign
a &= b          // AND and assign
a |= b          // OR and assign
a ^= b          // XOR and assign
a <<= b         // Left shift and assign
a >>= b         // Right shift and assign
```

### Ternary Expression

```c
condition ? true_expr : false_expr
```

Example:
```c
max = (a > b) ? a : b;
```

### Operator Precedence

From highest to lowest:

| Level | Operators                  | Associativity |
|-------|----------------------------|---------------|
| 1     | `()` `[]` `++` `--` (post) | Left to right |
| 2     | `++` `--` (pre) `+` `-` `!` `~` `&` `*` `(cast)` | Right to left |
| 3     | `*` `/` `%`                | Left to right |
| 4     | `+` `-`                    | Left to right |
| 5     | `<<` `>>`                  | Left to right |
| 6     | `<` `<=` `>` `>=`          | Left to right |
| 7     | `==` `!=`                  | Left to right |
| 8     | `&`                        | Left to right |
| 9     | `^`                        | Left to right |
| 10    | `|`                        | Left to right |
| 11    | `&&`                       | Left to right |
| 12    | `||`                       | Left to right |
| 13    | `?:`                       | Right to left |
| 14    | `=` `+=` `-=` etc.         | Right to left |

### Pointer Arithmetic

```c
int32 *p = arr;
p + 1           // Points to next int32 (address + 4)
p - 1           // Points to previous int32 (address - 4)
p[i]            // Equivalent to *(p + i)
```

Pointer arithmetic automatically scales by the size of the pointed-to type:
- `uint8*` increments by 1
- `uint16*` increments by 2
- `int32*` increments by 4
- Any pointer-to-pointer increments by 4

### Type Conversions

**Explicit casting**:
```c
(uint8)value          // Truncate to 8 bits
(int32)byte_value     // Sign-extend or zero-extend
(uint32*)ptr          // Pointer type conversion
```

**Implicit conversions**:
- Arrays decay to pointers
- Smaller integers promote to int32 in expressions

---

## 7. Statements

### Expression Statement

```c
expression;
```

Examples:
```c
x = 42;
function_call();
x++;
```

### Compound Statement (Block)

```c
{
    statement1;
    statement2;
    ...
}
```

Blocks create new scopes for local variables.

### If Statement

```c
if (condition)
    statement

if (condition)
    statement
else
    statement
```

Examples:
```c
if (x > 0)
    printf("positive\n");

if (x > 0) {
    printf("positive\n");
} else if (x < 0) {
    printf("negative\n");
} else {
    printf("zero\n");
}
```

### While Statement

```c
while (condition)
    statement
```

Example:
```c
while (x < 100) {
    x = x * 2;
}
```

### For Statement

```c
for (init; condition; increment)
    statement
```

The `init` can be:
- Empty: `for (; condition; increment)`
- Expression: `for (x = 0; x < 10; x++)`
- Declaration: `for (int32 i = 0; i < 10; i++)`

Example:
```c
for (int32 i = 0; i < 10; i = i + 1) {
    sum = sum + i;
}
```

### Switch Statement

```c
switch (expression) {
    case value1:
        statements
        break;
    case value2:
        statements
        break;
    default:
        statements
}
```

- Cases must be integer constants
- Fall-through is allowed (no automatic break)
- `default` is optional

Example:
```c
switch (day) {
    case 0:
        printf("Sunday\n");
        break;
    case 6:
        printf("Saturday\n");
        break;
    default:
        printf("Weekday\n");
}
```

### Break Statement

```c
break;
```

Exits the innermost `while`, `for`, or `switch` statement.

### Continue Statement

```c
continue;
```

Skips to the next iteration of the innermost `while` or `for` loop.

### Return Statement

```c
return;              // Return from void function
return expression;   // Return value
```

Example:
```c
return 42;
return x + y;
return;
```

---

## 8. Functions

### Function Declarations

```c
return_type function_name(parameter_list);
```

Forward declaration (prototype):
```c
int32 add(int32 a, int32 b);
```

### Function Definitions

```c
return_type function_name(parameter_list) {
    statements
}
```

Example:
```c
int32 add(int32 a, int32 b) {
    return a + b;
}
```

### Parameters

```c
void no_params(void) { }                    // No parameters
int32 one_param(int32 x) { }                // One parameter
int32 two_params(int32 x, uint8 *s) { }     // Multiple parameters
```

Parameters are passed by value. To modify caller's data, use pointers:

```c
void swap(int32 *a, int32 *b) {
    int32 temp = *a;
    *a = *b;
    *b = temp;
}
```

### Return Values

```c
int32 get_value(void) {
    return 42;
}

void no_return(void) {
    // No return statement needed
    return;  // Optional
}
```

Return value is passed in `eax` register (32-bit).

### Recursion

Recursion is fully supported:

```c
int32 factorial(int32 n) {
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}
```

### Calling Convention

Functions use **cdecl** convention:
- Arguments pushed right-to-left on stack
- Caller cleans up stack
- Return value in `eax`
- `eax`, `ecx`, `edx` are caller-saved
- `ebx`, `esi`, `edi`, `ebp` are callee-saved

### Calling C Functions

Common can call C library functions:

```c
// Declare C functions
void printf(uint8 *format, ...);
void *malloc(uint32 size);
void free(void *ptr);

int32 main(void) {
    printf("Hello from Common\n");
    void *mem = malloc(100);
    free(mem);
    return 0;
}
```

**Note**: Variadic functions (`...`) can be declared but not defined in Common.

---

## 9. Scope and Linkage

### Scope Rules

**Global scope**:
- Variables and functions declared outside any function
- Visible to all functions in the file

**Local scope**:
- Variables declared inside a function or block
- Visible only within that function/block
- Shadows global variables with the same name

**Block scope**:
```c
{
    int32 x = 1;
    {
        int32 x = 2;  // Different variable, shadows outer x
        printf("%d\n", x);  // Prints 2
    }
    printf("%d\n", x);  // Prints 1
}
```

### Linkage

**External linkage** (default for functions):
```c
int32 global_function(void) { ... }
```
Symbol is exported (`global` directive in assembly).

**No linkage** (local variables):
```c
void func(void) {
    int32 local;  // No linkage
}
```

**Static linkage**: Not supported. All functions have external linkage.

### Name Resolution

1. Check local scope (function parameters and locals)
2. Check global scope
3. If not found, assumed to be external symbol

---

## 10. Memory Model

### Stack Layout

```
High Address
+------------------+
| Return address   |
+------------------+
| Saved EBP        | <-- EBP
+------------------+
| Local variable 1 | EBP - 4
+------------------+
| Local variable 2 | EBP - 8
+------------------+
| ...              |
+------------------+
| Array data       | (grows down)
+------------------+ <-- ESP
Low Address
```

### Function Call Stack

```c
caller():
    push arg2
    push arg1
    call callee
    add esp, 8      // Clean up arguments

callee(arg1, arg2):
    push ebp        // Save old frame pointer
    mov ebp, esp    // Set up new frame
    sub esp, N      // Allocate locals
    ...
    mov esp, ebp    // Restore stack
    pop ebp
    ret
```

Arguments accessed via `[ebp+8]`, `[ebp+12]`, etc.
Locals accessed via `[ebp-4]`, `[ebp-8]`, etc.

### Data Sections

**.text**: Read-only code
```nasm
section .text
function_name:
    ; assembly code
```

**.data**: Initialized data
```nasm
section .data
global_var: dd 42
string: db "Hello", 0
```

**.bss**: Zero-initialized data
```nasm
section .bss
uninit_var: resd 1
array: resb 100
```

### Size Directives

| Directive | Size  | Common Type    |
|-----------|-------|----------------|
| `resb`/`db` | 1 byte | uint8/int8   |
| `resw`/`dw` | 2 bytes| uint16/int16 |
| `resd`/`dd` | 4 bytes| uint32/int32/pointers |
| `resq`/`dq` | 8 bytes| uint64/int64 |

### Alignment

- Stack is 16-byte aligned (per System V ABI)
- Local variables are 4-byte aligned
- Arrays follow element alignment

---

## 11. Assembly Interface

### Generated Assembly Structure

```nasm
BITS 32
section .text

; External function declarations
extern printf
extern malloc

; Exported functions
global main
global my_function

main:
    push ebp
    mov ebp, esp
    sub esp, 16          ; Allocate locals
    ; ... function body ...
    mov esp, ebp
    pop ebp
    ret

section .data
_s0: db "Hello", 0      ; String literal

section .bss
global_var: resd 1      ; Global variable
```

### Register Usage

**Caller-saved** (may be modified by called function):
- `eax` - Return value, scratch
- `ecx` - Scratch, left operand
- `edx` - Scratch, division remainder

**Callee-saved** (preserved across calls):
- `ebx` - Base register
- `esi` - Source index
- `edi` - Destination index
- `ebp` - Frame pointer
- `esp` - Stack pointer

**Common usage**:
- `eax` - Expression results, return values
- `ecx` - Left operand in binary operations
- `[ebp+N]` - Function parameters
- `[ebp-N]` - Local variables

### Calling C from Assembly

```nasm
; Call: printf("Value: %d\n", x);
push dword [ebp-4]          ; Push x
push _s0                     ; Push format string
call printf
add esp, 8                   ; Clean up (2 args × 4 bytes)
```

### Inline Assembly

Not supported. Use C library functions or write separate assembly files.

---

## 12. Limitations

### Language Limitations

1. **Single-file compilation only**
   - No `#include` or import mechanism
   - All code must be in one source file
   - Use forward declarations for ordering

2. **No structures or unions**
   - Can simulate with arrays: `node[0]` = data, `node[1]` = next
   - Manual offset calculation required

3. **No floating point**
   - Integer arithmetic only
   - No `float`, `double`, or `long double`

4. **No preprocessor**
   - No `#define`, `#ifdef`, etc.
   - No macro expansion
   - No file inclusion

5. **No enums**
   - Use integer constants instead

6. **Limited 64-bit support**
   - 64-bit types exist but operations truncate to 32-bit
   - Full 64-bit arithmetic not implemented

7. **No static/extern keywords**
   - All functions are global
   - No static local variables
   - No explicit extern declarations

8. **Single-dimensional arrays only**
   - Multidimensional arrays not supported
   - Can use pointer arithmetic for 2D: `arr[i * width + j]`

9. **No goto**
   - Use loops and breaks instead

10. **No comma operator**
    - Cannot use `a = (b, c)`

### Implementation Limitations

1. **Fixed buffer sizes**
   - 256 identifiers/strings
   - 256 local variables per function
   - 256 global variables total
   - 512 string literals

2. **No optimization**
   - Generated code is unoptimized
   - Expressions fully evaluated (no constant folding)

3. **Limited error messages**
   - Basic syntax errors reported
   - No semantic analysis warnings
   - No type mismatch warnings

4. **x86-32 only**
   - Not portable to other architectures
   - Requires 32-bit toolchain

### Workarounds

**Structures**: Use arrays
```c
// Instead of: struct { int x; int y; } point;
int32 point[2];  // point[0] = x, point[1] = y
```

**Multidimensional arrays**: Manual indexing
```c
// Instead of: int matrix[10][10];
int32 matrix[100];
int32 value = matrix[row * 10 + col];
```

**Enums**: Integer constants
```c
// Instead of: enum { RED, GREEN, BLUE };
int32 RED = 0;
int32 GREEN = 1;
int32 BLUE = 2;
```

---

## 13. Examples

### Hello World

```c
void puts(uint8 *s);

int32 main(void) {
    puts("Hello, World!");
    return 0;
}
```

### Factorial (Iterative)

```c
int32 factorial(int32 n) {
    int32 result = 1;
    for (int32 i = 2; i <= n; i = i + 1) {
        result = result * i;
    }
    return result;
}
```

### Fibonacci (Recursive)

```c
int32 fib(int32 n) {
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}
```

### String Length

```c
int32 strlen(uint8 *s) {
    int32 len = 0;
    while (s[len])
        len = len + 1;
    return len;
}
```

### Array Sum

```c
int32 sum_array(int32 *arr, int32 len) {
    int32 total = 0;
    for (int32 i = 0; i < len; i = i + 1) {
        total = total + arr[i];
    }
    return total;
}
```

### Pointer Swap

```c
void swap(int32 *a, int32 *b) {
    int32 temp = *a;
    *a = *b;
    *b = temp;
}
```

### Bubble Sort

```c
void bubble_sort(int32 *arr, int32 n) {
    for (int32 i = 0; i < n - 1; i = i + 1) {
        for (int32 j = 0; j < n - i - 1; j = j + 1) {
            if (arr[j] > arr[j + 1]) {
                int32 temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}
```

### Binary Search

```c
int32 binary_search(int32 *arr, int32 n, int32 target) {
    int32 left = 0;
    int32 right = n - 1;
    
    while (left <= right) {
        int32 mid = left + (right - left) / 2;
        
        if (arr[mid] == target)
            return mid;
        
        if (arr[mid] < target)
            left = mid + 1;
        else
            right = mid - 1;
    }
    
    return -1;  // Not found
}
```

### Linked List (Simulated)

```c
void *malloc(uint32 size);
void free(void *ptr);

// Node: [0] = data, [1] = next pointer
int32 *create_node(int32 value) {
    int32 *node = (int32*)malloc(8);
    node[0] = value;
    node[1] = 0;
    return node;
}

void insert_front(int32 **head, int32 value) {
    int32 *new_node = create_node(value);
    new_node[1] = (int32)(*head);
    *head = new_node;
}
```

### Bitwise Operations

```c
// Check if bit N is set
int32 is_bit_set(uint32 value, int32 n) {
    return (value >> n) & 1;
}

// Set bit N
uint32 set_bit(uint32 value, int32 n) {
    return value | (1 << n);
}

// Clear bit N
uint32 clear_bit(uint32 value, int32 n) {
    return value & ~(1 << n);
}

// Toggle bit N
uint32 toggle_bit(uint32 value, int32 n) {
    return value ^ (1 << n);
}
```

---

## Appendix A: Grammar Summary

```
program ::= declaration*

declaration ::= 
    | type_spec identifier "(" param_list ")" ( ";" | block )
    | type_spec identifier ";"
    | type_spec identifier "=" expr ";"
    | type_spec identifier "[" expr "]" ";"
    | type_spec identifier "[" expr "]" "=" "{" expr_list "}" ";"

type_spec ::= base_type "*"*

base_type ::= "void" | "int8" | "int16" | "int32" | "int64"
            | "uint8" | "uint16" | "uint32" | "uint64"

param_list ::= "void" | ( param ( "," param )* )?

param ::= type_spec identifier

block ::= "{" statement* "}"

statement ::=
    | block
    | type_spec identifier ";"
    | type_spec identifier "=" expr ";"
    | type_spec identifier "[" expr "]" ( "=" "{" expr_list "}" )? ";"
    | expr ";"
    | "if" "(" expr ")" statement ( "else" statement )?
    | "while" "(" expr ")" statement
    | "for" "(" (decl | expr)? ";" expr? ";" expr? ")" statement
    | "switch" "(" expr ")" "{" case_clause* "}"
    | "return" expr? ";"
    | "break" ";"
    | "continue" ";"

case_clause ::=
    | "case" expr ":" statement*
    | "default" ":" statement*

expr ::= assignment

assignment ::= ternary ( assign_op ternary )?

assign_op ::= "=" | "+=" | "-=" | "*=" | "/=" | "%="
            | "&=" | "|=" | "^=" | "<<=" | ">>="

ternary ::= logical_or ( "?" expr ":" ternary )?

logical_or ::= logical_and ( "||" logical_and )*

logical_and ::= bit_or ( "&&" bit_or )*

bit_or ::= bit_xor ( "|" bit_xor )*

bit_xor ::= bit_and ( "^" bit_and )*

bit_and ::= equality ( "&" equality )*

equality ::= relational ( ("==" | "!=") relational )*

relational ::= shift ( ("<" | "<=" | ">" | ">=") shift )*

shift ::= additive ( ("<<" | ">>") additive )*

additive ::= multiplicative ( ("+" | "-") multiplicative )*

multiplicative ::= unary ( ("*" | "/" | "%") unary )*

unary ::=
    | postfix
    | "++" unary
    | "--" unary
    | "-" unary
    | "!" unary
    | "~" unary
    | "&" unary
    | "*" unary
    | "(" type_spec ")" unary

postfix ::=
    | primary
    | postfix "[" expr "]"
    | postfix "(" expr_list? ")"
    | postfix "++"
    | postfix "--"

primary ::=
    | integer_literal
    | string_literal
    | identifier
    | "(" expr ")"
```

---

## Appendix B: Quick Reference Card

**Types**: void, int8, int16, int32, int64, uint8, uint16, uint32, uint64

**Operators**: + - * / % & | ^ ~ ! < > <= >= == != << >> && || ?: = += -= *= /= %= &= |= ^= <<= >>= ++ -- & * []

**Keywords**: if else while for switch case default break continue return

**Control**: if/else, while, for, switch/case, break, continue, return

**Functions**: type name(params) { body }

**Arrays**: type name[size], type name[size] = { values }

**Pointers**: type *name, &var, *ptr, ptr[index]

**Comments**: // line, /* block */

---

*End of Common Language Reference Manual*
