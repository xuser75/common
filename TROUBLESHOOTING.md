# Common Compiler Troubleshooting Guide

## Installation Issues

### Problem: "gcc: command not found"

**Solution**: Install GCC
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# Fedora/RHEL
sudo dnf install gcc

# Arch
sudo pacman -S gcc
```

### Problem: "nasm: command not found"

**Solution**: Install NASM assembler
```bash
# Ubuntu/Debian
sudo apt-get install nasm

# Fedora/RHEL
sudo dnf install nasm

# Arch
sudo pacman -S nasm
```

### Problem: "fatal error: bits/libc-header-start.h: No such file"

**Cause**: Missing 32-bit development libraries

**Solution**: Install 32-bit support
```bash
# Ubuntu/Debian
sudo apt-get install gcc-multilib

# Fedora/RHEL
sudo dnf install glibc-devel.i686 libgcc.i686
```

---

## Compilation Issues

### Problem: Compiler fails to build

**Error**:
```
gcc -o common common.c
common.c:15:10: fatal error: stdio.h: No such file or directory
```

**Solution**: Install build essentials
```bash
sudo apt-get install build-essential
```

### Problem: "Permission denied" when running compiler

**Solution**: Make compiler executable
```bash
chmod +x ./common
```

Or run directly:
```bash
gcc -o common common.c
./common source.cm output.asm
```

---

## Assembly Issues

### Problem: "error: invalid combination of opcode and operands"

**Cause**: NASM version incompatibility or corrupt assembly output

**Debug Steps**:
1. Check assembly output:
   ```bash
   ./common source.cm output.asm
   cat output.asm
   ```

2. Verify NASM version:
   ```bash
   nasm -version  # Should be 2.x or higher
   ```

3. Try manual assembly:
   ```bash
   nasm -f elf32 output.asm -o output.o
   ```

### Problem: "undefined reference to function_name"

**Cause**: Function called but not defined or linked

**Solutions**:

1. **Missing function definition**:
   ```c
   // Declare AND define the function
   int32 helper(int32 x) {
       return x * 2;
   }
   ```

2. **C library function not linked**:
   ```bash
   # Make sure you're linking with gcc
   gcc -m32 output.o -o program
   # NOT: ld output.o -o program
   ```

3. **External library needed**:
   ```bash
   gcc -m32 output.o -lm -o program  # Link math library
   ```

---

## Linker Issues

### Problem: "cannot find -lgcc_s"

**Cause**: Missing 32-bit GCC support libraries

**Solution**:
```bash
sudo apt-get install gcc-multilib
```

### Problem: "/usr/bin/ld: i386 architecture of input file is incompatible with i386:x86-64"

**Cause**: Mixing 32-bit and 64-bit object files

**Solution**: Ensure consistent 32-bit compilation:
```bash
nasm -f elf32 output.asm -o output.o  # Must be elf32
gcc -m32 output.o -o program           # Must use -m32
```

### Problem: "undefined reference to main"

**Cause**: No main function in source

**Solution**: Add main function:
```c
int32 main(void) {
    // Your code here
    return 0;
}
```

---

## Runtime Issues

### Problem: Segmentation fault

**Common Causes**:

1. **Null pointer dereference**:
   ```c
   int32 *ptr = 0;
   *ptr = 42;  // CRASH: dereferencing NULL
   ```
   
   **Fix**: Check pointers before dereferencing
   ```c
   if (ptr != 0) {
       *ptr = 42;
   }
   ```

2. **Array out of bounds**:
   ```c
   int32 arr[10];
   arr[10] = 5;  // CRASH: index 10 is out of bounds (0-9)
   ```
   
   **Fix**: Check array bounds
   ```c
   if (index < 10) {
       arr[index] = 5;
   }
   ```

3. **Stack overflow (infinite recursion)**:
   ```c
   int32 recurse(int32 n) {
       return recurse(n);  // CRASH: no base case
   }
   ```
   
   **Fix**: Add base case
   ```c
   int32 recurse(int32 n) {
       if (n <= 0) return 0;
       return recurse(n - 1);
   }
   ```

4. **Writing to read-only memory**:
   ```c
   uint8 *str = "constant";
   str[0] = 'C';  // CRASH: string literals are read-only
   ```
   
   **Fix**: Use array for modifiable strings
   ```c
   uint8 str[20];
   str[0] = 'C';  // OK
   ```

### Problem: Wrong output values

**Debug Steps**:

1. **Check integer overflow**:
   ```c
   int8 x = 127;
   x = x + 1;  // Wraps to -128
   ```

2. **Check division by zero**:
   ```c
   int32 result = 10 / 0;  // Undefined behavior
   ```
   
   **Fix**:
   ```c
   if (divisor != 0) {
       result = dividend / divisor;
   }
   ```

3. **Check type truncation**:
   ```c
   int32 large = 1000;
   uint8 small = (uint8)large;  // Truncated to 232 (1000 % 256)
   ```

### Problem: Program hangs / infinite loop

**Common Causes**:

1. **Loop condition never false**:
   ```c
   uint32 i = 10;
   while (i >= 0) {  // INFINITE: unsigned i never < 0
       i = i - 1;
   }
   ```
   
   **Fix**:
   ```c
   int32 i = 10;
   while (i >= 0) {
       i = i - 1;
   }
   ```

2. **Missing loop increment**:
   ```c
   for (int32 i = 0; i < 10; ) {  // Missing i++
       // ...
   }
   ```

---

## Compiler Error Messages

### "line N: syntax error near 'token'"

**Causes**:
- Missing semicolon
- Mismatched braces/parentheses
- Invalid expression syntax

**Debug**:
1. Check line N and surrounding lines
2. Look for missing `;` on previous line
3. Count braces: `{` should match `}`
4. Check operator usage

**Example**:
```c
int32 x = 10  // ERROR: missing semicolon
int32 y = 20;
```

### "line N: Unknown char 'X'"

**Cause**: Invalid character in source

**Common Examples**:
- Smart quotes: `"` `"` instead of `"`
- Non-ASCII characters
- Tab characters in wrong places

**Fix**: Use plain ASCII text editor

### "line N: expected expression"

**Cause**: Invalid or incomplete expression

**Example**:
```c
int32 x = ;      // ERROR: no expression after =
int32 y = + 5;   // ERROR: + needs left operand
```

### "too many locals"

**Cause**: More than 256 local variables in a function

**Solution**: 
1. Reduce number of variables
2. Use arrays instead of individual variables
3. Split into multiple functions

### "too many strings"

**Cause**: More than 512 string literals in program

**Solution**:
1. Reuse string literals
2. Build strings programmatically
3. Use character arrays

---

## Debugging Techniques

### Print Debugging

```c
void printf(uint8 *fmt, ...);

int32 main(void) {
    int32 x = 10;
    printf("x = %d\n", x);  // Print values
    
    int32 *ptr = &x;
    printf("ptr = %p, *ptr = %d\n", ptr, *ptr);  // Print pointers
    
    return 0;
}
```

### Check Assembly Output

```bash
./common source.cm output.asm
less output.asm  # Review generated assembly
```

Look for:
- Correct function labels
- Proper stack setup
- Expected instructions

### Use GDB

```bash
# Compile with debug info
gcc -m32 -g output.o -o program

# Run in debugger
gdb ./program

# GDB commands:
(gdb) break main      # Set breakpoint at main
(gdb) run             # Run program
(gdb) next            # Step to next line
(gdb) print x         # Print variable x
(gdb) backtrace       # Show call stack
(gdb) quit            # Exit gdb
```

### Valgrind (Memory Errors)

```bash
# Install valgrind
sudo apt-get install valgrind

# Run with valgrind
valgrind --leak-check=full ./program
```

---

## Common Mistakes

### 1. Assignment in Condition

**Wrong**:
```c
if (x = 5) {  // Assigns 5 to x, always true
    // ...
}
```

**Right**:
```c
if (x == 5) {  // Compares x to 5
    // ...
}
```

### 2. Infinite Loop with Unsigned

**Wrong**:
```c
for (uint32 i = 10; i >= 0; i--) {  // Infinite: unsigned never < 0
    // ...
}
```

**Right**:
```c
for (int32 i = 10; i >= 0; i--) {
    // ...
}
```

### 3. Pointer vs. Value

**Wrong**:
```c
void increment(int32 x) {
    x = x + 1;  // Only modifies local copy
}

int32 val = 5;
increment(val);
// val is still 5
```

**Right**:
```c
void increment(int32 *x) {
    *x = *x + 1;  // Modifies through pointer
}

int32 val = 5;
increment(&val);
// val is now 6
```

### 4. Array Decay

**Confusing**:
```c
int32 arr[10];
int32 *ptr = arr;  // arr decays to pointer

// arr and &arr are different:
arr        // Pointer to first element (type: int32*)
&arr       // Pointer to entire array (type: int32(*)[10])
```

### 5. String Modification

**Wrong**:
```c
uint8 *str = "Hello";
str[0] = 'h';  // CRASH: string literal is read-only
```

**Right**:
```c
uint8 str[20] = "Hello";  // Array, modifiable
str[0] = 'h';  // OK
```

---

## Performance Issues

### Slow Compilation

**Causes**:
- Very large source file
- Many string literals

**Solutions**:
- None (no optimization flags)
- Split code into modules (not supported in single-file compiler)

### Slow Execution

**Common Causes**:

1. **Inefficient algorithms**:
   ```c
   // O(n²) - slow
   for (int32 i = 0; i < n; i++)
       for (int32 j = 0; j < n; j++)
           // ...
   ```

2. **Excessive function calls**:
   ```c
   // Fibonacci - exponential time
   int32 fib(int32 n) {
       if (n <= 1) return n;
       return fib(n-1) + fib(n-2);  // Recomputes same values
   }
   ```
   
   **Fix**: Use iterative version or memoization

3. **No optimizations**: The compiler doesn't optimize. Write efficient code.

---

## Getting Help

### Information to Provide

When asking for help, include:

1. **Source code** (minimal example that reproduces issue)
2. **Compilation command** used
3. **Full error message** (copy-paste, not screenshot)
4. **System information**:
   ```bash
   uname -a
   gcc --version
   nasm -version
   ```

### Minimal Example

Reduce your code to the smallest program that shows the problem:

```c
// Minimal example showing segfault
int32 main(void) {
    int32 *ptr = 0;
    *ptr = 42;  // Crash here
    return 0;
}
```

### Check Examples First

Before reporting a bug, verify the example programs work:

```bash
make test
make examples
```

If examples work but your code doesn't, the issue is likely in your code, not the compiler.

---

## Known Issues

### 64-bit Types

**Issue**: 64-bit arithmetic truncates to 32 bits

```c
uint64 x = 5000000000;  // Stored as 64-bit
uint64 y = x * 2;       // Multiplied as 32-bit, overflow
```

**Workaround**: Use 32-bit types or implement 64-bit arithmetic manually

### Single File Limitation

**Issue**: Cannot split code across multiple files

**Workaround**: Put all code in one file, use forward declarations

### No Preprocessor

**Issue**: No `#define`, `#include`, etc.

**Workaround**: 
- Use const variables instead of #define
- Copy/paste shared code
- Write wrapper script to concatenate files

---

## Environment-Specific Issues

### WSL (Windows Subsystem for Linux)

Usually works fine. If issues:
```bash
sudo apt update
sudo apt install gcc-multilib nasm
```

### macOS

**Problem**: macOS doesn't support Linux ELF32

**Solution**: Use a Linux VM or Docker container

### 64-bit Only Systems

**Problem**: No 32-bit support installed

**Solution**: Install multilib packages (see Installation Issues above)

---

*For more help, see the full manual (MANUAL.md) or examples (examples/)*
