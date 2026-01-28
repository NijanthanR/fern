/* Fern Code Generator
 *
 * Generates QBE IR from type-checked Fern AST.
 * QBE is a simple compiler backend that produces native code.
 */

#ifndef FERN_CODEGEN_H
#define FERN_CODEGEN_H

#include "arena.h"
#include "ast.h"
#include "fern_string.h"
#include <stdbool.h>
#include <stdio.h>

/* Forward declaration */
typedef struct Codegen Codegen;

/* ========== Codegen Creation ========== */

/* Create a new code generator */
Codegen* codegen_new(Arena* arena);

/* ========== Code Generation ========== */

/* Generate QBE IR for an expression, returns the temporary holding the result */
String* codegen_expr(Codegen* cg, Expr* expr);

/* Generate QBE IR for a statement */
void codegen_stmt(Codegen* cg, Stmt* stmt);

/* Generate QBE IR for a list of statements (a program) */
void codegen_program(Codegen* cg, StmtVec* stmts);

/* ========== Output ========== */

/* Get the generated QBE IR as a string */
String* codegen_output(Codegen* cg);

/* Write generated QBE IR to a file */
bool codegen_write(Codegen* cg, const char* filename);

/* Write generated QBE IR to a FILE stream */
void codegen_emit(Codegen* cg, FILE* out);

#endif /* FERN_CODEGEN_H */
