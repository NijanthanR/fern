/* Fern Runtime Library
 *
 * Provides runtime support for Fern programs:
 * - Result type (Ok/Err tagged union)
 * - String operations
 * - List operations
 * - Memory allocation
 */

#ifndef FERN_RUNTIME_H
#define FERN_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ========== Result Type ========== */

/* Result tag values */
#define FERN_OK  0
#define FERN_ERR 1

/* Result type - a tagged union
 * Layout: [tag: i32][padding: i32][value: i64]
 * Total size: 16 bytes (aligned)
 */
typedef struct {
    int32_t tag;      /* 0 = Ok, 1 = Err */
    int32_t _pad;     /* Padding for alignment */
    int64_t value;    /* The Ok or Err value */
} FernResult;

/* Create an Ok result */
static inline FernResult fern_ok(int64_t value) {
    return (FernResult){ .tag = FERN_OK, ._pad = 0, .value = value };
}

/* Create an Err result */
static inline FernResult fern_err(int64_t error) {
    return (FernResult){ .tag = FERN_ERR, ._pad = 0, .value = error };
}

/* Check if result is Ok */
static inline bool fern_is_ok(FernResult r) {
    return r.tag == FERN_OK;
}

/* Check if result is Err */
static inline bool fern_is_err(FernResult r) {
    return r.tag == FERN_ERR;
}

/* Unwrap Ok value (undefined behavior if Err) */
static inline int64_t fern_unwrap(FernResult r) {
    return r.value;
}

/* ========== String Type ========== */

/* Fern strings are length-prefixed
 * Layout: [length: i64][data: char*]
 */
typedef struct {
    int64_t length;
    char* data;
} FernString;

/* Create a string from C string literal */
FernString* fern_string_new(const char* cstr);

/* Get string length */
int64_t fern_string_len(FernString* s);

/* Concatenate two strings */
FernString* fern_string_concat(FernString* a, FernString* b);

/* Compare strings for equality */
bool fern_string_eq(FernString* a, FernString* b);

/* ========== List Type ========== */

/* Fern lists are length + capacity + data
 * Layout: [length: i64][capacity: i64][data: void*]
 */
typedef struct {
    int64_t length;
    int64_t capacity;
    void* data;
} FernList;

/* Create an empty list */
FernList* fern_list_new(void);

/* Get list length */
int64_t fern_list_len(FernList* list);

/* Push an element to the list */
void fern_list_push(FernList* list, int64_t elem);

/* Get element at index */
int64_t fern_list_get(FernList* list, int64_t index);

/* ========== Memory ========== */

/* Allocate memory (uses malloc, to be replaced with GC) */
void* fern_alloc(size_t size);

/* Free memory */
void fern_free(void* ptr);

/* ========== Panic ========== */

/* Panic with message (for unwrap failures, etc.) */
void fern_panic(const char* msg) __attribute__((noreturn));

#endif /* FERN_RUNTIME_H */
