#!/usr/bin/env bash
# Ralph Loop - Initialization Script
#
# This script initializes the Ralph Loop in ROADMAP.md by adding
# the Ralph Loop Status section at the top.

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
    echo -e "${BLUE}[Ralph Init]${NC} $*"
}

success() {
    echo -e "${GREEN}[Success]${NC} $*"
}

warning() {
    echo -e "${YELLOW}[Warning]${NC} $*"
}

# Check if already initialized
if grep -q "## Ralph Loop Status" "$ROADMAP_FILE"; then
    warning "Ralph Loop already initialized in ROADMAP.md"
    echo ""
    echo "Current status:"
    sed -n '/## Ralph Loop Status/,/^---$/p' "$ROADMAP_FILE" | head -n -1
    exit 0
fi

log "Initializing Ralph Loop in ROADMAP.md..."

# Create the Ralph Loop Status section
RALPH_STATUS=$(cat << 'EOF'
## Ralph Loop Status

**Current Milestone**: 2 - Parser
**Current Iteration**: 1
**Agent Turn**: CONTROLLER
**Status**: IN_PROGRESS
**Started**: $(date +"%Y-%m-%d %H:%M:%S")
**Last Updated**: $(date +"%Y-%m-%d %H:%M:%S")

### Current Task

- [ ] Select first task for Ralph Loop

**Expected Tests**: TBD
**Expected Files**: TBD
**Success Criteria**: TBD

### Implementation Notes

(IMPLEMENTER writes notes here after completing work)

### Verification Notes

(CONTROLLER writes notes here after verification)

### Blockers

None

---

EOF
)

# Evaluate the date commands
RALPH_STATUS=$(eval "cat << 'EVALEOF'
$RALPH_STATUS
EVALEOF
")

# Create a temporary file with the new content
TEMP_FILE=$(mktemp)

# Read the original ROADMAP.md and insert Ralph Loop Status after the title
{
    # Copy everything up to and including the first header
    sed -n '1,/^# Fern Implementation Roadmap/p' "$ROADMAP_FILE"

    # Add a blank line
    echo ""

    # Add Ralph Loop Status
    echo "$RALPH_STATUS"

    # Copy the rest of the file
    sed -n '/^# Fern Implementation Roadmap/,$p' "$ROADMAP_FILE" | tail -n +2
} > "$TEMP_FILE"

# Backup original
cp "$ROADMAP_FILE" "$ROADMAP_FILE.backup"
log "Created backup: $ROADMAP_FILE.backup"

# Replace with new version
mv "$TEMP_FILE" "$ROADMAP_FILE"

success "Ralph Loop initialized in ROADMAP.md"
echo ""
echo "Next steps:"
echo "  1. Review ROADMAP.md to see the Ralph Loop Status section"
echo "  2. CONTROLLER should select the first task"
echo "  3. Run: $SCRIPT_DIR/ralph-iteration.sh"
echo ""
echo "Or manually update ROADMAP.md to set:"
echo "  - Agent Turn: IMPLEMENTER"
echo "  - Current Task: <specific task from milestone>"
echo "  - Then run ralph-iteration.sh"
