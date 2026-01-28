/**
 * Fern Runtime Library
 *
 * This library provides core functions for compiled Fern programs.
 * It is linked into every Fern executable.
 */

#ifndef FERN_RUNTIME_H
#define FERN_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

/* ========== I/O Functions ========== */

/**
 * Print an integer to stdout (no newline).
 * @param n The integer to print.
 */
void fern_print_int(int64_t n);

/**
 * Print an integer to stdout with newline.
 * @param n The integer to print.
 */
void fern_println_int(int64_t n);

/**
 * Print a string to stdout (no newline).
 * @param s The null-terminated string to print.
 */
void fern_print_str(const char* s);

/**
 * Print a string to stdout with newline.
 * @param s The null-terminated string to print.
 */
void fern_println_str(const char* s);

/**
 * Print a boolean to stdout (no newline).
 * @param b The boolean to print (0 = false, non-zero = true).
 */
void fern_print_bool(int64_t b);

/**
 * Print a boolean to stdout with newline.
 * @param b The boolean to print.
 */
void fern_println_bool(int64_t b);

/* ========== String Functions ========== */

/**
 * Get the length of a string.
 * @param s The null-terminated string.
 * @return The length in bytes.
 */
int64_t fern_str_len(const char* s);

/**
 * Concatenate two strings.
 * @param a First string.
 * @param b Second string.
 * @return Newly allocated concatenated string (caller must free).
 */
char* fern_str_concat(const char* a, const char* b);

/**
 * Compare two strings for equality.
 * @param a First string.
 * @param b Second string.
 * @return 1 if equal, 0 otherwise.
 */
int64_t fern_str_eq(const char* a, const char* b);

/* ========== List Functions ========== */

/**
 * Fern list structure.
 * Lists are immutable; operations return new lists.
 */
typedef struct FernList {
    int64_t* data;      /* Array of elements (int64_t for now) */
    int64_t len;        /* Number of elements */
    int64_t cap;        /* Allocated capacity */
} FernList;

/**
 * Create a new empty list.
 * @return Pointer to new list.
 */
FernList* fern_list_new(void);

/**
 * Create a list with given capacity.
 * @param cap Initial capacity.
 * @return Pointer to new list.
 */
FernList* fern_list_with_capacity(int64_t cap);

/**
 * Get the length of a list.
 * @param list The list.
 * @return Number of elements.
 */
int64_t fern_list_len(FernList* list);

/**
 * Get element at index.
 * @param list The list.
 * @param index The index (0-based).
 * @return The element value.
 */
int64_t fern_list_get(FernList* list, int64_t index);

/**
 * Append an element to a list (returns new list).
 * @param list The original list.
 * @param value The value to append.
 * @return New list with element appended.
 */
FernList* fern_list_push(FernList* list, int64_t value);

/**
 * Map a function over a list.
 * @param list The list.
 * @param fn Function pointer (int64_t -> int64_t).
 * @return New list with function applied to each element.
 */
FernList* fern_list_map(FernList* list, int64_t (*fn)(int64_t));

/**
 * Fold a list from left.
 * @param list The list.
 * @param init Initial accumulator value.
 * @param fn Function (acc, elem) -> acc.
 * @return Final accumulated value.
 */
int64_t fern_list_fold(FernList* list, int64_t init, int64_t (*fn)(int64_t, int64_t));

/**
 * Free a list.
 * @param list The list to free.
 */
void fern_list_free(FernList* list);

/* ========== Result Type ========== */

/**
 * Fern Result type.
 * Tag: 0 = Ok, 1 = Err
 * Value: The ok or err value (int64_t for simplicity).
 *
 * Represented as a packed 64-bit value:
 * - Upper 32 bits: value
 * - Lower 32 bits: tag (0 = Ok, 1 = Err)
 */

/**
 * Create an Ok result.
 * @param value The success value.
 * @return Packed Result value.
 */
int64_t fern_result_ok(int64_t value);

/**
 * Create an Err result.
 * @param value The error value.
 * @return Packed Result value.
 */
int64_t fern_result_err(int64_t value);

/**
 * Check if a Result is Ok.
 * @param result The packed Result value.
 * @return 1 if Ok, 0 if Err.
 */
int64_t fern_result_is_ok(int64_t result);

/**
 * Unwrap the value from a Result.
 * @param result The packed Result value.
 * @return The contained value (ok or err).
 */
int64_t fern_result_unwrap(int64_t result);

/**
 * Map a function over an Ok value.
 * @param result The Result.
 * @param fn Function to apply if Ok.
 * @return New Result with mapped value, or original Err.
 */
int64_t fern_result_map(int64_t result, int64_t (*fn)(int64_t));

/**
 * Chain a function that returns Result over an Ok value.
 * @param result The Result.
 * @param fn Function to apply if Ok (returns Result).
 * @return Result from fn if Ok, or original Err.
 */
int64_t fern_result_and_then(int64_t result, int64_t (*fn)(int64_t));

/**
 * Get the Ok value or a default.
 * @param result The Result.
 * @param default_val Value to return if Err.
 * @return Ok value or default.
 */
int64_t fern_result_unwrap_or(int64_t result, int64_t default_val);

/**
 * Get the Ok value or compute a default from the error.
 * @param result The Result.
 * @param fn Function to compute default from error.
 * @return Ok value or fn(err_value).
 */
int64_t fern_result_unwrap_or_else(int64_t result, int64_t (*fn)(int64_t));

/* ========== Option Type ========== */

/**
 * Fern Option type.
 * Tag: 0 = None, 1 = Some
 * Represented as packed 64-bit value (same as Result).
 */

/**
 * Create a Some option.
 * @param value The contained value.
 * @return Packed Option value.
 */
int64_t fern_option_some(int64_t value);

/**
 * Create a None option.
 * @return Packed Option value representing None.
 */
int64_t fern_option_none(void);

/**
 * Check if an Option is Some.
 * @param option The packed Option value.
 * @return 1 if Some, 0 if None.
 */
int64_t fern_option_is_some(int64_t option);

/**
 * Unwrap the value from an Option.
 * @param option The packed Option value.
 * @return The contained value (undefined if None).
 */
int64_t fern_option_unwrap(int64_t option);

/**
 * Map a function over a Some value.
 * @param option The Option.
 * @param fn Function to apply if Some.
 * @return New Option with mapped value, or None.
 */
int64_t fern_option_map(int64_t option, int64_t (*fn)(int64_t));

/**
 * Get the Some value or a default.
 * @param option The Option.
 * @param default_val Value to return if None.
 * @return Some value or default.
 */
int64_t fern_option_unwrap_or(int64_t option, int64_t default_val);

/* ========== Memory Functions ========== */

/**
 * Allocate memory.
 * @param size Number of bytes.
 * @return Pointer to allocated memory.
 */
void* fern_alloc(size_t size);

/**
 * Free memory.
 * @param ptr Pointer to free.
 */
void fern_free(void* ptr);

#endif /* FERN_RUNTIME_H */
