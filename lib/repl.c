/**
 * @file repl.c
 * @brief Fern REPL implementation.
 */

#include "repl.h"
#include "arena.h"
#include "ast.h"
#include "checker.h"
#include "codegen.h"
#include "errors.h"
#include "fern_string.h"
#include "lexer.h"
#include "parser.h"
#include "type.h"
#include "type_env.h"
#include "version.h"
#include "linenoise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* ========== Constants ========== */

/** Maximum history entries to keep. */
#define REPL_HISTORY_MAX 1000

/** History file path (relative to home directory). */
#define REPL_HISTORY_FILE ".fern_history"

/** REPL prompt string. */
#define REPL_PROMPT "fern> "

/* ========== REPL State ========== */

/**
 * REPL state structure.
 */
struct Repl {
    Arena* arena;           /**< Memory arena for allocations */
    Checker* checker;       /**< Persistent type checker */
    bool should_exit;       /**< Exit flag */
    int expr_count;         /**< Counter for expression naming */
    char history_path[512]; /**< Full path to history file */
};

/* ========== Forward Declarations ========== */

static void repl_print_welcome(void);
static void repl_print_help(void);
static bool repl_handle_command(Repl* repl, const char* line);
static bool repl_eval_expression(Repl* repl, const char* line);
static bool repl_eval_statement(Repl* repl, const char* line);
static void repl_completion(const char* buf, linenoiseCompletions* lc);
static char* repl_hints(const char* buf, int* color, int* bold);
static void repl_init_history(Repl* repl);
static void repl_save_history(Repl* repl);

/* ========== Constant Expression Evaluation ========== */

typedef enum {
    REPL_VAL_INT,
    REPL_VAL_FLOAT,
    REPL_VAL_STRING,
    REPL_VAL_BOOL,
} ReplValueKind;

typedef struct {
    ReplValueKind kind;
    union {
        int64_t i64;
        double f64;
        String* str;
        bool b;
    } data;
} ExprValue;

static bool repl_eval_const_expr(Expr* expr, ExprValue* out);
static void repl_print_expr_value(ExprValue* value, String* type_str);

/* ========== Keywords for Completion ========== */

// FERN_STYLE: allow(assertion-density) static data array
static const char* KEYWORDS[] = {
    "fn", "let", "if", "else", "match", "for", "while", "loop",
    "return", "break", "continue", "true", "false", "and", "or", "not",
    "type", "trait", "impl", "pub", "import", "module", "defer", "with",
    "do", "in", "as", "Ok", "Err", "Some", "None",
    NULL
};

// FERN_STYLE: allow(assertion-density) static data array
static const char* BUILTINS[] = {
    "print", "println",
    "str_len", "str_concat", "str_eq", "str_starts_with", "str_ends_with",
    "str_contains", "str_slice", "str_trim", "str_to_upper", "str_to_lower",
    "str_replace", "str_repeat", "str_is_empty",
    "list_len", "list_get", "list_push", "list_reverse", "list_concat",
    "list_head", "list_tail", "list_is_empty",
    "read_file", "write_file", "append_file", "file_exists", "delete_file",
    "file_size",
    NULL
};

/* ========== REPL Creation ========== */

/**
 * Create a new REPL instance.
 *
 * @param arena Memory arena for allocations.
 * @return New Repl instance, or NULL on failure.
 */
Repl* repl_new(Arena* arena) {
    assert(arena != NULL);
    
    Repl* repl = arena_alloc(arena, sizeof(Repl));
    assert(repl != NULL);
    
    repl->arena = arena;
    repl->checker = checker_new(arena);
    repl->should_exit = false;
    repl->expr_count = 0;
    repl->history_path[0] = '\0';
    
    // Set up completion and hints
    linenoiseSetCompletionCallback(repl_completion);
    linenoiseSetHintsCallback(repl_hints);
    linenoiseSetMultiLine(1);
    linenoiseHistorySetMaxLen(REPL_HISTORY_MAX);
    
    // Initialize history
    repl_init_history(repl);
    
    return repl;
}

/* ========== REPL Execution ========== */

/**
 * Run the REPL main loop.
 *
 * Starts the interactive loop, reading lines until exit.
 *
 * @param repl REPL instance.
 * @return Exit code.
 */
int repl_run(Repl* repl) {
    // FERN_STYLE: allow(assertion-density) main loop with linenoise
    assert(repl != NULL);
    
    repl_print_welcome();
    
    char* line;
    while (!repl->should_exit && (line = linenoise(REPL_PROMPT)) != NULL) {
        // Skip empty lines
        if (line[0] == '\0') {
            linenoiseFree(line);
            continue;
        }
        
        // Add to history
        linenoiseHistoryAdd(line);
        
        // Evaluate the line
        repl_eval_line(repl, line);
        
        linenoiseFree(line);
    }
    
    // Save history on exit
    repl_save_history(repl);
    
    // Print newline after Ctrl+D
    if (!repl->should_exit) {
        printf("\n");
    }
    
    return 0;
}

/**
 * Evaluate a single line of input.
 *
 * Parses, type checks, and evaluates the input line.
 *
 * @param repl REPL instance.
 * @param line Input line to evaluate.
 * @return true if evaluation succeeded.
 */
bool repl_eval_line(Repl* repl, const char* line) {
    assert(repl != NULL);
    assert(line != NULL);
    
    // Clear any previous errors
    checker_clear_errors(repl->checker);
    
    // Skip whitespace
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    
    // Skip empty lines
    if (*line == '\0') {
        return true;
    }
    
    // Handle special commands (starting with :)
    if (line[0] == ':') {
        return repl_handle_command(repl, line);
    }
    
    // Try to parse as statement first (let, fn, etc.)
    if (strncmp(line, "let ", 4) == 0 ||
        strncmp(line, "fn ", 3) == 0 ||
        strncmp(line, "pub ", 4) == 0 ||
        strncmp(line, "type ", 5) == 0 ||
        strncmp(line, "import ", 7) == 0) {
        return repl_eval_statement(repl, line);
    }
    
    // Otherwise, evaluate as expression
    return repl_eval_expression(repl, line);
}

/* ========== State Access ========== */

/**
 * Get the REPL's type environment.
 *
 * Returns the type environment for variable lookup.
 *
 * @param repl REPL instance.
 * @return Type environment.
 */
TypeEnv* repl_type_env(Repl* repl) {
    // FERN_STYLE: allow(assertion-density) simple accessor
    assert(repl != NULL);
    return checker_env(repl->checker);
}

/**
 * Check if the REPL should exit.
 *
 * Returns true after :quit command or EOF.
 *
 * @param repl REPL instance.
 * @return true if REPL should exit.
 */
bool repl_should_exit(Repl* repl) {
    // FERN_STYLE: allow(assertion-density) simple accessor
    assert(repl != NULL);
    return repl->should_exit;
}

/* ========== Internal Functions ========== */

/**
 * Print welcome message.
 *
 * Shows version and help hint.
 */
static void repl_print_welcome(void) {
    // FERN_STYLE: allow(assertion-density) simple print function
    printf("%s\n", FERN_VERSION);
    printf("Type :help for help, :quit to exit\n\n");
}

/**
 * Print help message.
 *
 * Shows available commands and examples.
 */
static void repl_print_help(void) {
    // FERN_STYLE: allow(assertion-density) simple print function
    printf("Fern REPL Commands:\n");
    printf("  :help, :h     Show this help message\n");
    printf("  :quit, :q     Exit the REPL\n");
    printf("  :type <expr>  Show the type of an expression\n");
    printf("  :clear        Clear the screen\n");
    printf("\n");
    printf("Examples:\n");
    printf("  1 + 2              Evaluate expression\n");
    printf("  let x = 42         Define a variable\n");
    printf("  fn add(a, b): a+b  Define a function\n");
    printf("  :type x + 1        Show type without evaluating\n");
    printf("\n");
}

/**
 * Handle special commands (starting with :).
 *
 * Dispatches to the appropriate command handler.
 *
 * @param repl REPL instance.
 * @param line Command line.
 * @return true if command was handled.
 */
static bool repl_handle_command(Repl* repl, const char* line) {
    assert(repl != NULL);
    assert(line != NULL);
    assert(line[0] == ':');
    
    const char* cmd = line + 1;
    
    // :quit, :q
    if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
        repl->should_exit = true;
        printf("Goodbye!\n");
        return true;
    }
    
    // :help, :h
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
        repl_print_help();
        return true;
    }
    
    // :clear
    if (strcmp(cmd, "clear") == 0) {
        linenoiseClearScreen();
        return true;
    }
    
    // :type <expr> or :t <expr>
    const char* expr_str = NULL;
    if (strncmp(cmd, "type ", 5) == 0) {
        expr_str = cmd + 5;
    } else if (strncmp(cmd, "t ", 2) == 0) {
        expr_str = cmd + 2;
    }
    
    if (expr_str != NULL) {
        // Skip whitespace
        while (*expr_str == ' ') expr_str++;
        
        if (*expr_str == '\0') {
            error_print("usage: :type <expression>");
            return false;
        }
        
        // Parse and type check without evaluating
        Parser* parser = parser_new(repl->arena, expr_str);
        Expr* expr = parse_expr(parser);
        
        if (parser_had_error(parser) || expr == NULL) {
            error_print("parse error");
            return false;
        }
        
        Type* type = checker_infer_expr(repl->checker, expr);
        if (checker_has_errors(repl->checker) || type == NULL) {
            const char* err = checker_first_error(repl->checker);
            error_print("%s", err ? err : "type error");
            return false;
        }
        
        String* type_str = type_to_string(repl->arena, type);
        printf("%s\n", string_cstr(type_str));
        return true;
    }
    
    // Unknown command
    error_print("unknown command: %s", line);
    printf("Type :help for available commands\n");
    return false;
}

/**
 * Evaluate an expression and print the result with its type.
 *
 * For literals, shows the value. For complex expressions, shows inferred type.
 *
 * @param repl REPL instance.
 * @param line Expression string.
 * @return true if evaluation succeeded.
 */
static bool repl_eval_expression(Repl* repl, const char* line) {
    // FERN_STYLE: allow(function-length) expression evaluation is cohesive
    assert(repl != NULL);
    assert(line != NULL);
    
    // Parse as expression
    Parser* parser = parser_new(repl->arena, line);
    Expr* expr = parse_expr(parser);
    
    if (parser_had_error(parser) || expr == NULL) {
        error_print("parse error");
        return false;
    }
    
    // Type check
    Type* type = checker_infer_expr(repl->checker, expr);
    if (checker_has_errors(repl->checker) || type == NULL) {
        const char* err = checker_first_error(repl->checker);
        error_print("%s", err ? err : "type error");
        return false;
    }
    
    String* type_str = type_to_string(repl->arena, type);

    ExprValue value;
    if (repl_eval_const_expr(expr, &value)) {
        repl_print_expr_value(&value, type_str);
    } else {
        /* Fall back when expression depends on runtime state or effects. */
        printf("<expr> : %s\n", string_cstr(type_str));
    }
    
    return true;
}

/**
 * Print a computed expression value with its inferred type.
 *
 * @param value Evaluated expression value.
 * @param type_str Inferred expression type string.
 */
static void repl_print_expr_value(ExprValue* value, String* type_str) {
    assert(value != NULL);
    assert(type_str != NULL);

    switch (value->kind) {
        case REPL_VAL_INT:
            printf("%lld : %s\n", (long long)value->data.i64, string_cstr(type_str));
            return;
        case REPL_VAL_FLOAT:
            printf("%g : %s\n", value->data.f64, string_cstr(type_str));
            return;
        case REPL_VAL_STRING:
            printf("\"%s\" : %s\n", string_cstr(value->data.str), string_cstr(type_str));
            return;
        case REPL_VAL_BOOL:
            printf("%s : %s\n", value->data.b ? "true" : "false", string_cstr(type_str));
            return;
    }
}

/**
 * Evaluate a unary expression in constant-evaluation mode.
 *
 * Supports numeric negation and boolean negation.
 *
 * @param unary Unary expression AST node.
 * @param out Output evaluated value.
 * @return true if expression could be evaluated as a constant.
 */
static bool repl_eval_const_unary(const UnaryExpr* unary, ExprValue* out) {
    assert(unary != NULL);
    assert(out != NULL);

    ExprValue operand;
    if (!repl_eval_const_expr(unary->operand, &operand)) {
        return false;
    }

    if (unary->op == UNOP_NEG) {
        if (operand.kind == REPL_VAL_INT) {
            out->kind = REPL_VAL_INT;
            out->data.i64 = -operand.data.i64;
            return true;
        }
        if (operand.kind == REPL_VAL_FLOAT) {
            out->kind = REPL_VAL_FLOAT;
            out->data.f64 = -operand.data.f64;
            return true;
        }
        return false;
    }

    if (unary->op == UNOP_NOT && operand.kind == REPL_VAL_BOOL) {
        out->kind = REPL_VAL_BOOL;
        out->data.b = !operand.data.b;
        return true;
    }

    return false;
}

/**
 * Evaluate a numeric binary expression in constant-evaluation mode.
 *
 * Supports Int and Float arithmetic/comparison operators.
 *
 * @param op Binary operator.
 * @param left Left operand value.
 * @param right Right operand value.
 * @param out Output evaluated value.
 * @return true if expression could be evaluated as a constant.
 */
static bool repl_eval_const_binary_numeric(
    BinaryOp op,
    ExprValue* left,
    ExprValue* right,
    ExprValue* out
) {
    assert(left != NULL);
    assert(right != NULL);
    assert(out != NULL);

    bool left_num = left->kind == REPL_VAL_INT || left->kind == REPL_VAL_FLOAT;
    bool right_num = right->kind == REPL_VAL_INT || right->kind == REPL_VAL_FLOAT;
    if (!left_num || !right_num) {
        return false;
    }

    if (left->kind == REPL_VAL_INT && right->kind == REPL_VAL_INT) {
        int64_t l = left->data.i64;
        int64_t r = right->data.i64;
        switch (op) {
            case BINOP_ADD: out->kind = REPL_VAL_INT; out->data.i64 = l + r; return true;
            case BINOP_SUB: out->kind = REPL_VAL_INT; out->data.i64 = l - r; return true;
            case BINOP_MUL: out->kind = REPL_VAL_INT; out->data.i64 = l * r; return true;
            case BINOP_DIV:
                if (r == 0) return false;
                out->kind = REPL_VAL_INT;
                out->data.i64 = l / r;
                return true;
            case BINOP_MOD:
                if (r == 0) return false;
                out->kind = REPL_VAL_INT;
                out->data.i64 = l % r;
                return true;
            case BINOP_POW:
                out->kind = REPL_VAL_FLOAT;
                out->data.f64 = pow((double)l, (double)r);
                return true;
            case BINOP_LT: out->kind = REPL_VAL_BOOL; out->data.b = l < r; return true;
            case BINOP_LE: out->kind = REPL_VAL_BOOL; out->data.b = l <= r; return true;
            case BINOP_GT: out->kind = REPL_VAL_BOOL; out->data.b = l > r; return true;
            case BINOP_GE: out->kind = REPL_VAL_BOOL; out->data.b = l >= r; return true;
            default: break;
        }
    }

    double l = (left->kind == REPL_VAL_FLOAT) ? left->data.f64 : (double)left->data.i64;
    double r = (right->kind == REPL_VAL_FLOAT) ? right->data.f64 : (double)right->data.i64;
    switch (op) {
        case BINOP_ADD: out->kind = REPL_VAL_FLOAT; out->data.f64 = l + r; return true;
        case BINOP_SUB: out->kind = REPL_VAL_FLOAT; out->data.f64 = l - r; return true;
        case BINOP_MUL: out->kind = REPL_VAL_FLOAT; out->data.f64 = l * r; return true;
        case BINOP_DIV:
            if (r == 0.0) return false;
            out->kind = REPL_VAL_FLOAT;
            out->data.f64 = l / r;
            return true;
        case BINOP_POW: out->kind = REPL_VAL_FLOAT; out->data.f64 = pow(l, r); return true;
        case BINOP_LT: out->kind = REPL_VAL_BOOL; out->data.b = l < r; return true;
        case BINOP_LE: out->kind = REPL_VAL_BOOL; out->data.b = l <= r; return true;
        case BINOP_GT: out->kind = REPL_VAL_BOOL; out->data.b = l > r; return true;
        case BINOP_GE: out->kind = REPL_VAL_BOOL; out->data.b = l >= r; return true;
        default: return false;
    }
}

/**
 * Evaluate a binary expression in constant-evaluation mode.
 *
 * Supports equality/inequality, boolean logic, and numeric operations.
 *
 * @param binary Binary expression AST node.
 * @param out Output evaluated value.
 * @return true if expression could be evaluated as a constant.
 */
static bool repl_eval_const_binary(const BinaryExpr* binary, ExprValue* out) {
    assert(binary != NULL);
    assert(out != NULL);

    ExprValue left;
    ExprValue right;
    if (!repl_eval_const_expr(binary->left, &left) ||
        !repl_eval_const_expr(binary->right, &right)) {
        return false;
    }

    switch (binary->op) {
        case BINOP_EQ:
            out->kind = REPL_VAL_BOOL;
            if (left.kind == REPL_VAL_INT && right.kind == REPL_VAL_INT) {
                out->data.b = left.data.i64 == right.data.i64;
                return true;
            }
            if ((left.kind == REPL_VAL_INT || left.kind == REPL_VAL_FLOAT) &&
                (right.kind == REPL_VAL_INT || right.kind == REPL_VAL_FLOAT)) {
                double l = (left.kind == REPL_VAL_FLOAT) ? left.data.f64 : (double)left.data.i64;
                double r = (right.kind == REPL_VAL_FLOAT) ? right.data.f64 : (double)right.data.i64;
                out->data.b = l == r;
                return true;
            }
            if (left.kind == REPL_VAL_BOOL && right.kind == REPL_VAL_BOOL) {
                out->data.b = left.data.b == right.data.b;
                return true;
            }
            if (left.kind == REPL_VAL_STRING && right.kind == REPL_VAL_STRING) {
                out->data.b = strcmp(string_cstr(left.data.str), string_cstr(right.data.str)) == 0;
                return true;
            }
            return false;
        case BINOP_NE:
            if (!repl_eval_const_binary(
                    &(BinaryExpr){ .op = BINOP_EQ, .left = binary->left, .right = binary->right },
                    out)) {
                return false;
            }
            out->data.b = !out->data.b;
            return true;
        case BINOP_AND:
            if (left.kind != REPL_VAL_BOOL || right.kind != REPL_VAL_BOOL) return false;
            out->kind = REPL_VAL_BOOL;
            out->data.b = left.data.b && right.data.b;
            return true;
        case BINOP_OR:
            if (left.kind != REPL_VAL_BOOL || right.kind != REPL_VAL_BOOL) return false;
            out->kind = REPL_VAL_BOOL;
            out->data.b = left.data.b || right.data.b;
            return true;
        default:
            return repl_eval_const_binary_numeric(binary->op, &left, &right, out);
    }
}

/**
 * Evaluate an expression in constant-evaluation mode.
 *
 * Handles literals, unary/binary operations, and if-expressions.
 *
 * @param expr Expression AST node.
 * @param out Output evaluated value.
 * @return true if expression could be evaluated as a constant.
 */
static bool repl_eval_const_expr(Expr* expr, ExprValue* out) {
    assert(expr != NULL);
    assert(out != NULL);

    switch (expr->type) {
        case EXPR_INT_LIT:
            out->kind = REPL_VAL_INT;
            out->data.i64 = expr->data.int_lit.value;
            return true;
        case EXPR_FLOAT_LIT:
            out->kind = REPL_VAL_FLOAT;
            out->data.f64 = expr->data.float_lit.value;
            return true;
        case EXPR_STRING_LIT:
            out->kind = REPL_VAL_STRING;
            out->data.str = expr->data.string_lit.value;
            return true;
        case EXPR_BOOL_LIT:
            out->kind = REPL_VAL_BOOL;
            out->data.b = expr->data.bool_lit.value;
            return true;
        case EXPR_UNARY:
            return repl_eval_const_unary(&expr->data.unary, out);
        case EXPR_BINARY:
            return repl_eval_const_binary(&expr->data.binary, out);
        case EXPR_IF: {
            ExprValue cond;
            if (!repl_eval_const_expr(expr->data.if_expr.condition, &cond)) return false;
            if (cond.kind != REPL_VAL_BOOL) return false;
            if (cond.data.b) return repl_eval_const_expr(expr->data.if_expr.then_branch, out);
            if (expr->data.if_expr.else_branch == NULL) return false;
            return repl_eval_const_expr(expr->data.if_expr.else_branch, out);
        }
        default:
            return false;
    }
}

/**
 * Evaluate a statement (let, fn, etc.).
 *
 * Type checks the statement and adds bindings to the environment.
 *
 * @param repl REPL instance.
 * @param line Statement string.
 * @return true if evaluation succeeded.
 */
static bool repl_eval_statement(Repl* repl, const char* line) {
    assert(repl != NULL);
    assert(line != NULL);
    
    // Parse as statement
    Parser* parser = parser_new(repl->arena, line);
    StmtVec* stmts = parse_stmts(parser);
    
    if (parser_had_error(parser) || stmts == NULL || stmts->len == 0) {
        error_print("parse error");
        return false;
    }
    
    // Type check
    bool ok = checker_check_stmts(repl->checker, stmts);
    if (!ok || checker_has_errors(repl->checker)) {
        const char* err = checker_first_error(repl->checker);
        error_print("%s", err ? err : "type error");
        return false;
    }
    
    // Report what was defined
    Stmt* stmt = stmts->data[0];
    if (stmt->type == STMT_LET) {
        Pattern* pattern = stmt->data.let.pattern;
        if (pattern && pattern->type == PATTERN_IDENT) {
            String* name = pattern->data.ident;
            Type* type = type_env_lookup(checker_env(repl->checker), name);
            if (type) {
                String* type_str = type_to_string(repl->arena, type);
                printf("%s : %s\n", string_cstr(name), string_cstr(type_str));
            }
        }
    } else if (stmt->type == STMT_FN) {
        String* name = stmt->data.fn.name;
        printf("fn %s defined\n", string_cstr(name));
    } else if (stmt->type == STMT_TYPE_DEF) {
        String* name = stmt->data.type_def.name;
        printf("type %s defined\n", string_cstr(name));
    }
    
    return true;
}

/**
 * Tab completion callback for linenoise.
 *
 * Completes keywords and built-in function names.
 *
 * @param buf Current input buffer.
 * @param lc Completions structure to fill.
 */
static void repl_completion(const char* buf, linenoiseCompletions* lc) {
    // FERN_STYLE: allow(assertion-density) completion callback uses early returns
    if (buf == NULL || lc == NULL) return;
    
    size_t len = strlen(buf);
    if (len == 0) return;
    
    // Find the start of the current word
    const char* word_start = buf + len;
    while (word_start > buf && *(word_start - 1) != ' ' && *(word_start - 1) != '(') {
        word_start--;
    }
    
    size_t word_len = (size_t)(buf + len - word_start);
    if (word_len == 0) return;
    
    // Check keywords
    for (const char** kw = KEYWORDS; *kw != NULL; kw++) {
        if (strncmp(*kw, word_start, word_len) == 0) {
            // Build full completion
            char completion[256];
            size_t prefix_len = (size_t)(word_start - buf);
            if (prefix_len >= sizeof(completion)) continue;
            
            memcpy(completion, buf, prefix_len);
            size_t kw_len = strlen(*kw);
            if (prefix_len + kw_len >= sizeof(completion)) continue;
            
            strcpy(completion + prefix_len, *kw);
            linenoiseAddCompletion(lc, completion);
        }
    }
    
    // Check built-in functions
    for (const char** fn = BUILTINS; *fn != NULL; fn++) {
        if (strncmp(*fn, word_start, word_len) == 0) {
            char completion[256];
            size_t prefix_len = (size_t)(word_start - buf);
            if (prefix_len >= sizeof(completion)) continue;
            
            memcpy(completion, buf, prefix_len);
            size_t fn_len = strlen(*fn);
            if (prefix_len + fn_len >= sizeof(completion)) continue;
            
            strcpy(completion + prefix_len, *fn);
            linenoiseAddCompletion(lc, completion);
        }
    }
}

/**
 * Hints callback for linenoise.
 *
 * Currently returns NULL (no hints).
 *
 * @param buf Current input buffer.
 * @param color Output: hint color.
 * @param bold Output: hint bold flag.
 * @return Hint string, or NULL for no hint.
 */
static char* repl_hints(const char* buf, int* color, int* bold) {
    // FERN_STYLE: allow(assertion-density) hints callback is a no-op for now
    (void)buf;
    (void)color;
    (void)bold;
    
    // No hints for now - could add type hints in the future
    return NULL;
}

/**
 * Initialize history file path and load history.
 *
 * Sets up the history file in the user's home directory.
 *
 * @param repl REPL instance.
 */
static void repl_init_history(Repl* repl) {
    // FERN_STYLE: allow(assertion-density) I/O setup with early returns
    assert(repl != NULL);
    
    // Get home directory
    const char* home = getenv("HOME");
    if (home == NULL) {
        return;
    }
    
    // Build history file path
    snprintf(repl->history_path, sizeof(repl->history_path),
             "%s/%s", home, REPL_HISTORY_FILE);
    
    // Load history if file exists
    struct stat st;
    if (stat(repl->history_path, &st) == 0) {
        linenoiseHistoryLoad(repl->history_path);
    }
}

/**
 * Save history to file.
 *
 * Persists command history for future sessions.
 *
 * @param repl REPL instance.
 */
static void repl_save_history(Repl* repl) {
    // FERN_STYLE: allow(assertion-density) simple history save
    assert(repl != NULL);
    
    if (repl->history_path[0] != '\0') {
        linenoiseHistorySave(repl->history_path);
    }
}
