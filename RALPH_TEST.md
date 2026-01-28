# Testing the Ralph Loop

This guide walks through a simple test of the Ralph Loop system.

## Quick Test (Automated Setup)

```bash
# Initialize and set up a test task
./scripts/ralph-test.sh

# Then manually complete the task or use the iteration script
./scripts/ralph-iteration.sh
```

## Manual Test (Step-by-Step)

### Step 1: Initialize Ralph Loop

```bash
./scripts/ralph-init.sh
```

This adds the Ralph Loop Status section to ROADMAP.md.

### Step 2: Set Up a Test Task (CONTROLLER role)

Edit ROADMAP.md and update the Ralph Loop Status section:

```markdown
## Ralph Loop Status

**Current Milestone**: 2 - Parser
**Current Iteration**: 1
**Agent Turn**: IMPLEMENTER
**Status**: IN_PROGRESS
**Started**: 2026-01-28
**Last Updated**: 2026-01-28

### Current Task

- [ ] Implement if expression parsing (test task)

**Expected Tests**:
- test_parse_if_simple() - Parse: if true: 42
- test_parse_if_else() - Parse: if x > 0: 1 else: 0

**Expected Files**:
- tests/test_parser.c (add 2 tests)
- lib/parser.c (add if parsing)

**Success Criteria**:
- Both new tests pass
- No regression in existing 36 tests
- No compiler warnings

### Implementation Notes

(IMPLEMENTER will fill this)

### Verification Notes

(CONTROLLER will fill this)

### Blockers

None
```

### Step 3: Implement (IMPLEMENTER role)

Now act as the IMPLEMENTER agent:

#### 3a. Read the specification

Check DESIGN.md for if expression syntax:
```bash
grep -A 20 "if expression" DESIGN.md
```

#### 3b. Write tests FIRST (RED phase)

Add to `tests/test_parser.c`:

```c
/* Test: Parse simple if expression */
void test_parse_if_simple(void) {
    Arena* arena = arena_create(4096);
    Parser* parser = parser_new(arena, "if true: 42");
    
    Expr* expr = parse_expr(parser);
    ASSERT_NOT_NULL(expr);
    ASSERT_EQ(expr->type, EXPR_IF);
    ASSERT_EQ(expr->data.if_expr.condition->type, EXPR_BOOL_LIT);
    ASSERT_EQ(expr->data.if_expr.then_branch->type, EXPR_INT_LIT);
    ASSERT_NULL(expr->data.if_expr.else_branch);
    
    arena_destroy(arena);
}

/* Test: Parse if-else expression */
void test_parse_if_else(void) {
    Arena* arena = arena_create(4096);
    Parser* parser = parser_new(arena, "if x > 0: 1 else: 0");
    
    Expr* expr = parse_expr(parser);
    ASSERT_NOT_NULL(expr);
    ASSERT_EQ(expr->type, EXPR_IF);
    ASSERT_NOT_NULL(expr->data.if_expr.condition);
    ASSERT_NOT_NULL(expr->data.if_expr.then_branch);
    ASSERT_NOT_NULL(expr->data.if_expr.else_branch);
    
    arena_destroy(arena);
}
```

Add to test runner:
```c
void run_parser_tests(void) {
    // ... existing tests ...
    TEST_RUN(test_parse_if_simple);
    TEST_RUN(test_parse_if_else);
}
```

#### 3c. Run tests (should FAIL - RED)

```bash
make clean && make test
# Should fail: EXPR_IF not handled in parser
```

#### 3d. Commit tests

```bash
git add tests/test_parser.c
git commit --no-verify -m "test(parser): add if expression tests (TDD - RED phase)"
```

#### 3e. Implement if parsing (GREEN phase)

Add to `lib/parser.c` in `parse_primary_internal()`:

```c
// If expression
if (match(parser, TOKEN_IF)) {
    SourceLoc loc = parser->previous.loc;
    
    Expr* condition = parse_expression(parser);
    consume(parser, TOKEN_COLON, "Expected ':' after if condition");
    Expr* then_branch = parse_expression(parser);
    
    Expr* else_branch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        consume(parser, TOKEN_COLON, "Expected ':' after else");
        else_branch = parse_expression(parser);
    }
    
    return expr_if(parser->arena, condition, then_branch, else_branch, loc);
}
```

Add helper to `lib/ast.c`:

```c
Expr* expr_if(Arena* arena, Expr* condition, Expr* then_branch, Expr* else_branch, SourceLoc loc) {
    assert(arena != NULL);
    assert(condition != NULL);
    assert(then_branch != NULL);
    
    Expr* expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_IF;
    expr->loc = loc;
    expr->data.if_expr.condition = condition;
    expr->data.if_expr.then_branch = then_branch;
    expr->data.if_expr.else_branch = else_branch;
    
    return expr;
}
```

Add to `include/ast.h`:
```c
Expr* expr_if(Arena* arena, Expr* condition, Expr* then_branch, Expr* else_branch, SourceLoc loc);
```

#### 3f. Run tests (should PASS - GREEN)

```bash
make clean && make test
# Should pass: 38/38 tests
```

#### 3g. Commit implementation

```bash
git add lib/parser.c lib/ast.c include/ast.h
git commit -m "feat(parser): implement if expression parsing (TDD - GREEN phase)"
```

#### 3h. Update ROADMAP.md

Edit the Ralph Loop Status section:

```markdown
**Agent Turn**: CONTROLLER
**Status**: WAITING_VERIFICATION
**Last Updated**: 2026-01-28 15:30:00

### Implementation Notes

**Written by**: IMPLEMENTER
**Time**: 2026-01-28 15:30:00

Implemented if expression parsing with optional else branch.

Changes:
- Added test_parse_if_simple() - basic if without else
- Added test_parse_if_else() - if with else branch
- Implemented if parsing in parse_primary_internal()
- Added expr_if() helper function
- All tests passing (38/38)

Commits:
- test(parser): add if expression tests (RED) - abc1234
- feat(parser): implement if expression parsing (GREEN) - def5678

Files Changed:
- tests/test_parser.c (+30 lines)
- lib/parser.c (+15 lines)
- lib/ast.c (+15 lines)
- include/ast.h (+1 line)

### Blockers

None
```

### Step 4: Verify (CONTROLLER role)

Now act as the CONTROLLER agent:

#### 4a. Run tests

```bash
make clean && make test
# Verify: 38/38 tests passing
```

#### 4b. Review code

```bash
git log --oneline -2
git diff HEAD~2
```

Check:
- ✅ Uses arena allocation
- ✅ No compiler warnings
- ✅ Follows existing patterns
- ✅ Tests are comprehensive

#### 4c. Update ROADMAP.md

```markdown
**Agent Turn**: IMPLEMENTER
**Status**: IN_PROGRESS
**Last Updated**: 2026-01-28 15:45:00

### Current Task

- [x] Implement if expression parsing ✅ VERIFIED

### Verification Notes

**Written by**: CONTROLLER
**Time**: 2026-01-28 15:45:00

✅ ACCEPTED

Verification Results:
- Tests: 38/38 passing (2 new if tests)
- Code quality: Excellent
- No compiler warnings
- Uses arena allocation correctly
- Error messages clear
- Follows existing parser patterns

Success Criteria:
- [x] Both new tests pass
- [x] No regression (36 existing tests still pass)
- [x] No compiler warnings

Ready for next task!

---

### Next Task

- [ ] Implement match expression parsing

(Set up next task or mark milestone complete)
```

## What This Tests

✅ **Communication**: IMPLEMENTER → ROADMAP.md → CONTROLLER
✅ **TDD Workflow**: RED → GREEN → Verification
✅ **State Management**: ROADMAP.md preserves state between steps
✅ **Quality Gate**: CONTROLLER can accept/reject work
✅ **Documentation**: All work is documented

## Success Criteria

After completing the test:

1. ✅ Tests go from 36/36 to 38/38
2. ✅ All tests passing
3. ✅ Zero compiler warnings
4. ✅ ROADMAP.md updated with complete notes
5. ✅ Two commits following TDD (RED, then GREEN)

## Cleanup

To reset after testing:

```bash
# Restore ROADMAP.md
git checkout ROADMAP.md

# Or keep the changes if test worked well
git add ROADMAP.md
git commit -m "test: verify Ralph Loop works with if expression task"
```

## Next Steps

If the test succeeds:

1. Continue with more parser tasks
2. Complete Milestone 2
3. Move to Milestone 3 (Type System)

The Ralph Loop is ready for production use! 🚀
