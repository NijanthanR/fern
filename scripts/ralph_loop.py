#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# ///
"""
Ralph Loop - Autonomous development system for the Fern compiler.

Runs CONTROLLER (Sonnet) and IMPLEMENTER (Opus) agents in a loop
via the Claude Code CLI. Each iteration:
  1. CONTROLLER verifies previous work and selects the next task
  2. IMPLEMENTER writes tests (RED) then implements (GREEN)

Usage:
    uv run scripts/ralph_loop.py [iterations]
    uv run scripts/ralph_loop.py           # default: 10 iterations
    uv run scripts/ralph_loop.py 5         # 5 iterations
"""

import json
import subprocess
import sys
import datetime
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent.parent

CONTROLLER_PROMPT = """\
You are the CONTROLLER agent in the Ralph Loop.

Read ROADMAP.md to see the current iteration status.

Your tasks:
1. Run `make test` to verify the most recent implementation
2. Review the code changes from the last iteration
3. Update ROADMAP.md with verification notes (accepted/rejected, details)
4. Select the next unfinished task from the current Milestone in ROADMAP.md
5. Update ROADMAP.md to start the next iteration with the new task
6. Commit your ROADMAP.md changes

Be concise and focused. Only modify ROADMAP.md.
"""

IMPLEMENTER_PROMPT = """\
You are the IMPLEMENTER agent in the Ralph Loop.

Read ROADMAP.md to find your assigned task under the current iteration.
Read DESIGN.md for the language specification.
Read CLAUDE.md for safety rules and patterns.

Your tasks:
1. Write tests FIRST (RED phase) based on DESIGN.md spec
2. Run tests - they should FAIL
3. Commit tests: git commit -m "test(<scope>): <description> (TDD - RED phase)"
4. Implement the feature (GREEN phase) - make tests pass
5. Run `make test` - all tests should PASS (no regressions)
6. Update ROADMAP.md with implementation notes
7. Commit implementation: git commit -m "feat(<scope>): <description>"

Follow TDD strictly. Use arena allocation. Keep code clean.
"""

# ANSI colors
DIM = "\033[2m"
BOLD = "\033[1m"
CYAN = "\033[36m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
RED = "\033[31m"
RESET = "\033[0m"
CLEAR_LINE = "\033[2K\r"


def format_tool_info(name: str, input_data: dict) -> str:
    """Format a tool call into a condensed status line."""
    if name == "Read":
        path = input_data.get("file_path", "")
        short = Path(path).name if path else "?"
        return f"Reading {short}"
    elif name == "Write":
        path = input_data.get("file_path", "")
        short = Path(path).name if path else "?"
        return f"Writing {short}"
    elif name == "Edit":
        path = input_data.get("file_path", "")
        short = Path(path).name if path else "?"
        return f"Editing {short}"
    elif name == "Bash":
        cmd = input_data.get("command", "")
        if len(cmd) > 60:
            cmd = cmd[:57] + "..."
        return f"$ {cmd}"
    elif name == "Glob":
        return f"Glob {input_data.get('pattern', '')}"
    elif name == "Grep":
        return f"Grep '{input_data.get('pattern', '')}'"
    elif name == "Task":
        return f"Agent: {input_data.get('description', '?')}"
    else:
        return name


def run_claude(model: str, prompt: str, label: str) -> bool:
    """Run claude with stream-json output, showing condensed progress."""
    cmd = [
        "claude",
        "--print",
        "--model", model,
        "--output-format", "stream-json",
        "--verbose",
        "--dangerously-skip-permissions",
        prompt,
    ]

    now = datetime.datetime.now().strftime("%H:%M:%S")
    print(f"\n{BOLD}{CYAN}--- {label} ({model}) started at {now} ---{RESET}\n")

    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=PROJECT_DIR,
        )

        tool_count = 0
        final_text = []

        for line in proc.stdout:
            line = line.strip()
            if not line:
                continue
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue

            etype = event.get("type")

            if etype == "assistant":
                msg = event.get("message", {})
                for block in msg.get("content", []):
                    btype = block.get("type")
                    if btype == "tool_use":
                        tool_count += 1
                        info = format_tool_info(block["name"], block.get("input", {}))
                        print(f"  {DIM}[{tool_count}]{RESET} {YELLOW}{info}{RESET}")
                    elif btype == "text":
                        text = block.get("text", "").strip()
                        if text:
                            final_text.append(text)

            elif etype == "result":
                cost = event.get("total_cost_usd", 0)
                turns = event.get("num_turns", 0)
                duration = event.get("duration_ms", 0) / 1000
                result_text = event.get("result", "")
                if result_text:
                    final_text.append(result_text)

                print()
                print(f"{BOLD}Summary:{RESET}")
                # Show the final result text (deduplicated)
                if final_text:
                    # The result field usually contains the full text; prefer it
                    summary = result_text if result_text else "\n".join(final_text)
                    # Trim to reasonable length for display
                    lines = summary.split("\n")
                    if len(lines) > 30:
                        for l in lines[:30]:
                            print(f"  {l}")
                        print(f"  {DIM}... ({len(lines) - 30} more lines){RESET}")
                    else:
                        for l in lines:
                            print(f"  {l}")

                print()
                print(
                    f"{DIM}{label} finished: {turns} turns, "
                    f"{tool_count} tool calls, {duration:.1f}s, "
                    f"${cost:.4f}{RESET}"
                )

        proc.wait()
        success = proc.returncode == 0

        if not success:
            stderr = proc.stderr.read()
            if stderr:
                print(f"{RED}stderr: {stderr.strip()}{RESET}")

        return success

    except FileNotFoundError:
        print(f"{RED}ERROR: 'claude' CLI not found. Install it first.{RESET}", file=sys.stderr)
        return False
    except Exception as e:
        print(f"{RED}ERROR running {label}: {e}{RESET}", file=sys.stderr)
        return False


def run_tests() -> tuple[int, int]:
    """Run make test and return (total, passed)."""
    try:
        result = subprocess.run(
            ["make", "test"],
            capture_output=True,
            text=True,
            cwd=PROJECT_DIR,
        )
        output = result.stdout + result.stderr

        for line in output.split("\n"):
            if "Total:" in line and "Passed:" in line:
                parts = line.split()
                try:
                    total = int(parts[parts.index("Total:") + 1])
                    passed = int(parts[parts.index("Passed:") + 1])
                    return (total, passed)
                except (ValueError, IndexError):
                    pass
        return (0, 0)
    except Exception as e:
        print(f"Warning: Could not run tests: {e}", file=sys.stderr)
        return (0, 0)


def main():
    iterations = int(sys.argv[1]) if len(sys.argv) > 1 else 10

    print(f"{BOLD}{'=' * 64}")
    print(f"  RALPH LOOP - AUTONOMOUS DEVELOPMENT")
    print(f"  Project: {PROJECT_DIR}")
    print(f"  Iterations: {iterations}")
    print(f"  Started: {datetime.datetime.now():%Y-%m-%d %H:%M:%S}")
    print(f"{'=' * 64}{RESET}")
    print()

    # Show initial test status
    total, passed = run_tests()
    color = GREEN if total == passed else RED
    print(f"Initial test status: {color}{passed}/{total} passing{RESET}")
    print()

    for iteration in range(1, iterations + 1):
        print(f"{BOLD}{'=' * 64}")
        print(f"  ITERATION {iteration}/{iterations}")
        print(f"{'=' * 64}{RESET}")

        # Step 1: CONTROLLER
        if not run_claude("sonnet", CONTROLLER_PROMPT, f"CONTROLLER (iter {iteration})"):
            print(f"{RED}CONTROLLER failed at iteration {iteration}. Stopping.{RESET}")
            sys.exit(1)

        # Step 2: IMPLEMENTER
        if not run_claude("opus", IMPLEMENTER_PROMPT, f"IMPLEMENTER (iter {iteration})"):
            print(f"{RED}IMPLEMENTER failed at iteration {iteration}. Stopping.{RESET}")
            sys.exit(1)

        # Show status after iteration
        total, passed = run_tests()
        color = GREEN if total == passed else RED
        print(f"\n{BOLD}After iteration {iteration}: {color}{passed}/{total} tests passing{RESET}")
        print()

    print()
    print(f"{BOLD}{'=' * 64}")
    print(f"  RALPH LOOP COMPLETE - {iterations} ITERATIONS")
    print(f"{'=' * 64}{RESET}")
    print()

    total, passed = run_tests()
    color = GREEN if total == passed else RED
    print(f"Final: {color}{passed}/{total} tests passing{RESET}")


if __name__ == "__main__":
    main()
