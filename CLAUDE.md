# Fern Compiler - AI Development Guide

This guide contains critical information for AI-assisted development of the Fern compiler.

## Language: C with Safety Libraries

The Fern compiler is written in **C11** with modern safety libraries. This provides:
- Excellent AI code generation (C is well-represented in training data)
- Fast compilation and iteration
- Direct control over memory and codegen
- Native performance

## Critical Safety Rules

### Memory Management: Arena Allocation Only

**ALWAYS use arena allocation. NEVER call free() on individual allocations.**

```c
#include "arena.h"

// ✅ CORRECT: Arena allocation
typedef struct {
    Arena ast_arena;
    Arena string_arena;
    Arena type_arena;
} CompilerContext;

Expr* new_expr(CompilerContext *ctx) {
    // Allocate from arena - no individual free needed
    Expr *e = arena_alloc(&ctx->ast_arena, sizeof(Expr));
    return e;
}

void compile(const char *filename) {
    CompilerContext ctx = {0};
    
    // ... use ctx ...
    
    // Free all at once when done
    arena_free(&ctx.ast_arena);
    arena_free(&ctx.string_arena);
    arena_free(&ctx.type_arena);
}

// ❌ WRONG: Individual malloc/free
Expr* bad_example() {
    Expr *e = malloc(sizeof(Expr));  // DON'T DO THIS
    // ...
    free(e);  // DANGEROUS - easy to forget or double-free
    return e;
}
```

**Why:** Arena allocation eliminates entire classes of bugs:
- No use-after-free
- No double-free
- No memory leaks
- Faster allocation (bump pointer)

### Tagged Unions: Use Datatype99

**ALWAYS use Datatype99 for variant types. NEVER manual tagged unions.**

```c
#include <datatype99.h>

// ✅ CORRECT: Datatype99 tagged unions
datatype(
    Expr,
    (IntLit, int64_t),
    (StringLit, sds),
    (BinOp, struct Expr*, TokenType, struct Expr*),
    (Ident, sds)
);

// Pattern matching with exhaustive checking
int64_t eval(Expr *expr) {
    match(*expr) {
        of(IntLit, n) return *n;
        of(BinOp, lhs, op, rhs) {
            // Compiler checks this is exhaustive
            return eval_binop(*lhs, *op, *rhs);
        }
        of(Ident, name) return lookup(*name);
        of(StringLit, s) return 0;  // Must handle ALL cases
    }
    // Compiler warns if we miss a case!
}

// ❌ WRONG: Manual tagged unions
typedef enum { EXPR_INT, EXPR_BINOP } ExprKind;
typedef struct {
    ExprKind kind;  // DON'T DO THIS
    union {
        int64_t int_val;
        struct { void *lhs, *rhs; } binop;
    } data;
} BadExpr;
```

**Why:** Datatype99 provides:
- Exhaustive matching (compiler warns on missing cases)
- Type safety (can't access wrong variant)
- Clearer code (AI generates it correctly)

### Strings: Use SDS (Simple Dynamic Strings)

**ALWAYS use SDS for strings. NEVER use char* with manual length tracking.**

```c
#include "sds.h"

// ✅ CORRECT: SDS strings
sds name = sdsnew("hello");
name = sdscat(name, " world");
printf("Length: %zu\n", sdslen(name));  // O(1)
printf("String: %s\n", name);
sdsfree(name);

// ✅ CORRECT: SDS in structs
typedef struct {
    sds filename;
    sds source;
    int line;
} SourceLoc;

SourceLoc make_loc(const char *file, const char *src) {
    return (SourceLoc){
        .filename = sdsnew(file),
        .source = sdsnew(src),
        .line = 1
    };
}

// ❌ WRONG: Manual char* management
char *bad = malloc(100);  // DON'T DO THIS
strcpy(bad, "hello");     // Unsafe
strcat(bad, " world");    // Can overflow
```

**Why:** SDS provides:
- Binary safety (handles null bytes)
- O(1) length (stored in header)
- Automatic resizing
- Still compatible with C string APIs

### Data Structures: Use stb_ds.h

**ALWAYS use stb_ds for dynamic arrays and hash maps.**

```c
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

// ✅ CORRECT: Dynamic arrays
Expr **exprs = NULL;  // Always initialize to NULL
arrput(exprs, expr1);
arrput(exprs, expr2);
printf("Length: %d\n", arrlen(exprs));

for (int i = 0; i < arrlen(exprs); i++) {
    process(exprs[i]);
}

arrfree(exprs);

// ✅ CORRECT: Hash maps (string -> value)
typedef struct {
    sds key;
    Type *value;
} SymbolEntry;

SymbolEntry *symbols = NULL;  // Initialize to NULL
shput(symbols, "x", type_int);
shput(symbols, "y", type_string);

Type *t = shget(symbols, "x");
if (t) {
    // Found
}

shfree(symbols);

// ❌ WRONG: Manual array management
Expr **bad = malloc(10 * sizeof(Expr*));  // DON'T
// ... resize manually, error-prone
```

**Why:** stb_ds provides:
- Type-safe macros
- Automatic resizing
- Hash maps with string keys (perfect for symbol tables)
- Simple API (AI generates correctly)

### Error Handling: Use Result Types

**ALWAYS return Result types for operations that can fail.**

```c
// ✅ CORRECT: Result types with Datatype99
datatype(
    ParseResult,
    (OkExpr, Expr*),
    (ParseErr, sds)
);

ParseResult parse_expr(Parser *p) {
    if (peek(p).type != TOK_INT) {
        sds err = sdscatprintf(sdsempty(),
            "%s:%d: Expected integer",
            p->filename, p->line);
        return ParseErr(err);
    }
    
    Expr *e = /* ... */;
    return OkExpr(e);
}

// Usage - MUST handle both cases
ParseResult result = parse_expr(parser);
match(result) {
    of(OkExpr, expr) {
        codegen(*expr);
    }
    of(ParseErr, msg) {
        fprintf(stderr, "%s\n", *msg);
        exit(1);
    }
}

// ❌ WRONG: Return NULL on error
Expr* bad_parse(Parser *p) {
    if (error) return NULL;  // Lost error information!
    return expr;
}
```

**Why:** Result types:
- Force error handling (exhaustive matching)
- Preserve error messages
- Type-safe (can't ignore errors)
- Clear in function signatures

## Compilation Flags

### Debug Build (Development)

```bash
gcc -std=c11 -g -O0 \
    -fsanitize=address,undefined \
    -fstack-protector-strong \
    -Wall -Wextra -Wpedantic \
    -fanalyzer \
    -ftrack-macro-expansion=0 \
    -Ideps -Isrc \
    src/*.c deps/sds.c \
    -o fern
```

**Required flags:**
- `-std=c11` - Use C11 standard
- `-ftrack-macro-expansion=0` - Required for Datatype99 (GCC)
- `-fmacro-backtrace-limit=1` - Required for Datatype99 (Clang)

**Safety flags (debug only):**
- `-fsanitize=address` - Detect buffer overflows, use-after-free
- `-fsanitize=undefined` - Detect undefined behavior
- `-fstack-protector-strong` - Stack overflow protection
- `-fanalyzer` - Static analysis (GCC 10+)

### Release Build (Production)

```bash
gcc -std=c11 -O2 -DNDEBUG \
    -ftrack-macro-expansion=0 \
    -Ideps -Isrc \
    src/*.c deps/sds.c \
    -o fern
```

## Common Patterns

### String Interning

```c
typedef struct {
    sds key;
    int id;
} InternEntry;

InternEntry *intern_table = NULL;  // stb_ds hash map
int next_id = 0;

int intern_string(const char *str) {
    int idx = shgeti(intern_table, str);
    if (idx >= 0) {
        return intern_table[idx].id;
    }
    
    InternEntry entry = {
        .key = sdsnew(str),
        .id = next_id++
    };
    shput(intern_table, entry.key, entry.id);
    return entry.id;
}
```

### Symbol Table

```c
typedef struct {
    sds name;
    Type *type;
    int scope_depth;
} Symbol;

Symbol *symbols = NULL;  // Dynamic array

void add_symbol(const char *name, Type *type, int depth) {
    Symbol sym = {
        .name = sdsnew(name),
        .type = type,
        .scope_depth = depth
    };
    arrput(symbols, sym);
}

Symbol* lookup_symbol(const char *name) {
    // Search backwards (innermost scope first)
    for (int i = arrlen(symbols) - 1; i >= 0; i--) {
        if (strcmp(symbols[i].name, name) == 0) {
            return &symbols[i];
        }
    }
    return NULL;
}
```

### Error Messages

```c
datatype(
    ErrorType,
    (SyntaxError, sds),
    (TypeError, sds),
    (NameError, sds)
);

typedef struct {
    ErrorType type;
    sds filename;
    int line;
    int column;
} CompileError;

CompileError make_error(ErrorType type, const char *file, int line, int col) {
    return (CompileError){
        .type = type,
        .filename = sdsnew(file),
        .line = line,
        .column = col
    };
}

void print_error(CompileError *err) {
    fprintf(stderr, "%s:%d:%d: ", err->filename, err->line, err->column);
    
    match(err->type) {
        of(SyntaxError, msg) {
            fprintf(stderr, "Syntax error: %s\n", *msg);
        }
        of(TypeError, msg) {
            fprintf(stderr, "Type error: %s\n", *msg);
        }
        of(NameError, msg) {
            fprintf(stderr, "Name error: %s\n", *msg);
        }
    }
}
```

## QBE Code Generation

### Basic Template

```c
void codegen_function(FILE *out, Function *fn) {
    // Function signature
    fprintf(out, "export function w $%s(", fn->name);
    
    for (int i = 0; i < arrlen(fn->params); i++) {
        if (i > 0) fprintf(out, ", ");
        fprintf(out, "w %%arg%d", i);
    }
    
    fprintf(out, ") {\n@start\n");
    
    // Function body
    for (int i = 0; i < arrlen(fn->body); i++) {
        codegen_stmt(out, fn->body[i]);
    }
    
    fprintf(out, "}\n\n");
}

sds codegen_expr(FILE *out, Expr *expr, int *tmp_counter) {
    match(*expr) {
        of(IntLit, n) {
            sds tmp = sdscatprintf(sdsempty(), "%%t%d", (*tmp_counter)++);
            fprintf(out, "    %s =w copy %ld\n", tmp, *n);
            return tmp;
        }
        
        of(BinOp, lhs, op, rhs) {
            sds left = codegen_expr(out, *lhs, tmp_counter);
            sds right = codegen_expr(out, *rhs, tmp_counter);
            sds result = sdscatprintf(sdsempty(), "%%t%d", (*tmp_counter)++);
            
            const char *qbe_op = NULL;
            switch (*op) {
                case TOK_PLUS:  qbe_op = "add"; break;
                case TOK_MINUS: qbe_op = "sub"; break;
                case TOK_STAR:  qbe_op = "mul"; break;
                case TOK_SLASH: qbe_op = "div"; break;
            }
            
            fprintf(out, "    %s =w %s %s, %s\n", result, qbe_op, left, right);
            
            sdsfree(left);
            sdsfree(right);
            return result;
        }
    }
    
    return NULL;
}
```

## Testing Pattern

```c
// test_lexer.c
#include <assert.h>
#include "lexer.h"

void test_lex_integer() {
    CompilerContext ctx = {0};
    Lexer *lex = lexer_new(&ctx, "42");
    Token tok = next_token(lex);
    
    assert(tok.type == TOK_INT);
    assert(strcmp(tok.text, "42") == 0);
    
    ctx_free(&ctx);
}

int main() {
    test_lex_integer();
    // ... more tests
    
    printf("All tests passed!\n");
    return 0;
}
```

## DO NOT Do These Things

### ❌ Manual Memory Management
```c
// WRONG
char *s = malloc(100);
strcpy(s, "hello");
// ... forget to free, or double free
```

### ❌ Manual Tagged Unions
```c
// WRONG
typedef struct {
    enum { INT, STRING } kind;
    union { int i; char *s; } data;
} BadVariant;
```

### ❌ Null-Terminated String Handling
```c
// WRONG
char buf[100];
strcpy(buf, input);  // Buffer overflow
strcat(buf, more);   // More overflow risk
```

### ❌ Returning NULL for Errors
```c
// WRONG
Expr* parse() {
    if (error) return NULL;  // Lost context
    return expr;
}
```

### ❌ Manual Array Management
```c
// WRONG
int size = 10;
Expr **arr = malloc(size * sizeof(Expr*));
// Manual resize, easy to get wrong
```

## Summary Checklist

Before committing code, verify:

- [ ] All allocations use arena (no malloc/free)
- [ ] All variants use Datatype99 (no manual unions)
- [ ] All strings use SDS (no char*)
- [ ] All arrays/maps use stb_ds (no manual management)
- [ ] All fallible functions return Result types
- [ ] All match() statements are exhaustive
- [ ] Compiles with `-Wall -Wextra -Wpedantic` (no warnings)
- [ ] Passes AddressSanitizer (no memory errors)
- [ ] Passes UndefinedBehaviorSanitizer (no UB)

## Resources

- **Arena**: https://github.com/tsoding/arena
- **Datatype99**: https://github.com/Hirrolot/datatype99
- **SDS**: https://github.com/antirez/sds
- **stb_ds.h**: https://github.com/nothings/stb
- **QBE**: https://c9x.me/compile/

## Key Insight for AI

The entire safety strategy is:
1. **Memory**: Never free individually, use arenas
2. **Variants**: Never manual tags, use Datatype99
3. **Strings**: Never char*, use SDS
4. **Collections**: Never manual arrays, use stb_ds
5. **Errors**: Never NULL, use Result types

These five rules eliminate 90% of C's danger while keeping AI productivity high.
