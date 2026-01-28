/**
 * @file arena.h
 * @brief Arena Allocator - Fast, safe memory management for compiler phases.
 *
 * Arena allocation is the primary memory management strategy in the Fern
 * compiler. All allocations from an arena are freed together when the arena
 * is destroyed, eliminating use-after-free and memory leak bugs.
 *
 * @note This is the only place in the codebase where malloc/free should be
 * used directly. All other code must use arena_alloc().
 *
 * @example
 *     Arena* arena = arena_create(4096);
 *
 *     // Allocate compiler data structures
 *     Expr* expr = arena_alloc(arena, sizeof(Expr));
 *     Token* tokens = arena_alloc(arena, sizeof(Token) * 100);
 *
 *     // ... use the data ...
 *
 *     // Free everything at once - no individual frees needed
 *     arena_destroy(arena);
 */

#ifndef FERN_ARENA_H
#define FERN_ARENA_H

#include <stddef.h>
#include <stdbool.h>

/** @brief Opaque arena allocator handle. */
typedef struct Arena Arena;

/**
 * @brief Create a new arena with the given block size.
 *
 * The block size determines how much memory is allocated at once from
 * the system. Larger blocks mean fewer system allocations but more
 * potential waste. 4096 is a good default.
 *
 * @param block_size Size of each memory block in bytes
 * @return New arena, or NULL on allocation failure
 *
 * @example
 *     Arena* arena = arena_create(4096);
 *     if (!arena) {
 *         // Handle out-of-memory
 *     }
 *
 * @see arena_destroy
 */
Arena* arena_create(size_t block_size);

/**
 * @brief Allocate memory from the arena.
 *
 * Memory is automatically zeroed. The returned pointer is valid until
 * the arena is destroyed or reset.
 *
 * @param arena Arena to allocate from (must not be NULL)
 * @param size Number of bytes to allocate
 * @return Pointer to zeroed memory, or NULL if allocation fails
 *
 * @note Memory is aligned to 16 bytes by default.
 *
 * @see arena_alloc_aligned
 */
void* arena_alloc(Arena* arena, size_t size);

/**
 * @brief Allocate aligned memory from the arena.
 *
 * Use this when you need specific alignment (e.g., for SIMD operations).
 * Memory is automatically zeroed.
 *
 * @param arena Arena to allocate from (must not be NULL)
 * @param size Number of bytes to allocate
 * @param alignment Required alignment (must be power of 2)
 * @return Pointer to zeroed, aligned memory, or NULL if allocation fails
 *
 * @see arena_alloc
 */
void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment);

/**
 * @brief Reset arena to initial state, keeping allocated blocks for reuse.
 *
 * This is much faster than destroy + create when you need to reuse an
 * arena (e.g., for parsing multiple files). All previously returned
 * pointers become invalid.
 *
 * @param arena Arena to reset (must not be NULL)
 *
 * @warning All pointers previously allocated from this arena become invalid.
 */
void arena_reset(Arena* arena);

/**
 * @brief Destroy arena and free all memory.
 *
 * All memory allocated from this arena is freed. All pointers previously
 * returned by arena_alloc() become invalid.
 *
 * @param arena Arena to destroy (may be NULL, which is a no-op)
 *
 * @warning All pointers previously allocated from this arena become invalid.
 *
 * @see arena_create
 */
void arena_destroy(Arena* arena);

/**
 * @brief Get total bytes allocated from this arena.
 *
 * Useful for profiling memory usage of different compiler phases.
 *
 * @param arena Arena to query (must not be NULL)
 * @return Total bytes allocated (not including arena overhead)
 */
size_t arena_total_allocated(Arena* arena);

#endif /* FERN_ARENA_H */
