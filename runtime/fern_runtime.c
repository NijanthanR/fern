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
