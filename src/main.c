/* Fern Compiler - Main Entry Point */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "arena.h"
#include "fern_string.h"
#include "lexer.h"
#include "parser.h"
#include "checker.h"
#include "codegen.h"

/* ========== File Utilities ========== */

/**
 * Read entire file contents into a string.
 * @param arena The arena to allocate from.
 * @param filename The file path to read.
 * @return The file contents, or NULL on error.
 */
static char* read_file(Arena* arena, const char* filename) {
    // FERN_STYLE: allow(assertion-density) file I/O with multiple error paths
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    
    // Allocate buffer
    char* buffer = arena_alloc(arena, (size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    // Read file
    size_t read_size = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    
    if ((long)read_size != size) {
        return NULL;
    }
    
    buffer[size] = '\0';
    return buffer;
}

/**
 * Get the base name of a file path (without extension).
 * @param arena The arena to allocate from.
 * @param filename The file path.
 * @return The base name without extension.
 */
static String* get_basename(Arena* arena, const char* filename) {
    // FERN_STYLE: allow(assertion-density) simple string manipulation
    // Find last slash
    const char* base = filename;
    for (const char* p = filename; *p; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    
    // Find extension
    const char* dot = NULL;
    for (const char* p = base; *p; p++) {
        if (*p == '.') {
            dot = p;
        }
    }
    
    // Create base name
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    return string_new_len(arena, base, len);
}

/* ========== Compilation Pipeline ========== */

/**
 * Print usage information.
 */
static void print_usage(void) {
    // FERN_STYLE: allow(assertion-density) simple print function
    fprintf(stderr, "Fern Compiler v0.0.1\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  fern build <source.fn>   Compile to executable\n");
    fprintf(stderr, "  fern check <source.fn>   Type check only\n");
    fprintf(stderr, "  fern emit <source.fn>    Emit QBE IR to stdout\n");
    fprintf(stderr, "\nBoth .fn and .🌿 file extensions are supported.\n");
}

/**
 * Compile a Fern source file to QBE IR.
 * @param arena The arena for allocations.
 * @param source The source code.
 * @param filename The source filename (for error messages).
 * @return The codegen instance, or NULL on error.
 */
static Codegen* compile_to_qbe(Arena* arena, const char* source, const char* filename) {
    // FERN_STYLE: allow(assertion-density) pipeline orchestration
    
    // Parse
    Parser* parser = parser_new(arena, source);
    StmtVec* stmts = parse_stmts(parser);
    
    if (parser_had_error(parser)) {
        fprintf(stderr, "Parse error in %s\n", filename);
        return NULL;
    }
    
    if (!stmts || stmts->len == 0) {
        fprintf(stderr, "Error: No statements found in %s\n", filename);
        return NULL;
    }
    
    // Type check
    Checker* checker = checker_new(arena);
    bool check_ok = checker_check_stmts(checker, stmts);
    
    if (!check_ok || checker_has_errors(checker)) {
        const char* err = checker_first_error(checker);
        fprintf(stderr, "Type error in %s: %s\n", filename, err ? err : "unknown error");
        return NULL;
    }
    
    // Generate QBE IR
    Codegen* cg = codegen_new(arena);
    codegen_program(cg, stmts);
    
    return cg;
}

/**
 * Run QBE compiler and linker to create executable.
 * @param ssa_file Path to QBE IR file.
 * @param output_file Path for output executable.
 * @return 0 on success, non-zero on error.
 */
static int run_qbe_and_link(const char* ssa_file, const char* output_file) {
    // FERN_STYLE: allow(assertion-density) external process invocation
    char cmd[1024];
    int ret;
    
    // Generate object file with QBE
    char obj_file[256];
    snprintf(obj_file, sizeof(obj_file), "%s.o", output_file);
    
    snprintf(cmd, sizeof(cmd), "qbe -o %s.s %s 2>&1", output_file, ssa_file);
    ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Error: QBE compilation failed (is qbe installed?)\n");
        fprintf(stderr, "  Install QBE: https://c9x.me/compile/\n");
        return 1;
    }
    
    // Assemble
    snprintf(cmd, sizeof(cmd), "cc -c -o %s %s.s 2>&1", obj_file, output_file);
    ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Error: Assembly failed\n");
        return 1;
    }
    
    // Link with runtime (if it exists)
    // For now, just link the object file
    snprintf(cmd, sizeof(cmd), "cc -o %s %s 2>&1", output_file, obj_file);
    ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Error: Linking failed\n");
        return 1;
    }
    
    // Clean up intermediate files
    snprintf(cmd, sizeof(cmd), "%s.s", output_file);
    unlink(cmd);
    unlink(obj_file);
    
    return 0;
}

/* ========== Commands ========== */

/**
 * Build command: compile source to executable.
 * @param arena The arena for allocations.
 * @param filename The source file path.
 * @return Exit code.
 */
static int cmd_build(Arena* arena, const char* filename) {
    // FERN_STYLE: allow(assertion-density) command handler
    
    // Read source file
    char* source = read_file(arena, filename);
    if (!source) {
        fprintf(stderr, "Error: Cannot read file '%s'\n", filename);
        return 1;
    }
    
    printf("Compiling %s...\n", filename);
    
    // Compile to QBE IR
    Codegen* cg = compile_to_qbe(arena, source, filename);
    if (!cg) {
        return 1;
    }
    
    // Write QBE IR to temp file
    String* basename = get_basename(arena, filename);
    char ssa_file[256];
    snprintf(ssa_file, sizeof(ssa_file), "%s.ssa", string_cstr(basename));
    
    if (!codegen_write(cg, ssa_file)) {
        fprintf(stderr, "Error: Cannot write QBE IR to '%s'\n", ssa_file);
        return 1;
    }
    
    // Run QBE and link
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s", string_cstr(basename));
    
    int ret = run_qbe_and_link(ssa_file, output_file);
    
    // Clean up SSA file on success
    if (ret == 0) {
        unlink(ssa_file);
        printf("Created executable: %s\n", output_file);
    }
    
    return ret;
}

/**
 * Check command: type check only (no code generation).
 * @param arena The arena for allocations.
 * @param filename The source file path.
 * @return Exit code.
 */
static int cmd_check(Arena* arena, const char* filename) {
    // FERN_STYLE: allow(assertion-density) command handler
    
    // Read source file
    char* source = read_file(arena, filename);
    if (!source) {
        fprintf(stderr, "Error: Cannot read file '%s'\n", filename);
        return 1;
    }
    
    // Parse
    Parser* parser = parser_new(arena, source);
    StmtVec* stmts = parse_stmts(parser);
    
    if (parser_had_error(parser)) {
        fprintf(stderr, "Parse error in %s\n", filename);
        return 1;
    }
    
    // Type check
    Checker* checker = checker_new(arena);
    bool check_ok = checker_check_stmts(checker, stmts);
    
    if (!check_ok || checker_has_errors(checker)) {
        const char* err = checker_first_error(checker);
        fprintf(stderr, "Type error: %s\n", err ? err : "unknown error");
        return 1;
    }
    
    printf("✓ %s: No type errors\n", filename);
    return 0;
}

/**
 * Emit command: output QBE IR to stdout.
 * @param arena The arena for allocations.
 * @param filename The source file path.
 * @return Exit code.
 */
static int cmd_emit(Arena* arena, const char* filename) {
    // FERN_STYLE: allow(assertion-density) command handler
    
    // Read source file
    char* source = read_file(arena, filename);
    if (!source) {
        fprintf(stderr, "Error: Cannot read file '%s'\n", filename);
        return 1;
    }
    
    // Compile to QBE IR
    Codegen* cg = compile_to_qbe(arena, source, filename);
    if (!cg) {
        return 1;
    }
    
    // Output to stdout
    codegen_emit(cg, stdout);
    return 0;
}

/* ========== Main Entry Point ========== */

/**
 * Main entry point for the Fern compiler.
 * @param argc The argument count.
 * @param argv The argument values.
 * @return Exit code (0 on success, non-zero on error).
 */
int main(int argc, char** argv) {
    // FERN_STYLE: allow(assertion-density) main entry point - handles args and setup
    
    if (argc < 3) {
        print_usage();
        return 1;
    }
    
    const char* command = argv[1];
    const char* filename = argv[2];
    
    // Create arena for compiler session
    Arena* arena = arena_create(4 * 1024 * 1024);  // 4MB
    if (!arena) {
        fprintf(stderr, "Error: Failed to initialize memory\n");
        return 1;
    }
    
    int result;
    
    if (strcmp(command, "build") == 0) {
        result = cmd_build(arena, filename);
    } else if (strcmp(command, "check") == 0) {
        result = cmd_check(arena, filename);
    } else if (strcmp(command, "emit") == 0) {
        result = cmd_emit(arena, filename);
    } else {
        fprintf(stderr, "Unknown command: %s\n\n", command);
        print_usage();
        result = 1;
    }
    
    arena_destroy(arena);
    return result;
}
