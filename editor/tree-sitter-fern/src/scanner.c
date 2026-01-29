/**
 * External scanner for Fern Tree-sitter grammar
 * Handles indentation-based syntax (INDENT/DEDENT tokens)
 *
 * This scanner implements Python-style indentation tracking:
 * - INDENT when indentation level increases
 * - DEDENT when indentation level decreases
 * - NEWLINE for significant line breaks
 */

#include <tree_sitter/parser.h>
#include <wctype.h>
#include <string.h>
#include <stdio.h>

// Token types that this scanner can emit
// IMPORTANT: Order must match the externals in grammar.js: [$._newline, $._indent, $._dedent]
enum TokenType {
    NEWLINE,   // 0 -> $._newline
    INDENT,    // 1 -> $._indent
    DEDENT,    // 2 -> $._dedent
};

// Maximum indentation stack depth
#define MAX_INDENT_STACK 256

// Scanner state (persisted across calls)
typedef struct {
    uint16_t indent_stack[MAX_INDENT_STACK];
    uint16_t indent_stack_size;
} Scanner;

// Initialize scanner
void *tree_sitter_fern_external_scanner_create() {
    Scanner *scanner = (Scanner *)calloc(1, sizeof(Scanner));
    scanner->indent_stack[0] = 0;
    scanner->indent_stack_size = 1;
    return scanner;
}

// Destroy scanner
void tree_sitter_fern_external_scanner_destroy(void *payload) {
    free(payload);
}

// Serialize scanner state
unsigned tree_sitter_fern_external_scanner_serialize(
    void *payload,
    char *buffer
) {
    Scanner *scanner = (Scanner *)payload;
    
    if (scanner->indent_stack_size >= MAX_INDENT_STACK) {
        return 0;
    }
    
    memcpy(buffer, scanner->indent_stack, scanner->indent_stack_size * sizeof(uint16_t));
    return scanner->indent_stack_size * sizeof(uint16_t);
}

// Deserialize scanner state
void tree_sitter_fern_external_scanner_deserialize(
    void *payload,
    const char *buffer,
    unsigned length
) {
    Scanner *scanner = (Scanner *)payload;
    
    if (length == 0) {
        scanner->indent_stack[0] = 0;
        scanner->indent_stack_size = 1;
        return;
    }
    
    scanner->indent_stack_size = length / sizeof(uint16_t);
    memcpy(scanner->indent_stack, buffer, length);
}

// Skip whitespace (spaces and tabs)
static void skip_whitespace(TSLexer *lexer) {
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        lexer->advance(lexer, true);
    }
}

// Count indentation (spaces/tabs)
static uint16_t count_indent(TSLexer *lexer) {
    uint16_t indent = 0;
    
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        if (lexer->lookahead == '\t') {
            indent += 4;  // Tabs count as 4 spaces
        } else {
            indent += 1;
        }
        lexer->advance(lexer, true);
    }
    
    return indent;
}

// Main scanning function
bool tree_sitter_fern_external_scanner_scan(
    void *payload,
    TSLexer *lexer,
    const bool *valid_symbols
) {
    Scanner *scanner = (Scanner *)payload;
    
    // Debug: log valid symbols
    #ifdef DEBUG_SCANNER
    fprintf(stderr, "Scanner: valid[NEWLINE]=%d, valid[INDENT]=%d, valid[DEDENT]=%d, lookahead='%c' (%d)\n",
            valid_symbols[NEWLINE], valid_symbols[INDENT], valid_symbols[DEDENT],
            lexer->lookahead, lexer->lookahead);
    #endif
    
    // Handle DEDENT tokens (when returning from deeper indentation)
    if (valid_symbols[DEDENT] && scanner->indent_stack_size > 1) {
        uint16_t current_indent = scanner->indent_stack[scanner->indent_stack_size - 1];
        uint16_t prev_indent = scanner->indent_stack[scanner->indent_stack_size - 2];
        
        // Check indentation at current position
        uint16_t indent = count_indent(lexer);
        
        // Also check for EOF
        if (lexer->eof(lexer)) {
            // Emit DEDENT for remaining indentation levels
            scanner->indent_stack_size--;
            lexer->result_symbol = DEDENT;
            return true;
        }
        
        // Skip blank lines and comments
        if (lexer->lookahead == '\n' || lexer->lookahead == '\r' || 
            lexer->lookahead == '#') {
            return false;
        }
        
        // Emit DEDENT if indentation decreased
        if (indent < current_indent) {
            scanner->indent_stack_size--;
            lexer->result_symbol = DEDENT;
            lexer->mark_end(lexer);
            return true;
        }
    }
    
    // Handle INDENT at start of line (after we've already consumed newline)
    if (valid_symbols[INDENT]) {
        // We're at the beginning of a line, check indentation
        uint16_t indent = count_indent(lexer);
        
        // Skip blank lines
        while (lexer->lookahead == '\n' || lexer->lookahead == '\r') {
            lexer->advance(lexer, true);
            indent = count_indent(lexer);
        }
        
        // Skip comment lines
        if (lexer->lookahead == '#') {
            return false;
        }
        
        uint16_t current_indent = scanner->indent_stack[scanner->indent_stack_size - 1];
        
        // Emit INDENT if indentation increased
        if (indent > current_indent) {
            if (scanner->indent_stack_size >= MAX_INDENT_STACK) {
                return false;
            }
            scanner->indent_stack[scanner->indent_stack_size++] = indent;
            lexer->result_symbol = INDENT;
            lexer->mark_end(lexer);
            return true;
        }
    }
    
    // Handle NEWLINE
    if (valid_symbols[NEWLINE]) {
        // Check for newline
        if (lexer->lookahead == '\n' || lexer->lookahead == '\r') {
            // Consume newline
            lexer->advance(lexer, false);
            if (lexer->lookahead == '\n') {
                lexer->advance(lexer, false);
            }
            
            lexer->mark_end(lexer);
            lexer->result_symbol = NEWLINE;
            return true;
        }
    }
    
    return false;
}
