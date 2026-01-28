#!/usr/bin/env bash
# Ralph Loop - Single Iteration Runner
#
# This script runs one complete Ralph Loop iteration:
# 1. Check current agent turn in ROADMAP.md
# 2. Launch appropriate agent (IMPLEMENTER or CONTROLLER)
# 3. Wait for completion
# 4. Check status and continue or stop

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ROADMAP_FILE="$PROJECT_ROOT/ROADMAP.md"
PROMPTS_FILE="$SCRIPT_DIR/ralph-prompts.md"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[Ralph]${NC} $*"
}

error() {
    echo -e "${RED}[Error]${NC} $*" >&2
}

success() {
    echo -e "${GREEN}[Success]${NC} $*"
}

warning() {
    echo -e "${YELLOW}[Warning]${NC} $*"
}

# Extract Ralph Loop status from ROADMAP.md
get_status_field() {
    local field="$1"
    grep "^\*\*${field}\*\*:" "$ROADMAP_FILE" | head -1 | sed "s/^\*\*${field}\*\*: *//" || echo ""
}

# Check if Ralph Loop is initialized in ROADMAP.md
check_ralph_initialized() {
    if ! grep -q "## Ralph Loop Status" "$ROADMAP_FILE"; then
        error "Ralph Loop not initialized in ROADMAP.md"
        echo ""
        echo "Run: $SCRIPT_DIR/ralph-init.sh"
        exit 1
    fi
}

# Parse current state
get_current_state() {
    log "Reading ROADMAP.md status..."

    AGENT_TURN=$(get_status_field "Agent Turn")
    STATUS=$(get_status_field "Status")
    ITERATION=$(get_status_field "Current Iteration")
    MILESTONE=$(get_status_field "Current Milestone")

    log "Milestone: $MILESTONE"
    log "Iteration: $ITERATION"
    log "Agent Turn: $AGENT_TURN"
    log "Status: $STATUS"
}

# Launch IMPLEMENTER agent
launch_implementer() {
    log "${PURPLE}Launching IMPLEMENTER agent...${NC}"
    echo ""

    # Extract implementer prompt
    IMPLEMENTER_PROMPT=$(sed -n '/## IMPLEMENTER Agent Prompt/,/## CONTROLLER Agent Prompt/p' "$PROMPTS_FILE" | head -n -1)

    # Create a temporary file with the prompt + context
    TEMP_PROMPT=$(mktemp)
    cat > "$TEMP_PROMPT" << EOF
$IMPLEMENTER_PROMPT

---

# Current Context

You are working on the Fern compiler project.

Please read ROADMAP.md for your current task and follow the IMPLEMENTER workflow.

The current state is:
- Milestone: $MILESTONE
- Iteration: $ITERATION
- Status: $STATUS

Read the "Ralph Loop Status" section in ROADMAP.md and complete your task.

Remember:
1. Write tests FIRST (RED phase)
2. Implement feature (GREEN phase)
3. Update ROADMAP.md with status WAITING_VERIFICATION
4. Hand off to CONTROLLER
EOF

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  IMPLEMENTER Agent"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    cat "$TEMP_PROMPT"
    echo ""
    echo "Press ENTER to launch IMPLEMENTER agent, or Ctrl+C to cancel"
    read -r

    # In a real implementation, this would launch Claude Code with the prompt
    # For now, we'll just show what would happen
    echo ""
    echo "${YELLOW}[Simulation Mode]${NC}"
    echo "In production, this would run:"
    echo "  claude code --prompt-file $TEMP_PROMPT"
    echo ""
    echo "For now, you can manually:"
    echo "  1. Read ROADMAP.md 'Current Task'"
    echo "  2. Implement following TDD"
    echo "  3. Update ROADMAP.md with WAITING_VERIFICATION"
    echo ""

    rm "$TEMP_PROMPT"
}

# Launch CONTROLLER agent
launch_controller() {
    log "${PURPLE}Launching CONTROLLER agent...${NC}"
    echo ""

    # Extract controller prompt
    CONTROLLER_PROMPT=$(sed -n '/## CONTROLLER Agent Prompt/,/^```$/p' "$PROMPTS_FILE" | tail -n +2 | sed '$d')

    # Create a temporary file with the prompt + context
    TEMP_PROMPT=$(mktemp)
    cat > "$TEMP_PROMPT" << EOF
$CONTROLLER_PROMPT

---

# Current Context

You are verifying work on the Fern compiler project.

Please read ROADMAP.md for the implementation to verify.

The current state is:
- Milestone: $MILESTONE
- Iteration: $ITERATION
- Status: $STATUS

Read the "Implementation Notes" section in ROADMAP.md and verify the work.

Remember:
1. Run make clean && make test
2. Review code quality
3. Check success criteria
4. ACCEPT (select next task) or REJECT (document issues)
5. Update ROADMAP.md
EOF

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  CONTROLLER Agent"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    cat "$TEMP_PROMPT"
    echo ""
    echo "Press ENTER to launch CONTROLLER agent, or Ctrl+C to cancel"
    read -r

    # In a real implementation, this would launch Claude Code with the prompt
    # For now, we'll just show what would happen
    echo ""
    echo "${YELLOW}[Simulation Mode]${NC}"
    echo "In production, this would run:"
    echo "  claude code --prompt-file $TEMP_PROMPT"
    echo ""
    echo "For now, you can manually:"
    echo "  1. Run: make clean && make test"
    echo "  2. Review: git diff"
    echo "  3. Verify success criteria"
    echo "  4. Update ROADMAP.md (ACCEPT or REJECT)"
    echo ""

    rm "$TEMP_PROMPT"
}

# Main execution
main() {
    cd "$PROJECT_ROOT"

    echo ""
    echo "╔═══════════════════════════════════════════════════════════╗"
    echo "║          Ralph Loop - Iteration Runner                   ║"
    echo "╚═══════════════════════════════════════════════════════════╝"
    echo ""

    check_ralph_initialized
    get_current_state

    echo ""

    case "$STATUS" in
        "IN_PROGRESS")
            if [ "$AGENT_TURN" = "IMPLEMENTER" ]; then
                launch_implementer
            elif [ "$AGENT_TURN" = "CONTROLLER" ]; then
                error "Status is IN_PROGRESS but agent turn is CONTROLLER"
                error "This shouldn't happen. Check ROADMAP.md"
                exit 1
            else
                error "Unknown agent: $AGENT_TURN"
                exit 1
            fi
            ;;

        "WAITING_VERIFICATION")
            if [ "$AGENT_TURN" = "CONTROLLER" ]; then
                launch_controller
            else
                error "Status is WAITING_VERIFICATION but agent is not CONTROLLER"
                error "Check ROADMAP.md"
                exit 1
            fi
            ;;

        "BLOCKED")
            warning "Status is BLOCKED"
            echo ""
            echo "Current blockers:"
            sed -n '/### Blockers/,/^$/p' "$ROADMAP.md"
            echo ""
            echo "Agent turn: $AGENT_TURN"
            if [ "$AGENT_TURN" = "IMPLEMENTER" ]; then
                echo "IMPLEMENTER needs to fix blockers and update status to WAITING_VERIFICATION"
                launch_implementer
            else
                error "Unexpected agent $AGENT_TURN for BLOCKED status"
                exit 1
            fi
            ;;

        "MILESTONE_COMPLETE")
            success "Milestone $MILESTONE is complete!"
            echo ""
            echo "Please review and approve before starting next milestone."
            exit 0
            ;;

        *)
            error "Unknown status: $STATUS"
            exit 1
            ;;
    esac

    echo ""
    success "Iteration step complete"
    echo ""
    echo "Next: Check ROADMAP.md and run again for next step"
}

main "$@"
