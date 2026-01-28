# Ralph Loop - Autonomous Development System

**Ralph** = **R**oadmap **A**gent **L**oop with **P**lanning and **H**andoff

A two-agent system where agents communicate via ROADMAP.md to systematically implement the Fern compiler.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     ROADMAP.md (Shared State)                │
│  - Current milestone status                                  │
│  - Next task to implement                                    │
│  - Test requirements                                         │
│  - Success criteria                                          │
└─────────────────────────────────────────────────────────────┘
         ↑                                      ↓
         │                                      │
    Write Status                           Read Task
         │                                      │
┌────────┴────────┐                   ┌────────┴────────┐
│   IMPLEMENTER   │                   │   CONTROLLER    │
│                 │                   │                 │
│ 1. Read next    │                   │ 1. Read status  │
│    task from    │                   │    from         │
│    ROADMAP.md   │                   │    ROADMAP.md   │
│                 │                   │                 │
│ 2. Write tests  │                   │ 2. Verify tests │
│    (TDD RED)    │                   │    pass         │
│                 │                   │                 │
│ 3. Implement    │                   │ 3. Review code  │
│    feature      │                   │    quality      │
│                 │                   │                 │
│ 4. Run tests    │                   │ 4. Check        │
│    (TDD GREEN)  │                   │    criteria     │
│                 │                   │                 │
│ 5. Commit       │                   │ 5. Mark done in │
│                 │                   │    ROADMAP.md   │
│ 6. Update       │                   │                 │
│    ROADMAP.md   │                   │ 6. Select next  │
│    with status  │                   │    task         │
└─────────────────┘                   └─────────────────┘
         │                                      ↑
         │                                      │
         └──────────── Handoff ─────────────────┘
```

## Communication Protocol

### ROADMAP.md Markers

The agents use special markers in ROADMAP.md to communicate:

```markdown
## Current Iteration

**Agent**: IMPLEMENTER | CONTROLLER
**Iteration**: 3
**Status**: IN_PROGRESS | WAITING_VERIFICATION | COMPLETE | BLOCKED

### Current Task

- [ ] Task description from milestone
- **Expected Tests**: List of test functions to write
- **Expected Files**: Files that will be created/modified
- **Success Criteria**: Specific criteria to meet

### Implementation Notes

(IMPLEMENTER writes notes here about what was done)

### Verification Notes

(CONTROLLER writes notes here about verification results)

### Blockers

(Either agent can note blockers here)
```

## Agent Roles

### IMPLEMENTER Agent

**Model**: Claude Haiku 4.5 (default, budget-conscious)
**Upgrade**: Sonnet 4.5 only if task is too complex for Haiku
**Budget**: Stay within $100/month Max Plan limit

**Responsibilities:**
1. Read next task from ROADMAP.md
2. Follow TDD strictly:
   - Write tests FIRST (RED phase)
   - Commit tests with `--no-verify`
   - Implement feature
   - Make tests pass (GREEN phase)
   - Refactor if needed
3. Update ROADMAP.md with:
   - Status: WAITING_VERIFICATION
   - Implementation notes
   - Files changed
   - Tests written/passing
4. Commit work using conventional commits

**Constraints:**
- MUST follow TDD (tests first)
- MUST update ROADMAP.md after each task
- MUST commit atomically (one feature at a time)
- CANNOT mark tasks as COMPLETE (only CONTROLLER can)

### CONTROLLER Agent

**Model**: Claude Haiku 4.5 (verification is simple, Haiku is sufficient)

**Responsibilities:**
1. Read ROADMAP.md status
2. Verify implementation:
   - All tests pass
   - Code quality acceptable
   - Success criteria met
   - No regressions
3. Update ROADMAP.md with:
   - Mark task complete (✅)
   - Select next task
   - Set status: IN_PROGRESS
   - Write verification notes
4. Decide when to move to next milestone

**Constraints:**
- MUST verify all success criteria
- MUST check test coverage
- CAN request fixes by setting status: BLOCKED
- CANNOT implement (only verify)

## Workflow

### Iteration N

1. **CONTROLLER** reads ROADMAP.md, selects next task, writes:
   ```markdown
   **Agent**: IMPLEMENTER
   **Status**: IN_PROGRESS
   ### Current Task
   - [ ] Implement X feature
   ```

2. **IMPLEMENTER** reads task, implements following TDD:
   - Writes tests
   - Implements feature
   - Commits
   - Updates ROADMAP.md:
     ```markdown
     **Agent**: CONTROLLER
     **Status**: WAITING_VERIFICATION
     ### Implementation Notes
     - Wrote 5 tests in tests/test_X.c
     - Implemented lib/X.c and include/X.h
     - All 41 tests passing
     - Commits: abc123, def456
     ```

3. **CONTROLLER** verifies:
   - Runs `make test`
   - Reviews code
   - Checks success criteria
   - Updates ROADMAP.md:
     ```markdown
     **Agent**: IMPLEMENTER
     **Status**: IN_PROGRESS
     - [x] Implement X feature ✅
     ### Verification Notes
     - All tests pass (41/41)
     - Code quality: good
     - Success criteria: met
     ### Current Task
     - [ ] Next feature Y
     ```

4. Loop continues...

## ROADMAP.md Structure

The ROADMAP.md file serves as the shared memory:

```markdown
# Fern Implementation Roadmap

## Ralph Loop Status

**Current Milestone**: 2 - Parser
**Current Iteration**: 5
**Agent Turn**: IMPLEMENTER
**Status**: IN_PROGRESS

### Current Task

- [ ] Implement if expression parsing

**Expected Tests**:
- test_parse_if_simple()
- test_parse_if_else()
- test_parse_if_nested()

**Expected Files**:
- lib/parser.c (modify parse_primary)
- tests/test_parser.c (add tests)

**Success Criteria**:
- All new tests pass
- No regression in existing tests
- Follows precedence rules

### Implementation Notes

(Written by IMPLEMENTER after completing work)

### Verification Notes

(Written by CONTROLLER after verification)

### Blockers

(Either agent can document issues)

---

## Milestone 2: Parser

**Status**: 🚧 In Progress - Core parser complete, 36/36 tests passing

... rest of milestone content ...
```

## Running the Loop

### Option 1: Sequential Invocation

```bash
# Terminal 1: IMPLEMENTER
while true; do
  claude --agent implementer --file ROADMAP.md
  # Waits for CONTROLLER
done

# Terminal 2: CONTROLLER  
while true; do
  claude --agent controller --file ROADMAP.md
  # Waits for IMPLEMENTER
done
```

### Option 2: Turn-Based in Single Session

```bash
# Run one complete iteration
./scripts/ralph-iteration.sh

# The script alternates:
# 1. CONTROLLER selects task
# 2. IMPLEMENTER implements
# 3. CONTROLLER verifies
# 4. Repeat
```

### Option 3: Using Claude Code Skills

Create two skills:

1. `/ralph-implement` - Launches IMPLEMENTER agent
2. `/ralph-verify` - Launches CONTROLLER agent

## Benefits

1. **Clear Handoff**: ROADMAP.md provides explicit communication
2. **Fault Tolerance**: If an agent fails, state is preserved in ROADMAP.md
3. **Auditability**: Complete history in ROADMAP.md and git commits
4. **Parallelizable**: Could run multiple IMPLEMENTER agents on different tasks
5. **Human Oversight**: Human can review ROADMAP.md and intervene
6. **TDD Enforced**: IMPLEMENTER must follow RED-GREEN-REFACTOR
7. **Quality Gate**: CONTROLLER ensures standards before proceeding

## Example ROADMAP.md Entry

```markdown
## Ralph Loop Status - Iteration 7

**Current Milestone**: 2 - Parser
**Agent Turn**: CONTROLLER
**Status**: WAITING_VERIFICATION
**Started**: 2024-01-28 14:30:22
**Last Updated**: 2024-01-28 14:45:18

### Current Task

- [x] Implement if expression parsing

**Expected Tests**:
- test_parse_if_simple() ✅
- test_parse_if_else() ✅
- test_parse_if_nested() ✅

**Expected Files**:
- lib/parser.c ✅
- tests/test_parser.c ✅

**Success Criteria**:
- [x] All new tests pass (3/3)
- [x] No regression in existing tests (39/39 total)
- [x] Follows precedence rules

### Implementation Notes

**Written by**: IMPLEMENTER
**Time**: 2024-01-28 14:45:18

Implemented if expression parsing following the grammar:

```
if_expr := "if" expr ":" expr ["else" ":" expr]
```

Changes:
- Added EXPR_IF handling in parse_primary
- Wrote 3 comprehensive tests
- All tests passing (39/39)

Commits:
- test(parser): add if expression tests (RED phase) - commit abc123
- feat(parser): implement if expression parsing (GREEN phase) - commit def456

### Verification Notes

**Written by**: CONTROLLER
**Time**: [Pending]

[To be filled by CONTROLLER]

### Blockers

None

---

### Next Tasks Queue

1. [ ] Implement match expression parsing
2. [ ] Implement block expressions
3. [ ] Implement function definition parsing
4. [ ] Add type annotation parsing
```

## Monitoring and Control

### Human Checkpoints

Humans should review:
- After each milestone
- When status is BLOCKED
- Every N iterations (e.g., every 5)

### Quality Metrics

Track in ROADMAP.md:
- Tests written vs passing
- Code coverage
- Compilation warnings
- Time per iteration
- Blocker frequency

## Cost Optimization

### Model Selection Strategy (Budget-Conscious)

**⚠️ IMPORTANT: Monthly Budget = $100 (Max Plan)**

Current status (Jan 2026): $110.16 spent - OVER BUDGET
Strategy: Use Haiku 4.5 as default to stay within budget

**IMPLEMENTER**: Claude Haiku 4.5 (default)
- Haiku is surprisingly capable for focused tasks
- Excellent test coverage guides implementation
- Clear specifications in DESIGN.md reduce complexity
- **Cost**: ~$0.02-0.04 per iteration (20x cheaper than Opus!)
- **Upgrade to Sonnet**: Only if Haiku genuinely struggles

**CONTROLLER**: Claude Haiku 4.5 (always)
- Verification is straightforward (run tests, check criteria)
- Haiku is perfect for this role
- **Cost**: ~$0.01-0.02 per iteration

**Token Management**:
- Monitor with `ccusage` command daily
- Check budget: `ccusage | grep Total`
- Target: <$3.33/day average ($100/30 days)
- Stop if approaching monthly limit

**Cost Projection (Revised)**:
- IMPLEMENTER (Haiku): ~$0.02-0.04 per iteration
- CONTROLLER (Haiku): ~$0.01-0.02 per iteration
- **Total**: ~$0.03-0.06 per iteration
- **Full milestone (20 iterations)**: ~$0.60-1.20
- **Budget allows**: 60-100 iterations per month!

### Model Upgrade Path

Start with Haiku, upgrade only if needed:

1. **Try Haiku first** (default)
   - Most tasks work fine with Haiku
   - Great for TDD with good tests
   
2. **Upgrade to Sonnet if**:
   - Haiku produces buggy code repeatedly
   - Complex algorithm needs deeper reasoning
   - Architecture decision needed
   
3. **Never use Opus**:
   - Too expensive for $100/month budget
   - Haiku + good tests > Opus alone

### When to Use Each Model

**Haiku 4.5** (USE FOR EVERYTHING):
- ✅ All implementation work (yes!)
- ✅ All verification tasks
- ✅ Documentation updates
- ✅ Test writing
- ✅ Debugging
- ✅ Code reviews
- ✅ Formatting/linting

**Sonnet 4.5** (ONLY IF HAIKU FAILS):
- ⚠️ Complex algorithms after Haiku fails
- ⚠️ Tricky bugs Haiku can't solve
- ⚠️ Architecture decisions

**Opus 4.5** (NEVER):
- ❌ Too expensive - avoid completely
- ❌ Not worth cost for this budget

## Safety Measures

1. **Max Iterations**: Stop after N iterations without CONTROLLER approval
2. **Test Coverage**: Require minimum test coverage
3. **No Compile Warnings**: Strict -Werror enforcement
4. **Commit Atomicity**: One feature per commit
5. **Rollback**: CONTROLLER can request rollback if quality issues
6. **Token Budget**: Monitor usage with ccusage, stop at 180k/200k tokens

## Next Steps

To implement Ralph Loop:

1. ✅ Create this RALPH.md design document
2. [ ] Add Ralph Loop Status section to ROADMAP.md
3. [ ] Create IMPLEMENTER agent prompt/skill
4. [ ] Create CONTROLLER agent prompt/skill
5. [ ] Create helper script `scripts/ralph-iteration.sh`
6. [ ] Test with one iteration manually
7. [ ] Automate the loop
8. [ ] Add monitoring dashboard
