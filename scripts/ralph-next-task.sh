#!/usr/bin/env bash
# Ralph Loop - Next Task Selector
#
# Helper script for CONTROLLER to select the next task from current milestone

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ROADMAP_FILE="$PROJECT_ROOT/ROADMAP.md"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() {
    echo -e "${BLUE}[Next Task]${NC} $*"
}

# Get current milestone from Ralph Loop Status
CURRENT_MILESTONE=$(grep "^\*\*Current Milestone\*\*:" "$ROADMAP_FILE" | head -1 | sed 's/.*: *//')

log "Current Milestone: $CURRENT_MILESTONE"
echo ""

# Extract the milestone section
MILESTONE_SECTION=$(sed -n "/## Milestone $CURRENT_MILESTONE:/,/^## Milestone/p" "$ROADMAP_FILE" | head -n -1)

# Find unchecked tasks
echo "Available tasks in Milestone $CURRENT_MILESTONE:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

TASK_NUM=1
while IFS= read -r line; do
    if [[ "$line" =~ ^[[:space:]]*-[[:space:]]\[[[:space:]]\] ]]; then
        # Extract task description
        TASK_DESC=$(echo "$line" | sed 's/^[[:space:]]*- \[ \] *//')
        echo "${GREEN}${TASK_NUM}.${NC} $TASK_DESC"
        TASK_NUM=$((TASK_NUM + 1))
    fi
done << EOF
$MILESTONE_SECTION
EOF

echo ""

if [ "$TASK_NUM" -eq 1 ]; then
    echo "${GREEN}✓${NC} All tasks in Milestone $CURRENT_MILESTONE are complete!"
    echo ""
    echo "Consider:"
    echo "  1. Marking milestone as COMPLETE in ROADMAP.md"
    echo "  2. Setting status to MILESTONE_COMPLETE"
    echo "  3. Moving to next milestone after review"
else
    echo "Select a task number (1-$((TASK_NUM-1))) or press Ctrl+C to cancel"
fi
