# Common Language Quick Reference

## Compilation

```bash
./common source.cm output.asm
nasm -f elf32 output.asm -o output.o
gcc -m32 output.o -o program
```

## Types

```c
void                          // No return value
int8   int16   int32   int64  // Signed integers
uint8  uint16  uint32  uint64 // Unsigned integers
type*                         // Pointer to type
type[N]                       // Array of N elements
```

## Variables

```c
int32 x;                      // Declaration
int32 y = 42;                 // Initialization
int32 arr[10];                // Array
int32 nums[3] = {1, 2, 3};   // Array with initializer
int32 *ptr;                   // Pointer
```

## Operators

### Arithmetic
```c
+  -  *  /  %                 // Add, Sub, Mul, Div, Mod
```

### Comparison
```c
==  !=  <  <=  >  >=          // Equal, Not-equal, Less, etc.
```

### Logical
```c
&&  ||  !                     // AND, OR, NOT (short-circuit)
```

### Bitwise
```c
&   |   ^   ~                 // AND, OR, XOR, NOT
<<  >>                        // Left shift, Right shift
```

### Assignment
```c
=   +=  -=  *=  /=  %=        // Assign, Add-assign, etc.
&=  |=  ^=  <<=  >>=          // Bitwise assign ops
```

### Increment/Decrement
```c
++  --                        // Increment, Decrement (pre/post)
```

### Pointer/Array
```c
&x                            // Address of x
*ptr                          // Dereference ptr
arr[i]                        // Array index (same as *(arr+i))
```

### Ternary
```c
cond ? true_val : false_val   // Conditional expression
```

## Control Flow

### If-Else
```c
if (condition)
    statement;
    
if (condition) {
    statements;
} else {
    statements;
}
```

### While Loop
```c
while (condition) {
    statements;
}
```

### For Loop
```c
for (int32 i = 0; i < n; i++) {
    statements;
}
```

### Switch
```c
switch (expr) {
    case 1:
        statements;
        break;
    case 2:
        statements;
        break;
    default:
        statements;
}
```

### Break/Continue/Return
```c
break;                        // Exit loop or switch
continue;                     // Next loop iteration
return;                       // Return from void function
return expr;                  // Return value
```

## Functions

### Declaration
```c
int32 add(int32 a, int32 b);  // Forward declaration
```

### Definition
```c
int32 add(int32 a, int32 b) {
    return a + b;
}
```

### No Parameters
```c
void func(void) {
    // ...
}
```

### Main Function
```c
int32 main(void) {
    // Entry point
    return 0;
}
```

## Pointers

```c
int32 x = 42;
int32 *p = &x;                // p points to x
*p = 100;                     // Set x to 100 via pointer
int32 y = *p;                 // Read through pointer
```

## Arrays

```c
int32 arr[5];                 // Declare
arr[0] = 10;                  // Set element
int32 x = arr[2];             // Get element

// Array initialization
int32 nums[5] = {1, 2, 3, 4, 5};

// Arrays decay to pointers
int32 *p = arr;               // p points to arr[0]
```

## Strings

```c
uint8 *str = "Hello";         // String literal
printf("%s\n", str);          // Print string

// String as array
uint8 msg[] = "Hello";
msg[0] = 'h';                 // Modify
```

## Comments

```c
// Single-line comment

/* Multi-line
   comment */
```

## Type Casting

```c
(type)expression              // Cast to type

int32 x = 1000;
uint8 y = (uint8)x;           // Truncate to 8 bits

uint8 *s = (uint8*)"string";  // Pointer cast
```

## Common Patterns

### Swap
```c
void swap(int32 *a, int32 *b) {
    int32 temp = *a;
    *a = *b;
    *b = temp;
}
```

### String Length
```c
int32 strlen(uint8 *s) {
    int32 len = 0;
    while (s[len]) len++;
    return len;
}
```

### Array Sum
```c
int32 sum(int32 *arr, int32 n) {
    int32 total = 0;
    for (int32 i = 0; i < n; i++)
        total += arr[i];
    return total;
}
```

### Min/Max
```c
int32 min(int32 a, int32 b) {
    return (a < b) ? a : b;
}

int32 max(int32 a, int32 b) {
    return (a > b) ? a : b;
}
```

## Calling C Functions

```c
// Declare before use
void printf(uint8 *fmt, ...);
void *malloc(uint32 size);
void free(void *ptr);

// Use
printf("Value: %d\n", x);
void *mem = malloc(100);
free(mem);
```

## Operator Precedence (High to Low)

1. `()` `[]` `.` `->`
2. `!` `~` `++` `--` `+` `-` `*` `&` (unary) `(cast)`
3. `*` `/` `%`
4. `+` `-`
5. `<<` `>>`
6. `<` `<=` `>` `>=`
7. `==` `!=`
8. `&`
9. `^`
10. `|`
11. `&&`
12. `||`
13. `?:`
14. `=` `+=` `-=` etc.

## Limitations

- No structs/unions (use arrays)
- No enums (use int32 constants)
- No floats (integers only)
- No preprocessor (#define, #include)
- Single file compilation only
- 1D arrays only (simulate 2D: `arr[row*width+col]`)
- No goto
- No static/extern keywords
- 64-bit types partially supported

## Common Gotchas

```c
// Assignment vs. Equality
if (x = 5)   // WRONG: assigns 5 to x
if (x == 5)  // RIGHT: compares x to 5

// Array indexing
int32 arr[10];
arr[10] = 0;  // WRONG: out of bounds
arr[9] = 0;   // RIGHT: last element

// Pointer arithmetic scales by type size
int32 *p = arr;
p + 1;        // Points 4 bytes ahead (size of int32)

// Semicolons required
if (x > 0)
    y = 1     // WRONG: missing semicolon
    
if (x > 0)
    y = 1;    // RIGHT
```

## Error Messages

```
line N: syntax error near 'token'
line N: Unknown char 'X'
line N: expected expression
line N: too many locals/globals/strings
```

Check:
- Missing semicolons
- Mismatched braces/parentheses
- Undeclared variables
- Type mismatches
- Buffer limits exceeded
