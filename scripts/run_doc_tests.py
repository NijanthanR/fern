#!/usr/bin/env python3
"""Run doc tests extracted from `@doc` comment blocks in Fern source files."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

DOC_SINGLE_RE = re.compile(r'^\s*@doc\s+"""(.*)"""\s*$')
DOC_START_RE = re.compile(r'^\s*@doc\s+"""\s*$')
DOC_END_RE = re.compile(r'^\s*"""\s*$')
FENCE_START_RE = re.compile(r"^\s*```fern\s*$")
FENCE_END_RE = re.compile(r"^\s*```\s*$")


@dataclass
class DocSnippet:
    """Single extracted doc snippet."""

    source: Path
    line: int
    code: str


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments.

    Returns:
        Parsed argument namespace.
    """

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", help="Files or directories to scan.")
    parser.add_argument(
        "--path",
        dest="extra_paths",
        action="append",
        default=[],
        help="Additional file/directory path to scan (repeatable).",
    )
    parser.add_argument("--fern-bin", default="./bin/fern", help="Path to fern binary.")
    return parser.parse_args()


def normalize_paths(args: argparse.Namespace) -> list[Path]:
    """Resolve and validate scan paths.

    Args:
        args: Parsed CLI arguments.

    Returns:
        Normalized scan paths.
    """

    raw_paths = list(args.paths) + list(args.extra_paths)
    if not raw_paths:
        raw_paths = ["examples", "docs/doctests"]

    paths: list[Path] = []
    for raw in raw_paths:
        path = Path(raw)
        if not path.exists():
            print(f"doc test error: path does not exist: {path}", file=sys.stderr)
            raise SystemExit(1)
        paths.append(path)
    return paths


def collect_sources(paths: list[Path]) -> list[Path]:
    """Collect Fern source files from provided paths.

    Args:
        paths: Files or directories.

    Returns:
        Deduplicated sorted `.fn` files.
    """

    files: set[Path] = set()
    for path in paths:
        if path.is_file():
            if path.suffix == ".fn":
                files.add(path)
            continue
        if path.is_dir():
            for source in path.rglob("*.fn"):
                files.add(source)

    return sorted(files)


def extract_doc_blocks(lines: list[str]) -> list[tuple[int, str]]:
    """Extract `@doc` block text with source line numbers.

    Args:
        lines: File lines.

    Returns:
        List of `(line_number, doc_text)` tuples.
    """

    blocks: list[tuple[int, str]] = []
    i = 0
    while i < len(lines):
        line = lines[i]

        single_match = DOC_SINGLE_RE.match(line)
        if single_match:
            blocks.append((i + 1, single_match.group(1)))
            i += 1
            continue

        if not DOC_START_RE.match(line):
            i += 1
            continue

        start_line = i + 1
        i += 1
        buffer: list[str] = []
        while i < len(lines) and not DOC_END_RE.match(lines[i]):
            buffer.append(lines[i])
            i += 1

        if i < len(lines) and DOC_END_RE.match(lines[i]):
            blocks.append((start_line, "\n".join(buffer)))

        i += 1

    return blocks


def extract_snippets(source: Path) -> list[DocSnippet]:
    """Extract fenced `fern` snippets from `@doc` blocks.

    Args:
        source: Source file.

    Returns:
        Snippets with source location.
    """

    lines = source.read_text(encoding="utf-8").splitlines()
    snippets: list[DocSnippet] = []
    for start_line, doc_text in extract_doc_blocks(lines):
        doc_lines = doc_text.splitlines()
        collecting = False
        buffer: list[str] = []
        snippet_line = start_line

        for idx, line in enumerate(doc_lines):
            if not collecting and FENCE_START_RE.match(line):
                collecting = True
                buffer = []
                snippet_line = start_line + idx + 1
                continue
            if collecting and FENCE_END_RE.match(line):
                code = "\n".join(buffer).strip()
                if code:
                    snippets.append(DocSnippet(source=source, line=snippet_line, code=code))
                collecting = False
                buffer = []
                continue
            if collecting:
                buffer.append(line)

    return snippets


def should_keep_statement(line: str) -> bool:
    """Decide whether snippet line should be treated as a statement.

    Args:
        line: Normalized line.

    Returns:
        True if line can be emitted as a statement.
    """

    if line.endswith(":"):
        return True

    prefixes = (
        "let ",
        "return ",
        "if ",
        "match ",
        "with ",
        "for ",
        "break",
        "continue",
        "defer ",
    )
    return line.startswith(prefixes)


def strip_expectation(line: str) -> str:
    """Remove inline expected-output comments.

    Args:
        line: Snippet line.

    Returns:
        Line without trailing `# => ...` expectation.
    """

    marker = "# =>"
    if marker not in line:
        return line
    return line.split(marker, maxsplit=1)[0].rstrip()


def build_wrapper(snippet: str) -> str | None:
    """Wrap expression-style snippets in a minimal compilable Fern program.

    Args:
        snippet: Raw snippet code.

    Returns:
        Wrapped Fern source, or None if snippet has no executable lines.
    """

    lines = snippet.splitlines()
    body: list[str] = []
    value_index = 0

    for raw in lines:
        stripped = strip_expectation(raw).strip()
        if not stripped or stripped.startswith("#"):
            continue

        if should_keep_statement(stripped):
            body.append(f"    {stripped}")
        else:
            body.append(f"    let _doc_value_{value_index} = {stripped}")
            value_index += 1

    if not body:
        return None

    body.append("    0")
    return "module doc_tests\n\nfn main() -> Int:\n" + "\n".join(body) + "\n"


def run_check(fern_bin: str, source_text: str) -> tuple[int, str]:
    """Run `fern check` for a temporary source.

    Args:
        fern_bin: Path to fern executable.
        source_text: Fern source text to validate.

    Returns:
        Tuple of `(exit_code, combined_output)`.
    """

    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".fn",
        encoding="utf-8",
        delete=False,
    ) as temp:
        temp.write(source_text)
        temp_path = Path(temp.name)

    try:
        result = subprocess.run(
            [fern_bin, "check", str(temp_path)],
            capture_output=True,
            text=True,
            check=False,
        )
        output = f"{result.stdout}{result.stderr}"
        return result.returncode, output
    finally:
        temp_path.unlink(missing_ok=True)


def check_snippet(snippet: DocSnippet, fern_bin: str) -> tuple[bool, str]:
    """Validate a single snippet.

    Args:
        snippet: Extracted snippet metadata.
        fern_bin: Path to fern executable.

    Returns:
        Tuple `(passed, failure_output)`.
    """

    direct_code, direct_output = run_check(fern_bin, snippet.code)
    if direct_code == 0:
        return True, ""

    wrapped = build_wrapper(snippet.code)
    if wrapped is None:
        return True, ""

    wrapped_code, wrapped_output = run_check(fern_bin, wrapped)
    if wrapped_code == 0:
        return True, ""

    if wrapped_output.strip():
        return False, wrapped_output
    return False, direct_output


def main() -> int:
    """Entry point.

    Returns:
        Exit code.
    """

    args = parse_args()
    paths = normalize_paths(args)
    sources = collect_sources(paths)

    snippets: list[DocSnippet] = []
    for source in sources:
        snippets.extend(extract_snippets(source))

    if not snippets:
        print("doc tests: no @doc fern snippets found")
        return 0

    failures = 0
    for snippet in snippets:
        passed, output = check_snippet(snippet, args.fern_bin)
        if passed:
            continue
        failures += 1
        print(f"doc test failed: {snippet.source}:{snippet.line}", file=sys.stderr)
        print("----- snippet -----", file=sys.stderr)
        print(snippet.code, file=sys.stderr)
        if output.strip():
            print("----- compiler output -----", file=sys.stderr)
            print(output.rstrip(), file=sys.stderr)

    passed = len(snippets) - failures
    print(f"doc tests: {passed}/{len(snippets)} passed")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
