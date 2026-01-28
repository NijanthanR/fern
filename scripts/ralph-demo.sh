#!/usr/bin/env bash
# Ralph Loop - Simple Demo
#
# This script demonstrates the Ralph Loop by showing what each agent would do

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
NC='\033[0m'

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║              Ralph Loop - Live Demo                      ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

echo -e "${CYAN}This demo shows how the Ralph Loop works with a simple task.${NC}"
cat << 'EOF'

Task: Implement if expression parsing
Goal: Add 2 tests, implement feature, verify

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

${PURPLE}ITERATION 1: IMPLEMENTER${NC}

${BLUE}Step 1: IMPLEMENTER reads ROADMAP.md${NC}

Current Task:
  - [ ] Implement if expression parsing

Expected Tests:
  - test_parse_if_simple()
  - test_parse_if_else()

Expected Files:
  - tests/test_parser.c (add tests)
  - lib/parser.c (implement)

${BLUE}Step 2: IMPLEMENTER writes tests (RED phase)${NC}

I would add to tests/test_parser.c:
${GREEN}
void test_parse_if_simple(void) {
    Arena* arena = arena_create(4096);
    Parser* parser = parser_new(arena, "if true: 42");

    Expr* expr = parse_expr(parser);
    ASSERT_EQ(expr->type, EXPR_IF);
    // ... more assertions

    arena_destroy(arena);
}

void test_parse_if_else(void) {
    // Similar test with else branch
}
${NC}

Run: make test
Result: ${YELLOW}FAIL${NC} - EXPR_IF not implemented yet

Commit: git commit --no-verify -m "test(parser): add if tests (RED)"

${BLUE}Step 3: IMPLEMENTER implements feature (GREEN phase)${NC}

I would add to lib/parser.c:
${GREEN}
// In parse_primary_internal():
if (match(parser, TOKEN_IF)) {
    Expr* condition = parse_expression(parser);
    consume(parser, TOKEN_COLON, "Expected ':' after if condition");
    Expr* then_branch = parse_expression(parser);

    Expr* else_branch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        consume(parser, TOKEN_COLON, "Expected ':' after else");
        else_branch = parse_expression(parser);
    }

    return expr_if(parser->arena, condition, then_branch,
                   else_branch, parser->previous.loc);
}
${NC}

Also add expr_if() helper function to lib/ast.c

Run: make test
Result: ${GREEN}PASS${NC} - 38/38 tests passing!

Commit: git commit -m "feat(parser): implement if expressions (GREEN)"

${BLUE}Step 4: IMPLEMENTER updates ROADMAP.md${NC}

Updates Ralph Loop Status section:
  - Agent Turn: CONTROLLER
  - Status: WAITING_VERIFICATION
  - Implementation Notes: (what was done)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

${PURPLE}ITERATION 2: CONTROLLER${NC}

${BLUE}Step 1: CONTROLLER reads ROADMAP.md${NC}

Status: WAITING_VERIFICATION
Implementation Notes: (reads what IMPLEMENTER did)

${BLUE}Step 2: CONTROLLER verifies${NC}

Run: make clean && make test
Result: ${GREEN}38/38 tests passing${NC} ✓

Review code: git diff HEAD~2
Checks:
  ✓ Uses arena allocation
  ✓ No compiler warnings
  ✓ Follows existing patterns
  ✓ Tests are comprehensive

${BLUE}Step 3: CONTROLLER decides${NC}

Decision: ${GREEN}ACCEPT${NC} ✓

Updates ROADMAP.md:
  - Mark task: [x] Implement if expression parsing ✅
  - Status: IN_PROGRESS
  - Agent Turn: IMPLEMENTER
  - Verification Notes: (results)
  - Next Task: Implement match expression parsing

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

${CYAN}Loop continues with next task...${NC}

Key Points:
  ✓ ROADMAP.md is the shared state
  ✓ Strict TDD: RED → GREEN → VERIFY
  ✓ Quality gate: CONTROLLER can reject
  ✓ Full audit trail in ROADMAP.md and git

EOF

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "${GREEN}Want to try it for real?${NC}"
echo ""
echo "1. Manual test (recommended):"
echo "   ${YELLOW}cat RALPH_TEST.md${NC} - Read the step-by-step guide"
echo ""
echo "2. Use Claude Code as IMPLEMENTER:"
echo "   - Read scripts/ralph-prompts.md for IMPLEMENTER prompt"
echo "   - Invoke Claude Code with Opus 4.5"
echo "   - Give it the IMPLEMENTER task from ROADMAP.md"
echo ""
echo "3. Use Claude Code as CONTROLLER:"
echo "   - Read scripts/ralph-prompts.md for CONTROLLER prompt"
echo "   - Invoke Claude Code with Sonnet 4.5"
echo "   - Give it the verification task"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
