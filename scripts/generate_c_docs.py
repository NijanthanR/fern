#!/usr/bin/env python3
"""Generate C API docs from Fern public headers."""

from __future__ import annotations

import argparse
import html
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

DOC_START_RE = re.compile(r"^\s*/\*\*")
BLOCK_COMMENT_START_RE = re.compile(r"^\s*/\*")
DOC_END_RE = re.compile(r"\*/")
MACRO_RE = re.compile(r"^\s*#")
LINE_COMMENT_RE = re.compile(r"^\s*//")
FUNC_POINTER_RE = re.compile(r"\(\s*\*\s*[A-Za-z_][A-Za-z0-9_]*\s*\)")
IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


@dataclass
class CFunctionDoc:
    """Documented C function declaration."""

    name: str
    signature: str
    comment: str
    line: int
    anchor: str
    has_doc: bool


@dataclass
class HeaderDoc:
    """Documentation extracted from one header file."""

    source: Path
    anchor: str
    functions: list[CFunctionDoc]


def slug(text: str) -> str:
    lowered = text.lower()
    lowered = re.sub(r"[^a-z0-9_]+", "-", lowered)
    lowered = re.sub(r"-{2,}", "-", lowered).strip("-")
    return lowered or "item"


def normalize_comment_text(raw_lines: list[str]) -> str:
    """Normalize a C doc comment block into plain markdown-like text."""
    cleaned: list[str] = []
    for line in raw_lines:
        text = line.strip()
        text = re.sub(r"^/\*\*?", "", text)
        text = re.sub(r"\*/$", "", text)
        text = re.sub(r"^\*\s?", "", text)
        cleaned.append(text.rstrip())

    while cleaned and not cleaned[0].strip():
        cleaned.pop(0)
    while cleaned and not cleaned[-1].strip():
        cleaned.pop()
    return "\n".join(cleaned).strip()


def extract_doc_comment(lines: list[str], start_idx: int) -> tuple[str, int]:
    """Extract doc comment at start_idx (must point to '/**')."""
    cursor = start_idx
    buffer: list[str] = []
    while cursor < len(lines):
        buffer.append(lines[cursor].rstrip("\n"))
        if DOC_END_RE.search(lines[cursor]):
            break
        cursor += 1
    return normalize_comment_text(buffer), cursor


def is_section_comment(comment: str) -> bool:
    """Heuristic: section comments apply to many subsequent declarations."""
    lowered = comment.lower()
    if "@param" in lowered or "@return" in lowered:
        return False
    non_empty_lines = [line.strip() for line in comment.splitlines() if line.strip()]
    if not non_empty_lines:
        return False
    if len(non_empty_lines) <= 2:
        return True
    if len(non_empty_lines) == 3 and non_empty_lines[0].endswith(":"):
        return True
    return False


def normalize_signature_text(raw: str) -> str:
    """Normalize declaration whitespace for readable output."""
    text = raw.strip()
    text = re.sub(r"\s+", " ", text)
    text = re.sub(r"\s*,\s*", ", ", text)
    text = re.sub(r"\s*\(\s*", "(", text)
    text = re.sub(r"\s*\)\s*", ")", text)
    text = re.sub(r"\)\s*;", ");", text)
    return text


def looks_like_function_declaration(text: str) -> bool:
    """Filter declaration-like chunks down to function prototypes."""
    if not text.endswith(";"):
        return False
    if "(" not in text or ")" not in text:
        return False
    if "typedef" in text:
        return False
    if FUNC_POINTER_RE.search(text):
        return False
    if "=" in text:
        return False
    if "{" in text or "}" in text:
        return False
    return True


def extract_function_name(signature: str) -> str | None:
    """Extract function identifier from a prototype declaration."""
    prefix = signature.split("(", maxsplit=1)[0].strip()
    if not prefix:
        return None
    token = prefix.split()[-1].strip("*")
    if not IDENT_RE.match(token):
        return None
    if token.upper() == token:
        return None
    if token in {"if", "for", "while", "switch", "return", "sizeof"}:
        return None
    return token


def parse_header(path: Path, repo_root: Path) -> HeaderDoc:
    """Parse one header and collect function docs."""
    lines = path.read_text(encoding="utf-8").splitlines()
    pending_doc = ""
    pending_is_section = False
    active_section_doc = ""
    functions: list[CFunctionDoc] = []
    in_macro = False

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if in_macro:
            in_macro = stripped.endswith("\\")
            i += 1
            continue

        if MACRO_RE.match(line):
            in_macro = stripped.endswith("\\")
            i += 1
            continue

        if DOC_START_RE.match(line) or BLOCK_COMMENT_START_RE.match(line):
            pending_doc, i = extract_doc_comment(lines, i)
            pending_is_section = is_section_comment(pending_doc)
            if pending_is_section:
                active_section_doc = pending_doc
            i += 1
            continue

        if LINE_COMMENT_RE.match(line):
            pending_doc = stripped[2:].strip()
            pending_is_section = is_section_comment(pending_doc)
            if pending_is_section:
                active_section_doc = pending_doc
            i += 1
            continue

        if not stripped:
            i += 1
            continue

        if "(" not in line:
            pending_doc = ""
            pending_is_section = False
            active_section_doc = ""
            i += 1
            continue

        start_line = i + 1
        chunk = [line.rstrip("\n")]
        while i + 1 < len(lines) and ";" not in chunk[-1] and "{" not in chunk[-1]:
            i += 1
            if MACRO_RE.match(lines[i]):
                break
            chunk.append(lines[i].rstrip("\n"))

        declaration = normalize_signature_text(" ".join(chunk))
        if not looks_like_function_declaration(declaration):
            pending_doc = ""
            pending_is_section = False
            i += 1
            continue

        name = extract_function_name(declaration)
        if not name:
            pending_doc = ""
            pending_is_section = False
            i += 1
            continue

        rel = path.relative_to(repo_root) if path.is_relative_to(repo_root) else path
        anchor = f"{slug(rel.as_posix())}-{slug(name)}"
        comment = pending_doc if pending_doc else active_section_doc
        functions.append(
            CFunctionDoc(
                name=name,
                signature=declaration,
                comment=comment,
                line=start_line,
                anchor=anchor,
                has_doc=bool(comment),
            )
        )

        if pending_doc and not pending_is_section:
            active_section_doc = ""
        pending_doc = ""
        pending_is_section = False
        i += 1

    rel_source = path.relative_to(repo_root) if path.is_relative_to(repo_root) else path
    return HeaderDoc(
        source=rel_source,
        anchor=slug(rel_source.as_posix()),
        functions=functions,
    )


def resolve_inputs(paths: list[str]) -> list[Path]:
    """Resolve files/directories into deterministic header list."""
    files: set[Path] = set()
    for raw in paths:
        path = Path(raw)
        if not path.exists():
            continue
        if path.is_file() and path.suffix == ".h":
            files.add(path)
            continue
        if path.is_dir():
            for candidate in path.rglob("*.h"):
                files.add(candidate)
    return sorted(files)


def render_comment_html(comment: str) -> str:
    if not comment:
        return "<p><em>No description provided.</em></p>"
    chunks = [chunk.strip() for chunk in re.split(r"\n\s*\n", comment) if chunk.strip()]
    if not chunks:
        return "<p><em>No description provided.</em></p>"
    return "\n".join(f"<p>{html.escape(chunk)}</p>" for chunk in chunks)


def render_html(headers: list[HeaderDoc], scanned_paths: list[str]) -> str:
    """Render extracted docs as HTML."""
    lines: list[str] = [
        "<!doctype html>",
        "<html lang=\"en\">",
        "<head>",
        "<meta charset=\"utf-8\">",
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">",
        "<title>Fern C API Documentation</title>",
        "<style>",
        "body { font-family: ui-sans-serif, -apple-system, sans-serif; margin: 2rem auto; max-width: 1000px; line-height: 1.55; padding: 0 1rem; }",
        "h1, h2, h3 { line-height: 1.2; }",
        "code { background: #f4f4f4; padding: 0.1rem 0.3rem; border-radius: 4px; }",
        "pre { background: #111827; color: #e5e7eb; padding: 0.75rem; border-radius: 8px; overflow-x: auto; }",
        "a { color: #0f5da8; text-decoration: none; }",
        "a:hover { text-decoration: underline; }",
        "</style>",
        "</head>",
        "<body>",
        "<h1>Fern C API Documentation</h1>",
        f"<p>Scanned paths: <code>{html.escape(', '.join(scanned_paths))}</code></p>",
        f"<p>Headers documented: {len(headers)}</p>",
        "<h2>Headers</h2>",
        "<ul>",
    ]

    for header in headers:
        lines.append(
            f"<li><a href=\"#{header.anchor}\"><code>{html.escape(str(header.source))}</code></a></li>"
        )
    lines.append("</ul>")

    for header in headers:
        lines.append(f"<section id=\"{header.anchor}\">")
        lines.append(f"<h2><code>{html.escape(str(header.source))}</code></h2>")
        lines.append(f"<p>Public declarations: {len(header.functions)}</p>")
        if not header.functions:
            lines.append("<p><em>No function declarations found.</em></p>")
            lines.append("</section>")
            continue

        for fn in header.functions:
            lines.append(f"<article id=\"{fn.anchor}\">")
            lines.append(f"<h3><code>{html.escape(fn.name)}</code></h3>")
            lines.append(f"<p>Line: {fn.line}</p>")
            lines.append(
                f"<pre><code class=\"language-c\">{html.escape(fn.signature)}</code></pre>"
            )
            lines.append(render_comment_html(fn.comment))
            lines.append("</article>")

        lines.append("</section>")

    lines.append("</body>")
    lines.append("</html>")
    return "\n".join(lines) + "\n"


def validate_docs(headers: list[HeaderDoc]) -> list[str]:
    """Return validation errors for missing public C API docs."""
    errors: list[str] = []
    for header in headers:
        for fn in header.functions:
            if fn.has_doc:
                continue
            errors.append(f"{header.source}:{fn.line}: missing doc comment for `{fn.name}`")
    return errors


def open_output(path: Path) -> bool:
    opener = None
    if sys.platform == "darwin":
        opener = shutil.which("open")
    elif sys.platform.startswith("linux"):
        opener = shutil.which("xdg-open")

    if not opener:
        print(f"note: no opener available for {path}", file=sys.stderr)
        return False

    try:
        subprocess.run([opener, str(path)], check=False)
    except OSError as exc:
        print(f"note: failed to open docs: {exc}", file=sys.stderr)
        return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--path",
        dest="paths",
        action="append",
        default=[],
        help="Header file or directory to scan (repeatable).",
    )
    parser.add_argument(
        "--output",
        default="docs/generated/c-api.html",
        help="Output HTML file path.",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Fail if public declarations are missing doc comments.",
    )
    parser.add_argument(
        "--open",
        action="store_true",
        dest="open_docs",
        help="Open generated documentation after writing.",
    )
    args = parser.parse_args()

    scan_paths = args.paths or ["include", "runtime/fern_runtime.h", "runtime/fern_gc.h"]
    header_files = resolve_inputs(scan_paths)
    if not header_files:
        print("error: no C header files found for documentation", file=sys.stderr)
        return 1

    repo_root = Path.cwd()
    headers = [parse_header(path, repo_root) for path in header_files]
    errors = validate_docs(headers)
    if args.check and errors:
        for message in errors:
            print(f"error: {message}", file=sys.stderr)
        print(f"error: {len(errors)} undocumented public declarations", file=sys.stderr)
        return 1

    body = render_html(headers, scan_paths)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(body, encoding="utf-8")
    print(f"Generated docs: {output}")

    if args.open_docs:
        open_output(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
