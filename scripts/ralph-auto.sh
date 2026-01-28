#!/usr/bin/env bash
# Ralph Loop - Automated Runner
#
# This script runs Ralph Loop iterations automatically overnight

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ROADMAP_FILE="$PROJECT_ROOT/ROADMAP.md"

# Configuration
MAX_ITERATIONS=${MAX_ITERATIONS:-10}  # Stop after 10 iterations
SLEEP_BETWEEN=${SLEEP_BETWEEN:-5}     # Wait 5 seconds between iterations

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

log() {
    echo -e "${BLUE}[$(date +'%H:%M:%S')]${NC} $*"
}

success() {
    echo -e "${GREEN}[$(date +'%H:%M:%S')]${NC} $*"
}

error() {
    echo -e "${RED}[$(date +'%H:%M:%S')]${NC} $*" >&2
}

warning() {
    echo -e "${YELLOW}[$(date +'%H:%M:%S')]${NC} $*"
}

get_status_field() {
    local field="$1"
    grep "^\*\*${field}\*\*:" "$ROADMAP_FILE" | head -1 | sed "s/^\*\*${field}\*\*: *//" || echo ""
}

# Main loop
main() {
    cd "$PROJECT_ROOT"

    echo ""
    echo "╔═══════════════════════════════════════════════════════════╗"
    echo "║         Ralph Loop - Automated Runner                    ║"
    echo "╚═══════════════════════════════════════════════════════════╝"
    echo ""

    log "Starting automated Ralph Loop"
    log "Max iterations: $MAX_ITERATIONS"
    log "Sleep between: ${SLEEP_BETWEEN}s"
    echo ""

    for i in $(seq 1 $MAX_ITERATIONS); do
        log "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        log "Iteration $i of $MAX_ITERATIONS"
        log "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

        # Read current state
        AGENT_TURN=$(get_status_field "Agent Turn")
        STATUS=$(get_status_field "Status")
        ITERATION=$(get_status_field "Current Iteration")

        log "Agent: $AGENT_TURN | Status: $STATUS | Iteration: $ITERATION"

        # Check if we should stop
        if [ "$STATUS" = "MILESTONE_COMPLETE" ]; then
            success "Milestone complete! Stopping."
            break
        fi

        if [ "$STATUS" = "BLOCKED" ]; then
            error "Status is BLOCKED. Human intervention needed."
            echo ""
            echo "Blockers:"
            sed -n '/### Blockers/,/^$/p' "$ROADMAP_FILE"
            break
        fi

        # For now, just show what would happen
        # In production, this would invoke Claude Code

        if [ "$AGENT_TURN" = "IMPLEMENTER" ] && [ "$STATUS" = "IN_PROGRESS" ]; then
            warning "Would launch IMPLEMENTER agent here"
            echo ""
            echo "In production mode, this would:"
            echo "  1. Extract IMPLEMENTER prompt from scripts/ralph-prompts.md"
            echo "  2. Extract current task from ROADMAP.md"
            echo "  3. Invoke: claude-code --model opus-4-5 --task \"<task>\""
            echo "  4. Wait for completion"
            echo ""
            echo "For now, stopping. Manual implementation needed."
            break

        elif [ "$AGENT_TURN" = "CONTROLLER" ] && [ "$STATUS" = "WAITING_VERIFICATION" ]; then
            log "Running CONTROLLER verification"

            # CONTROLLER can be automated more easily
            echo ""
            log "Verifying implementation..."

            # Run tests
            if make clean && make test > /tmp/ralph_test.log 2>&1; then
                success "Tests pass!"
                TEST_RESULT="PASS"
            else
                error "Tests failed!"
                TEST_RESULT="FAIL"
                cat /tmp/ralph_test.log
            fi

            # For now, just report
            echo ""
            if [ "$TEST_RESULT" = "PASS" ]; then
                success "CONTROLLER would ACCEPT implementation"
                echo ""
                echo "Next steps:"
                echo "  1. Update ROADMAP.md with verification notes"
                echo "  2. Mark task as complete"
                echo "  3. Select next task"
                echo "  4. Set Agent Turn: IMPLEMENTER"
                echo ""
                warning "Stopping here. Manual ROADMAP update needed."
                break
            else
                error "CONTROLLER would REJECT implementation"
                echo ""
                echo "Next steps:"
                echo "  1. Update ROADMAP.md with issues"
                echo "  2. Set Status: BLOCKED"
                echo "  3. Document problems in Blockers section"
                echo ""
                warning "Stopping here. Manual intervention needed."
                break
            fi
        fi

        # Sleep between iterations
        if [ $i -lt $MAX_ITERATIONS ]; then
            log "Sleeping ${SLEEP_BETWEEN}s before next iteration..."
            sleep $SLEEP_BETWEEN
        fi
    done

    echo ""
    log "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    success "Automated run complete after $i iterations"
    log "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
}

main "$@"
