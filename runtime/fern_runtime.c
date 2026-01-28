/**
 * Fern Runtime Library Implementation
 *
 * Core functions for compiled Fern programs.
 */

#include "fern_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ========== I/O Functions ========== */

/**
 * Print an integer to stdout (no newline).
 * @param n The integer to print.
 */
void fern_print_int(int64_t n) {
    printf("%lld", (long long)n);
}

/**
 * Print an integer to stdout with newline.
 * @param n The integer to print.
 */
void fern_println_int(int64_t n) {
    printf("%lld\n", (long long)n);
}

/**
 * Print a string to stdout (no newline).
 * @param s The null-terminated string to print.
 */
void fern_print_str(const char* s) {
    assert(s != NULL);
    printf("%s", s);
}

/**
 * Print a string to stdout with newline.
 * @param s The null-terminated string to print.
 */
void fern_println_str(const char* s) {
    assert(s != NULL);
    printf("%s\n", s);
}

/**
 * Print a boolean to stdout (no newline).
 * @param b The boolean to print (0 = false, non-zero = true).
 */
void fern_print_bool(int64_t b) {
    printf("%s", b ? "true" : "false");
}

/**
 * Print a boolean to stdout with newline.
 * @param b The boolean to print.
 */
void fern_println_bool(int64_t b) {
    printf("%s\n", b ? "true" : "false");
}

/* ========== String Functions ========== */

/**
 * Get the length of a string.
 * @param s The null-terminated string.
 * @return The length in bytes.
 */
int64_t fern_str_len(const char* s) {
    assert(s != NULL);
    return (int64_t)strlen(s);
}

/**
 * Concatenate two strings.
 * @param a First string.
 * @param b Second string.
 * @return Newly allocated concatenated string (caller must free).
 */
char* fern_str_concat(const char* a, const char* b) {
    assert(a != NULL);
    assert(b != NULL);
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char* result = malloc(len_a + len_b + 1);
    assert(result != NULL);
    memcpy(result, a, len_a);
    memcpy(result + len_a, b, len_b + 1);
    return result;
}

/**
 * Compare two strings for equality.
 * @param a First string.
 * @param b Second string.
 * @return 1 if equal, 0 otherwise.
 */
int64_t fern_str_eq(const char* a, const char* b) {
    assert(a != NULL);
    assert(b != NULL);
    return strcmp(a, b) == 0 ? 1 : 0;
}

/* ========== List Functions ========== */

/**
 * Create a new empty list.
 * @return Pointer to new list.
 */
FernList* fern_list_new(void) {
    return fern_list_with_capacity(8);
}

/**
 * Create a list with given capacity.
 * @param cap Initial capacity.
 * @return Pointer to new list.
 */
FernList* fern_list_with_capacity(int64_t cap) {
    assert(cap > 0);
    FernList* list = malloc(sizeof(FernList));
    assert(list != NULL);
    list->data = malloc((size_t)cap * sizeof(int64_t));
    assert(list->data != NULL);
    list->len = 0;
    list->cap = cap;
    return list;
}

/**
 * Get the length of a list.
 * @param list The list.
 * @return Number of elements.
 */
int64_t fern_list_len(FernList* list) {
    assert(list != NULL);
    return list->len;
}

/**
 * Get element at index.
 * @param list The list.
 * @param index The index (0-based).
 * @return The element value.
 */
int64_t fern_list_get(FernList* list, int64_t index) {
    assert(list != NULL);
    assert(index >= 0 && index < list->len);
    return list->data[index];
}

/**
 * Append an element to a list (returns new list).
 * @param list The original list.
 * @param value The value to append.
 * @return New list with element appended.
 */
FernList* fern_list_push(FernList* list, int64_t value) {
    assert(list != NULL);

    /* Create new list with copied data */
    int64_t new_cap = list->len + 1;
    if (new_cap < list->cap) {
        new_cap = list->cap;
    }
    FernList* new_list = fern_list_with_capacity(new_cap);
    assert(new_list != NULL);

    /* Copy existing elements */
    for (int64_t i = 0; i < list->len; i++) {
        new_list->data[i] = list->data[i];
    }
    new_list->data[list->len] = value;
    new_list->len = list->len + 1;

    return new_list;
}

/**
 * Map a function over a list.
 * @param list The list.
 * @param fn Function pointer (int64_t -> int64_t).
 * @return New list with function applied to each element.
 */
FernList* fern_list_map(FernList* list, int64_t (*fn)(int64_t)) {
    assert(list != NULL);
    assert(fn != NULL);

    FernList* new_list = fern_list_with_capacity(list->len > 0 ? list->len : 1);
    assert(new_list != NULL);

    for (int64_t i = 0; i < list->len; i++) {
        new_list->data[i] = fn(list->data[i]);
    }
    new_list->len = list->len;

    return new_list;
}

/**
 * Fold a list from left.
 * @param list The list.
 * @param init Initial accumulator value.
 * @param fn Function (acc, elem) -> acc.
 * @return Final accumulated value.
 */
int64_t fern_list_fold(FernList* list, int64_t init, int64_t (*fn)(int64_t, int64_t)) {
    assert(list != NULL);
    assert(fn != NULL);

    int64_t acc = init;
    for (int64_t i = 0; i < list->len; i++) {
        acc = fn(acc, list->data[i]);
    }
    return acc;
}

/**
 * Free a list.
 * @param list The list to free.
 */
void fern_list_free(FernList* list) {
    if (list != NULL) {
        free(list->data);
        free(list);
    }
}

/* ========== Memory Functions ========== */

/**
 * Allocate memory.
 * @param size Number of bytes.
 * @return Pointer to allocated memory.
 */
void* fern_alloc(size_t size) {
    void* ptr = malloc(size);
    assert(ptr != NULL);
    return ptr;
}

/**
 * Free memory.
 * @param ptr Pointer to free.
 */
void fern_free(void* ptr) {
    free(ptr);
}

/* ========== Result Type ========== */

/*
 * Result encoding (packed 64-bit):
 * - Bits 0-31: tag (0 = Ok, 1 = Err)
 * - Bits 32-63: value
 */

#define RESULT_TAG_OK  0
#define RESULT_TAG_ERR 1

/**
 * Create an Ok result.
 * @param value The success value.
 * @return Packed Result value.
 */
int64_t fern_result_ok(int64_t value) {
    /* Pack: value in upper 32 bits, tag=0 in lower 32 bits */
    return ((value & 0xFFFFFFFF) << 32) | RESULT_TAG_OK;
}

/**
 * Create an Err result.
 * @param value The error value.
 * @return Packed Result value.
 */
int64_t fern_result_err(int64_t value) {
    /* Pack: value in upper 32 bits, tag=1 in lower 32 bits */
    return ((value & 0xFFFFFFFF) << 32) | RESULT_TAG_ERR;
}

/**
 * Check if a Result is Ok.
 * @param result The packed Result value.
 * @return 1 if Ok, 0 if Err.
 */
int64_t fern_result_is_ok(int64_t result) {
    return (result & 0xFFFFFFFF) == RESULT_TAG_OK ? 1 : 0;
}

/**
 * Unwrap the value from a Result.
 * @param result The packed Result value.
 * @return The contained value (ok or err).
 */
int64_t fern_result_unwrap(int64_t result) {
    /* Extract value from upper 32 bits, sign-extend */
    return (int64_t)((int32_t)(result >> 32));
}

/**
 * Map a function over an Ok value.
 * @param result The Result.
 * @param fn Function to apply if Ok.
 * @return New Result with mapped value, or original Err.
 */
int64_t fern_result_map(int64_t result, int64_t (*fn)(int64_t)) {
    assert(fn != NULL);
    if (fern_result_is_ok(result)) {
        int64_t value = fern_result_unwrap(result);
        return fern_result_ok(fn(value));
    }
    return result;
}

/**
 * Chain a function that returns Result over an Ok value.
 * @param result The Result.
 * @param fn Function to apply if Ok (returns Result).
 * @return Result from fn if Ok, or original Err.
 */
int64_t fern_result_and_then(int64_t result, int64_t (*fn)(int64_t)) {
    assert(fn != NULL);
    if (fern_result_is_ok(result)) {
        int64_t value = fern_result_unwrap(result);
        return fn(value);
    }
    return result;
}

/**
 * Get the Ok value or a default.
 * @param result The Result.
 * @param default_val Value to return if Err.
 * @return Ok value or default.
 */
int64_t fern_result_unwrap_or(int64_t result, int64_t default_val) {
    if (fern_result_is_ok(result)) {
        return fern_result_unwrap(result);
    }
    return default_val;
}

/**
 * Get the Ok value or compute a default from the error.
 * @param result The Result.
 * @param fn Function to compute default from error.
 * @return Ok value or fn(err_value).
 */
int64_t fern_result_unwrap_or_else(int64_t result, int64_t (*fn)(int64_t)) {
    assert(fn != NULL);
    if (fern_result_is_ok(result)) {
        return fern_result_unwrap(result);
    }
    return fn(fern_result_unwrap(result));
}

/* ========== Option Type ========== */

/*
 * Option encoding (packed 64-bit):
 * - Bits 0-31: tag (0 = None, 1 = Some)
 * - Bits 32-63: value (only meaningful if Some)
 */

#define OPTION_TAG_NONE 0
#define OPTION_TAG_SOME 1

/**
 * Create a Some option.
 * @param value The contained value.
 * @return Packed Option value.
 */
int64_t fern_option_some(int64_t value) {
    return ((value & 0xFFFFFFFF) << 32) | OPTION_TAG_SOME;
}

/**
 * Create a None option.
 * @return Packed Option value representing None.
 */
int64_t fern_option_none(void) {
    return OPTION_TAG_NONE;
}

/**
 * Check if an Option is Some.
 * @param option The packed Option value.
 * @return 1 if Some, 0 if None.
 */
int64_t fern_option_is_some(int64_t option) {
    return (option & 0xFFFFFFFF) == OPTION_TAG_SOME ? 1 : 0;
}

/**
 * Unwrap the value from an Option.
 * @param option The packed Option value.
 * @return The contained value (undefined if None).
 */
int64_t fern_option_unwrap(int64_t option) {
    return (int64_t)((int32_t)(option >> 32));
}

/**
 * Map a function over a Some value.
 * @param option The Option.
 * @param fn Function to apply if Some.
 * @return New Option with mapped value, or None.
 */
int64_t fern_option_map(int64_t option, int64_t (*fn)(int64_t)) {
    assert(fn != NULL);
    if (fern_option_is_some(option)) {
        int64_t value = fern_option_unwrap(option);
        return fern_option_some(fn(value));
    }
    return option;
}

/**
 * Get the Some value or a default.
 * @param option The Option.
 * @param default_val Value to return if None.
 * @return Some value or default.
 */
int64_t fern_option_unwrap_or(int64_t option, int64_t default_val) {
    if (fern_option_is_some(option)) {
        return fern_option_unwrap(option);
    }
    return default_val;
}
