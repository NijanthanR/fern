/* Code Generator Tests */

#include "test.h"
#include "codegen.h"
#include "parser.h"
#include "arena.h"
#include "fern_string.h"
#include <string.h>

/* Helper to generate QBE from source code */
static const char* generate_qbe(Arena* arena, const char* src) {
    Parser* parser = parser_new(arena, src);
    StmtVec* stmts = parse_stmts(parser);
    if (!stmts || parser->had_error) return NULL;
    
    Codegen* cg = codegen_new(arena);
    codegen_program(cg, stmts);
    return string_cstr(codegen_output(cg));
}

/* Helper to generate QBE for a single expression */
static const char* generate_expr_qbe(Arena* arena, const char* src) {
    Parser* parser = parser_new(arena, src);
    Expr* expr = parse_expr(parser);
    if (!expr || parser->had_error) return NULL;
    
    Codegen* cg = codegen_new(arena);
    codegen_expr(cg, expr);
    return string_cstr(codegen_output(cg));
}

/* ========== Integer Literal Tests ========== */

void test_codegen_int_literal(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "42");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "copy 42") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_negative_int(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "-5");
    
    ASSERT_NOT_NULL(qbe);
    /* -5 is parsed as unary negation of 5 */
    ASSERT_TRUE(strstr(qbe, "copy 5") != NULL);
    ASSERT_TRUE(strstr(qbe, "sub 0") != NULL);
    
    arena_destroy(arena);
}

/* ========== Binary Operation Tests ========== */

void test_codegen_add(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "1 + 2");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "copy 1") != NULL);
    ASSERT_TRUE(strstr(qbe, "copy 2") != NULL);
    ASSERT_TRUE(strstr(qbe, "add") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_sub(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "10 - 3");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "sub") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_mul(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "4 * 5");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "mul") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_div(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "20 / 4");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "div") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_complex_expr(void) {
    Arena* arena = arena_create(4096);
    
    /* (1 + 2) * 3 */
    const char* qbe = generate_expr_qbe(arena, "(1 + 2) * 3");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "add") != NULL);
    ASSERT_TRUE(strstr(qbe, "mul") != NULL);
    
    arena_destroy(arena);
}

/* ========== Comparison Tests ========== */

void test_codegen_eq(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "1 == 2");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "ceqw") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_lt(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "1 < 2");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "csltw") != NULL);
    
    arena_destroy(arena);
}

/* ========== Function Definition Tests ========== */

void test_codegen_fn_simple(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_qbe(arena, "fn answer() -> Int: 42");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "function") != NULL);
    ASSERT_TRUE(strstr(qbe, "$answer") != NULL);
    ASSERT_TRUE(strstr(qbe, "ret") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_fn_with_params(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_qbe(arena, "fn add(a: Int, b: Int) -> Int: a + b");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "$add") != NULL);
    ASSERT_TRUE(strstr(qbe, "%a") != NULL);
    ASSERT_TRUE(strstr(qbe, "%b") != NULL);
    ASSERT_TRUE(strstr(qbe, "add") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_fn_call(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "add(1, 2)");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "call $add") != NULL);
    
    arena_destroy(arena);
}

/* ========== If Expression Tests ========== */

void test_codegen_if_expr(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "if true: 1 else: 0");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "jnz") != NULL);  /* Conditional jump */
    ASSERT_TRUE(strstr(qbe, "@L") != NULL);   /* Labels */
    
    arena_destroy(arena);
}

/* ========== Let Statement Tests ========== */

void test_codegen_let(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_qbe(arena, "let x = 42");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "%x") != NULL);
    ASSERT_TRUE(strstr(qbe, "copy") != NULL);
    
    arena_destroy(arena);
}

/* ========== String Tests ========== */

void test_codegen_string_literal(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "\"hello\"");
    
    ASSERT_NOT_NULL(qbe);
    /* String literals should create a data section */
    ASSERT_TRUE(strstr(qbe, "data") != NULL);
    ASSERT_TRUE(strstr(qbe, "hello") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_string_in_fn(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_qbe(arena, "fn greet() -> String: \"hello\"");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "$greet") != NULL);
    ASSERT_TRUE(strstr(qbe, "data") != NULL);
    
    arena_destroy(arena);
}

/* ========== Boolean Tests ========== */

void test_codegen_bool_true(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "true");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "copy 1") != NULL);
    
    arena_destroy(arena);
}

void test_codegen_bool_false(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "false");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "copy 0") != NULL);
    
    arena_destroy(arena);
}

/* ========== Float Tests ========== */

void test_codegen_float_literal(void) {
    Arena* arena = arena_create(4096);
    
    const char* qbe = generate_expr_qbe(arena, "3.14");
    
    ASSERT_NOT_NULL(qbe);
    /* Float literals use 'd' (double) type in QBE */
    ASSERT_TRUE(strstr(qbe, "3.14") != NULL);
    
    arena_destroy(arena);
}

/* ========== Match Expression Tests ========== */

void test_codegen_match_int(void) {
    Arena* arena = arena_create(4096);
    
    /* match x: 1 -> 10, 2 -> 20, _ -> 0 */
    const char* qbe = generate_qbe(arena, 
        "fn test(x: Int) -> Int: match x: 1 -> 10, 2 -> 20, _ -> 0");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "$test") != NULL);
    /* Should have comparison and jumps */
    ASSERT_TRUE(strstr(qbe, "ceqw") != NULL);  /* compare equal */
    ASSERT_TRUE(strstr(qbe, "jnz") != NULL);   /* conditional jump */
    
    arena_destroy(arena);
}

void test_codegen_match_wildcard(void) {
    Arena* arena = arena_create(4096);
    
    /* Simple match with wildcard */
    const char* qbe = generate_qbe(arena,
        "fn always_zero(x: Int) -> Int: match x: _ -> 0");
    
    ASSERT_NOT_NULL(qbe);
    ASSERT_TRUE(strstr(qbe, "copy 0") != NULL);
    
    arena_destroy(arena);
}

/* ========== Tuple Tests ========== */

void test_codegen_tuple(void) {
    Arena* arena = arena_create(4096);
    
    /* Simple tuple */
    const char* qbe = generate_expr_qbe(arena, "(1, 2, 3)");
    
    ASSERT_NOT_NULL(qbe);
    /* Tuple elements should be generated */
    ASSERT_TRUE(strstr(qbe, "copy 1") != NULL);
    ASSERT_TRUE(strstr(qbe, "copy 2") != NULL);
    ASSERT_TRUE(strstr(qbe, "copy 3") != NULL);
    
    arena_destroy(arena);
}

/* ========== List Tests ========== */

void test_codegen_list(void) {
    Arena* arena = arena_create(4096);
    
    /* Simple list */
    const char* qbe = generate_expr_qbe(arena, "[1, 2, 3]");
    
    ASSERT_NOT_NULL(qbe);
    /* List elements should be generated */
    ASSERT_TRUE(strstr(qbe, "copy 1") != NULL);
    ASSERT_TRUE(strstr(qbe, "copy 2") != NULL);
    ASSERT_TRUE(strstr(qbe, "copy 3") != NULL);
    
    arena_destroy(arena);
}

/* ========== Lambda Tests ========== */

void test_codegen_lambda(void) {
    Arena* arena = arena_create(4096);
    
    /* Lambda that adds 1 to its argument */
    const char* qbe = generate_expr_qbe(arena, "(x) -> x + 1");
    
    ASSERT_NOT_NULL(qbe);
    /* Lambda should generate a function */
    ASSERT_TRUE(strstr(qbe, "function") != NULL);
    ASSERT_TRUE(strstr(qbe, "add") != NULL);
    
    arena_destroy(arena);
}

/* ========== Test Runner ========== */

void run_codegen_tests(void) {
    printf("\n--- Code Generator Tests ---\n");
    
    /* Integer literals */
    TEST_RUN(test_codegen_int_literal);
    TEST_RUN(test_codegen_negative_int);
    
    /* Binary operations */
    TEST_RUN(test_codegen_add);
    TEST_RUN(test_codegen_sub);
    TEST_RUN(test_codegen_mul);
    TEST_RUN(test_codegen_div);
    TEST_RUN(test_codegen_complex_expr);
    
    /* Comparisons */
    TEST_RUN(test_codegen_eq);
    TEST_RUN(test_codegen_lt);
    
    /* Functions */
    TEST_RUN(test_codegen_fn_simple);
    TEST_RUN(test_codegen_fn_with_params);
    TEST_RUN(test_codegen_fn_call);
    
    /* Control flow */
    TEST_RUN(test_codegen_if_expr);
    
    /* Statements */
    TEST_RUN(test_codegen_let);
    
    /* Strings */
    TEST_RUN(test_codegen_string_literal);
    TEST_RUN(test_codegen_string_in_fn);
    
    /* Booleans */
    TEST_RUN(test_codegen_bool_true);
    TEST_RUN(test_codegen_bool_false);
    
    /* Float literals */
    TEST_RUN(test_codegen_float_literal);
    
    /* Match expressions */
    TEST_RUN(test_codegen_match_int);
    TEST_RUN(test_codegen_match_wildcard);
    
    /* Tuple expressions */
    TEST_RUN(test_codegen_tuple);
    
    /* List expressions */
    TEST_RUN(test_codegen_list);
    
    /* Lambda expressions */
    TEST_RUN(test_codegen_lambda);
}
