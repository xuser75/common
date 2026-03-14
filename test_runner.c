/*
 * Public domain / CC0. Use freely for any purpose. RoyR 2026
 * test_runner.c - Test harness for Common compiler
 * 
 * Build: gcc -std=c99 -o test_runner test_runner.c
 * Usage: ./test_runner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    const char *name;
    const char *source;
    int expected_exit;
    const char *expected_output;
} Test;

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

static int run_command(const char *cmd) {
    int status = system(cmd);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}

static void run_test(Test *t) {
    char cmd[1024];
    test_count++;
    
    printf("Test %d: %s ... ", test_count, t->name);
    fflush(stdout);
    
    /* Write source file */
    FILE *f = fopen("/tmp/test.cm", "w");
    if (!f) { printf("FAIL (cannot write source)\n"); test_failed++; return; }
    fprintf(f, "%s", t->source);
    fclose(f);
    
    /* Compile */
    if (run_command("./common /tmp/test.cm /tmp/test.asm 2>/tmp/test.err") != 0) {
        printf("FAIL (compiler error)\n");
        test_failed++;
        return;
    }
    
    /* Assemble */
    if (run_command("nasm -f elf32 /tmp/test.asm -o /tmp/test.o 2>/tmp/test.err") != 0) {
        printf("FAIL (assembler error)\n");
        test_failed++;
        return;
    }
    
    /* Link */
    if (run_command("gcc -m32 /tmp/test.o -o /tmp/test 2>/tmp/test.err -no-pie") != 0) {
        printf("FAIL (linker error)\n");
        test_failed++;
        return;
    }
    
    /* Run and capture output */
    int exit_code = run_command("/tmp/test > /tmp/test.out 2>&1");
    
    /* Check exit code */
    if (exit_code != t->expected_exit) {
        printf("FAIL (exit=%d, expected=%d)\n", exit_code, t->expected_exit);
        test_failed++;
        return;
    }
    
    /* Check output if specified */
    if (t->expected_output) {
        char *output = read_file("/tmp/test.out");
        if (!output) {
            printf("FAIL (cannot read output)\n");
            test_failed++;
            return;
        }
        if (strcmp(output, t->expected_output) != 0) {
            printf("FAIL (output mismatch)\n");
            printf("  Expected: %s\n", t->expected_output);
            printf("  Got:      %s\n", output);
            free(output);
            test_failed++;
            return;
        }
        free(output);
    }
    
    printf("PASS\n");
    test_passed++;
}

/* ============================================
   TEST CASES
   ============================================ */

Test tests[] = {
    /* Basic arithmetic */
    {
        "simple_return",
        "int32 main(void) { return 42; }",
        42,
        NULL
    },
    
    {
        "addition",
        "int32 main(void) { return 10 + 32; }",
        42,
        NULL
    },
    
    {
        "subtraction",
        "int32 main(void) { return 50 - 8; }",
        42,
        NULL
    },
    
    {
        "multiplication",
        "int32 main(void) { return 6 * 7; }",
        42,
        NULL
    },
    
    {
        "division",
        "int32 main(void) { return 84 / 2; }",
        42,
        NULL
    },
    
    {
        "modulo",
        "int32 main(void) { return 142 % 100; }",
        42,
        NULL
    },
    
    /* Variables */
    {
        "local_variable",
        "int32 main(void) { int32 x; x = 42; return x; }",
        42,
        NULL
    },
    
    {
        "variable_with_init",
        "int32 main(void) { int32 x = 42; return x; }",
        42,
        NULL
    },
    
    {
        "multiple_variables",
        "int32 main(void) { int32 x = 10; int32 y = 32; return x + y; }",
        42,
        NULL
    },
    
    /* Global variables */
    {
        "global_variable",
        "int32 g = 42;\n"
        "int32 main(void) { return g; }",
        42,
        NULL
    },
    
    {
        "global_read_write",
        "int32 g;\n"
        "int32 main(void) { g = 42; return g; }",
        42,
        NULL
    },
    
    /* Control flow */
    {
        "if_true",
        "int32 main(void) { if (1) return 42; return 0; }",
        42,
        NULL
    },
    
    {
        "if_false",
        "int32 main(void) { if (0) return 0; return 42; }",
        42,
        NULL
    },
    
    {
        "if_else",
        "int32 main(void) { if (0) return 0; else return 42; }",
        42,
        NULL
    },
    
    {
        "while_loop",
        "int32 main(void) {\n"
        "  int32 x = 0;\n"
        "  while (x < 42) x = x + 1;\n"
        "  return x;\n"
        "}",
        42,
        NULL
    },
    
    {
        "for_loop",
        "int32 main(void) {\n"
        "  int32 sum = 0;\n"
        "  for (int32 i = 0; i < 10; i = i + 1) sum = sum + i;\n"
        "  return sum;\n"
        "}",
        45,
        NULL
    },
    
    /* Comparisons */
    {
        "eq_true",
        "int32 main(void) { return 42 == 42; }",
        1,
        NULL
    },
    
    {
        "eq_false",
        "int32 main(void) { return 42 == 43; }",
        0,
        NULL
    },
    
    {
        "neq_true",
        "int32 main(void) { return 42 != 43; }",
        1,
        NULL
    },
    
    {
        "lt_true",
        "int32 main(void) { return 10 < 42; }",
        1,
        NULL
    },
    
    {
        "gt_true",
        "int32 main(void) { return 42 > 10; }",
        1,
        NULL
    },
    
    /* Logical operators */
    {
        "and_true",
        "int32 main(void) { return 1 && 1; }",
        1,
        NULL
    },
    
    {
        "and_false",
        "int32 main(void) { return 1 && 0; }",
        0,
        NULL
    },
    
    {
        "or_true",
        "int32 main(void) { return 0 || 1; }",
        1,
        NULL
    },
    
    {
        "or_false",
        "int32 main(void) { return 0 || 0; }",
        0,
        NULL
    },
    
    /* Bitwise operators */
    {
        "bitwise_and",
        "int32 main(void) { return 63 & 42; }",
        42,
        NULL
    },
    
    {
        "bitwise_or",
        "int32 main(void) { return 32 | 10; }",
        42,
        NULL
    },
    
    {
        "bitwise_xor",
        "int32 main(void) { return 50 ^ 24; }",
        42,
        NULL
    },
    
    {
        "left_shift",
        "int32 main(void) { return 21 << 1; }",
        42,
        NULL
    },
    
    {
        "right_shift",
        "int32 main(void) { return 84 >> 1; }",
        42,
        NULL
    },
    
    /* Unary operators */
    {
        "negation",
        "int32 main(void) { return -(-42); }",
        42,
        NULL
    },
    
    {
        "logical_not",
        "int32 main(void) { return !0; }",
        1,
        NULL
    },
    
    {
        "bitwise_not",
        "int32 main(void) { return ~(-43); }",
        42,
        NULL
    },
    
    /* Increment/decrement */
    {
        "post_increment",
        "int32 main(void) { int32 x = 41; int32 y = x++; return x; }",
        42,
        NULL
    },
    
    {
        "pre_increment",
        "int32 main(void) { int32 x = 41; int32 y = ++x; return x; }",
        42,
        NULL
    },
    
    {
        "post_decrement",
        "int32 main(void) { int32 x = 43; int32 y = x--; return x; }",
        42,
        NULL
    },
    
    /* Compound assignment */
    {
        "add_assign",
        "int32 main(void) { int32 x = 10; x += 32; return x; }",
        42,
        NULL
    },
    
    {
        "sub_assign",
        "int32 main(void) { int32 x = 50; x -= 8; return x; }",
        42,
        NULL
    },
    
    /* Ternary operator */
    {
        "ternary_true",
        "int32 main(void) { return 1 ? 42 : 0; }",
        42,
        NULL
    },
    
    {
        "ternary_false",
        "int32 main(void) { return 0 ? 0 : 42; }",
        42,
        NULL
    },
    
    /* Functions */
    {
        "function_call",
        "int32 add(int32 a, int32 b) { return a + b; }\n"
        "int32 main(void) { return add(10, 32); }",
        42,
        NULL
    },
    
    {
        "recursive_function",
        "int32 fib(int32 n) {\n"
        "  if (n <= 1) return n;\n"
        "  return fib(n - 1) + fib(n - 2);\n"
        "}\n"
        "int32 main(void) { return fib(10); }",
        55,
        NULL
    },
    
    /* Arrays */
    {
        "local_array",
        "int32 main(void) {\n"
        "  int32 arr[5];\n"
        "  arr[0] = 42;\n"
        "  return arr[0];\n"
        "}",
        42,
        NULL
    },
    
    {
        "array_init",
        "int32 main(void) {\n"
        "  int32 arr[3] = { 10, 32, 99 };\n"
        "  return arr[0] + arr[1];\n"
        "}",
        42,
        NULL
    },
    
    {
        "global_array",
        "int32 arr[3] = { 10, 32, 99 };\n"
        "int32 main(void) { return arr[0] + arr[1]; }",
        42,
        NULL
    },
    
    /* Pointers */
    {
        "address_and_deref",
        "int32 main(void) {\n"
        "  int32 x = 42;\n"
        "  int32 *p = &x;\n"
        "  return *p;\n"
        "}",
        42,
        NULL
    },
    
    {
        "pointer_assignment",
        "int32 main(void) {\n"
        "  int32 x = 0;\n"
        "  int32 *p = &x;\n"
        "  *p = 42;\n"
        "  return x;\n"
        "}",
        42,
        NULL
    },
    
    /* Type casting */
    {
        "cast_to_uint8",
        "int32 main(void) { return (uint8)42; }",
        42,
        NULL
    },
    
    {
        "cast_truncate",
        "int32 main(void) { return (uint8)298; }",
        42,
        NULL
    },
    
    /* Different integer sizes */
    {
        "uint8_type",
        "int32 main(void) { uint8 x = 42; return x; }",
        42,
        NULL
    },
    
    {
        "uint16_type",
        "int32 main(void) { uint16 x = 42; return x; }",
        42,
        NULL
    },
    
    {
        "int8_signed",
        "int32 main(void) { int8 x = -42; return -x; }",
        42,
        NULL
    },
    
    /* Switch statement */
    {
        "switch_basic",
        "int32 main(void) {\n"
        "  int32 x = 2;\n"
        "  switch (x) {\n"
        "    case 1: return 10;\n"
        "    case 2: return 42;\n"
        "    case 3: return 20;\n"
        "  }\n"
        "  return 0;\n"
        "}",
        42,
        NULL
    },
    
    {
        "switch_default",
        "int32 main(void) {\n"
        "  int32 x = 99;\n"
        "  switch (x) {\n"
        "    case 1: return 10;\n"
        "    default: return 42;\n"
        "  }\n"
        "}",
        42,
        NULL
    },
    
    /* Break/continue */
    {
        "break_loop",
        "int32 main(void) {\n"
        "  int32 x = 0;\n"
        "  while (1) {\n"
        "    x = x + 1;\n"
        "    if (x == 42) break;\n"
        "  }\n"
        "  return x;\n"
        "}",
        42,
        NULL
    },
    
    {
        "continue_loop",
        "int32 main(void) {\n"
        "  int32 x = 0;\n"
        "  int32 sum = 0;\n"
        "  while (x < 50) {\n"
        "    x = x + 1;\n"
        "    if (x > 42) continue;\n"
        "    sum = sum + 1;\n"
        "  }\n"
        "  return sum;\n"
        "}",
        42,
        NULL
    },
    
    /* End marker */
    { NULL, NULL, 0, NULL }
};

int main(void) {
    printf("Common Compiler Test Suite\n");
    printf("===========================\n\n");
    
    /* Check if compiler exists */
    if (access("./common", X_OK) != 0) {
        fprintf(stderr, "Error: ./common not found or not executable\n");
        fprintf(stderr, "Please build it first: gcc -o common common.c\n");
        return 1;
    }
    
    /* Run all tests */
    for (int i = 0; tests[i].name != NULL; i++) {
        run_test(&tests[i]);
    }
    
    /* Summary */
    printf("\n===========================\n");
    printf("Total:  %d\n", test_count);
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("===========================\n");
    
    return test_failed > 0 ? 1 : 0;
}
