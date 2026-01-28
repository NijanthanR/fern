/* Fern Runtime Library Implementation */

#include "fern_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========== Memory ========== */

void* fern_alloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fern_panic("out of memory");
    }
    return ptr;
}

void fern_free(void* ptr) {
    free(ptr);
}

/* ========== Panic ========== */

void fern_panic(const char* msg) {
    fprintf(stderr, "panic: %s\n", msg);
    exit(1);
}

/* ========== String Type ========== */

FernString* fern_string_new(const char* cstr) {
    size_t len = strlen(cstr);
    FernString* s = fern_alloc(sizeof(FernString));
    s->length = (int64_t)len;
    s->data = fern_alloc(len + 1);
    memcpy(s->data, cstr, len + 1);
    return s;
}

int64_t fern_string_len(FernString* s) {
    return s ? s->length : 0;
}

FernString* fern_string_concat(FernString* a, FernString* b) {
    if (!a || !b) return NULL;
    
    size_t new_len = (size_t)(a->length + b->length);
    FernString* s = fern_alloc(sizeof(FernString));
    s->length = (int64_t)new_len;
    s->data = fern_alloc(new_len + 1);
    memcpy(s->data, a->data, (size_t)a->length);
    memcpy(s->data + a->length, b->data, (size_t)b->length);
    s->data[new_len] = '\0';
    return s;
}

bool fern_string_eq(FernString* a, FernString* b) {
    if (!a || !b) return a == b;
    if (a->length != b->length) return false;
    return memcmp(a->data, b->data, (size_t)a->length) == 0;
}

/* ========== List Type ========== */

#define INITIAL_CAPACITY 8

FernList* fern_list_new(void) {
    FernList* list = fern_alloc(sizeof(FernList));
    list->length = 0;
    list->capacity = INITIAL_CAPACITY;
    list->data = fern_alloc(INITIAL_CAPACITY * sizeof(int64_t));
    return list;
}

int64_t fern_list_len(FernList* list) {
    return list ? list->length : 0;
}

void fern_list_push(FernList* list, int64_t elem) {
    if (!list) return;
    
    /* Grow if needed */
    if (list->length >= list->capacity) {
        list->capacity *= 2;
        int64_t* new_data = fern_alloc((size_t)list->capacity * sizeof(int64_t));
        memcpy(new_data, list->data, (size_t)list->length * sizeof(int64_t));
        fern_free(list->data);
        list->data = new_data;
    }
    
    ((int64_t*)list->data)[list->length++] = elem;
}

int64_t fern_list_get(FernList* list, int64_t index) {
    if (!list || index < 0 || index >= list->length) {
        fern_panic("list index out of bounds");
    }
    return ((int64_t*)list->data)[index];
}

/* ========== Result Helper Functions (called from generated code) ========== */

/* These are exported as symbols that QBE-generated code can call */

/* Create Ok result - returns the struct by value (passed as pointer in QBE) */
FernResult fern_result_ok(int64_t value) {
    return fern_ok(value);
}

/* Create Err result */
FernResult fern_result_err(int64_t error) {
    return fern_err(error);
}

/* Check if result is Ok (returns 1 or 0) */
int64_t fern_result_is_ok(FernResult* r) {
    return r->tag == FERN_OK ? 1 : 0;
}

/* Get the value from a result (panics if Err) */
int64_t fern_result_unwrap(FernResult* r) {
    if (r->tag == FERN_ERR) {
        fern_panic("unwrap called on Err");
    }
    return r->value;
}

/* Get the error from a result (panics if Ok) */
int64_t fern_result_unwrap_err(FernResult* r) {
    if (r->tag == FERN_OK) {
        fern_panic("unwrap_err called on Ok");
    }
    return r->value;
}
