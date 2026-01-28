# Ralph Loop Scripts

Scripts for running the autonomous Ralph Loop development system.

## Quick Start

```bash
# 1. Initialize Ralph Loop
./scripts/ralph-init.sh

# 2. Run one iteration
./scripts/ralph-iteration.sh

# 3. Repeat step 2 until milestone complete
```

## Files

- `ralph-init.sh` - Initialize Ralph Loop in ROADMAP.md
- `ralph-iteration.sh` - Run one iteration (IMPLEMENTER or CONTROLLER)
- `ralph-prompts.md` - Agent prompts and instructions
- `README.md` - This file

## How It Works

The Ralph Loop uses ROADMAP.md as shared state between two agents:

1. **CONTROLLER** - Selects tasks, verifies implementations
2. **IMPLEMENTER** - Writes tests, implements features (TDD)

Each iteration:
1. CONTROLLER selects next task → hands off to IMPLEMENTER
2. IMPLEMENTER writes tests (RED) → implements (GREEN) → hands off to CONTROLLER
3. CONTROLLER verifies → ACCEPT (next task) or REJECT (fix issues)
4. Repeat

## Manual Mode (Current)

The scripts currently run in simulation mode. To use:

1. Run `ralph-iteration.sh` to see the current agent and task
2. The script will show you what to do
3. Manually complete the task
4. Update ROADMAP.md with results
5. Run `ralph-iteration.sh` again

## Future: Automated Mode

To fully automate, the scripts need Claude Code integration:

```bash
# Pseudo-code for automation
if [ "$AGENT_TURN" = "IMPLEMENTER" ]; then
    claude code --task "$(extract_task_from_roadmap)" \
                --system-prompt "$(get_implementer_prompt)" \
                --callback "update_roadmap_with_results"
fi
```

## ROADMAP.md Format

The Ralph Loop Status section in ROADMAP.md:

```markdown
## Ralph Loop Status

**Current Milestone**: 2 - Parser
**Current Iteration**: 5
**Agent Turn**: IMPLEMENTER | CONTROLLER
**Status**: IN_PROGRESS | WAITING_VERIFICATION | BLOCKED | MILESTONE_COMPLETE

### Current Task

- [ ] Task description

**Expected Tests**: test_foo(), test_bar()
**Expected Files**: lib/foo.c, include/foo.h
**Success Criteria**: All tests pass, no warnings

### Implementation Notes

(IMPLEMENTER fills this after completing work)

### Verification Notes

(CONTROLLER fills this after verification)

### Blockers

(Either agent documents issues)
```

## Monitoring

Check progress:

```bash
# View current status
sed -n '/## Ralph Loop Status/,/^---$/p' ROADMAP.md

# View iteration count
grep "Current Iteration" ROADMAP.md

# View blockers
sed -n '/### Blockers/,/^$/p' ROADMAP.md
```

## Troubleshooting

### Status is stuck

If the status seems stuck, check:
1. Is `Agent Turn` correct?
2. Is `Status` correct?
3. Are there blockers documented?

### Agent confusion

If an agent doesn't know what to do:
1. Check the "Current Task" section
2. Verify success criteria are clear
3. Check that expected tests/files are listed

### Tests failing

If tests are failing:
1. IMPLEMENTER should fix and update to WAITING_VERIFICATION
2. CONTROLLER should document in Blockers and set status to BLOCKED

## Best Practices

1. **Keep tasks small** - Each task should be completable in 1 iteration
2. **Clear success criteria** - CONTROLLER needs objective criteria
3. **Good test names** - Expected tests should be descriptive
4. **Atomic commits** - One feature per commit
5. **Update ROADMAP** - Always update after completing work

## Examples

See `RALPH.md` for detailed examples and architecture.
