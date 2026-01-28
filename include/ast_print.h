/* AST Pretty-Printer
 *
 * Functions for printing AST nodes in a human-readable format.
 * Useful for debugging the parser.
 */

#ifndef FERN_AST_PRINT_H
#define FERN_AST_PRINT_H

#include "ast.h"
#include <stdio.h>

/* Print an expression tree with indentation */
void ast_print_expr(FILE* out, Expr* expr, int indent);

/* Print a statement with indentation */
void ast_print_stmt(FILE* out, Stmt* stmt, int indent);

/* Print a type expression with indentation */
void ast_print_type(FILE* out, TypeExpr* type, int indent);

/* Print a pattern with indentation */
void ast_print_pattern(FILE* out, Pattern* pattern, int indent);

/* Convenience functions that print to stdout */
void ast_dump_expr(Expr* expr);
void ast_dump_stmt(Stmt* stmt);

#endif /* FERN_AST_PRINT_H */
