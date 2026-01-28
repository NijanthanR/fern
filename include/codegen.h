/**
 * @file codegen.h
 * @brief Fern Code Generator - Produces QBE IR from the AST.
 *
 * The code generator is the final stage of the compiler pipeline. It takes
 * a type-checked AST and produces QBE Intermediate Representation, which
 * can then be compiled to native code by the QBE compiler.
 *
 * QBE (https://c9x.me/compile/) is a simple, lightweight compiler backend
 * that handles register allocation and instruction selection.
 *
 * @example
 *     Arena* arena = arena_create(4096);
 *     Parser* parser = parser_new(arena, "fn main(): 42");
 *     StmtVec* stmts = parse_stmts(parser);
 *
 *     Checker* checker = checker_new(arena);
 *     checker_check_stmts(checker, stmts);
 *
 *     Codegen* cg = codegen_new(arena);
 *     codegen_program(cg, stmts);
 *
 *     // Write QBE IR to file
 *     codegen_write(cg, "output.ssa");
 *
 *     arena_destroy(arena);
 *
 * @see checker.h, ast.h
 */

#ifndef FERN_CODEGEN_H
#define FERN_CODEGEN_H

#include "arena.h"
#include "ast.h"
#include "fern_string.h"
#include <stdbool.h>
#include <stdio.h>

/** @brief Opaque code generator state. */
typedef struct Codegen Codegen;

/* ========== Codegen Creation ========== */

/**
 * @brief Create a new code generator.
 *
 * Initializes a code generator with empty output buffers.
 *
 * @param arena Memory arena for allocations (must not be NULL)
 * @return New Codegen instance
 */
Codegen* codegen_new(Arena* arena);

/* ========== Code Generation ========== */

/**
 * @brief Generate QBE IR for an expression.
 *
 * Emits QBE instructions for the expression and returns the name
 * of the temporary variable holding the result.
 *
 * @param cg Code generator instance (must not be NULL)
 * @param expr Expression to compile (must not be NULL)
 * @return Name of temporary holding the result (e.g., "%t0")
 */
String* codegen_expr(Codegen* cg, Expr* expr);

/**
 * @brief Generate QBE IR for a statement.
 *
 * Emits QBE instructions for the statement. For function definitions,
 * emits a complete QBE function.
 *
 * @param cg Code generator instance (must not be NULL)
 * @param stmt Statement to compile (must not be NULL)
 */
void codegen_stmt(Codegen* cg, Stmt* stmt);

/**
 * @brief Generate QBE IR for a complete program.
 *
 * Emits QBE IR for all statements, including necessary preamble
 * and runtime support.
 *
 * @param cg Code generator instance (must not be NULL)
 * @param stmts Program statements (must not be NULL)
 */
void codegen_program(Codegen* cg, StmtVec* stmts);

/* ========== Output ========== */

/**
 * @brief Get the generated QBE IR as a string.
 *
 * Returns the complete QBE IR output accumulated so far.
 *
 * @param cg Code generator instance (must not be NULL)
 * @return Generated QBE IR string
 */
String* codegen_output(Codegen* cg);

/**
 * @brief Write generated QBE IR to a file.
 *
 * @param cg Code generator instance (must not be NULL)
 * @param filename Output file path
 * @return true on success, false on I/O error
 */
bool codegen_write(Codegen* cg, const char* filename);

/**
 * @brief Write generated QBE IR to a FILE stream.
 *
 * @param cg Code generator instance (must not be NULL)
 * @param out Output stream (must not be NULL)
 */
void codegen_emit(Codegen* cg, FILE* out);

#endif /* FERN_CODEGEN_H */
