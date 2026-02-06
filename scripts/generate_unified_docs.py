#!/usr/bin/env python3
"""Generate unified Fern + C API documentation site."""

from __future__ import annotations

import argparse
import html
import re
import shutil
import subprocess
import sys
from pathlib import Path

import generate_c_docs
import generate_docs

HREF_RE = re.compile(r'href="([^"]+)"')
ID_RE = re.compile(r'id="([^"]+)"')


def collect_fern_modules(paths: list[str], repo_root: Path) -> list[generate_docs.ModuleDoc]:
    """Collect Fern module docs across multiple source roots."""
    sources: set[Path] = set()
    for root in paths:
        for source in generate_docs.collect_sources(root):
            sources.add(source)
    return [generate_docs.collect_module_docs(source, repo_root) for source in sorted(sources)]


def build_index_html(
    fern_paths: list[str],
    c_paths: list[str],
    fern_modules: int,
    c_headers: int,
    c_symbols: int,
) -> str:
    """Render docs landing page."""
    fern_paths_text = ", ".join(fern_paths)
    c_paths_text = ", ".join(c_paths)
    lines = [
        "<!doctype html>",
        "<html lang=\"en\">",
        "<head>",
        "<meta charset=\"utf-8\">",
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">",
        "<title>Fern Documentation</title>",
        "<style>",
        "body { font-family: ui-sans-serif, -apple-system, sans-serif; margin: 2rem auto; max-width: 900px; line-height: 1.55; padding: 0 1rem; }",
        "h1, h2 { line-height: 1.2; }",
        "a { color: #0f5da8; text-decoration: none; }",
        "a:hover { text-decoration: underline; }",
        "code { background: #f4f4f4; padding: 0.1rem 0.3rem; border-radius: 4px; }",
        ".card { border: 1px solid #e5e7eb; border-radius: 12px; padding: 1rem; margin: 1rem 0; }",
        "</style>",
        "</head>",
        "<body>",
        "<h1>Fern Documentation</h1>",
        "<p>Unified generated docs for Fern language modules and C API surface.</p>",
        "<div class=\"card\">",
        "<h2><a href=\"fern-docs.html\">Fern Modules</a></h2>",
        f"<p>Modules: {fern_modules}</p>",
        f"<p>Source roots: <code>{html.escape(fern_paths_text)}</code></p>",
        "</div>",
        "<div class=\"card\">",
        "<h2><a href=\"c-api.html\">C API</a></h2>",
        f"<p>Headers: {c_headers}</p>",
        f"<p>Public declarations: {c_symbols}</p>",
        f"<p>Source roots: <code>{html.escape(c_paths_text)}</code></p>",
        "</div>",
        "</body>",
        "</html>",
    ]
    return "\n".join(lines) + "\n"


def collect_ids(path: Path) -> set[str]:
    """Collect HTML id attributes for anchor validation."""
    text = path.read_text(encoding="utf-8")
    return set(ID_RE.findall(text))


def validate_site_links(output_dir: Path, html_files: list[Path]) -> list[str]:
    """Validate local file links and same-file anchors."""
    errors: list[str] = []
    id_map = {path.name: collect_ids(path) for path in html_files}

    for path in html_files:
        text = path.read_text(encoding="utf-8")
        for href in HREF_RE.findall(text):
            if href.startswith(("http://", "https://", "mailto:")):
                continue
            if href.startswith("#"):
                anchor = href[1:]
                if anchor and anchor not in id_map[path.name]:
                    errors.append(f"{path.name}: missing local anchor #{anchor}")
                continue

            target, _, anchor = href.partition("#")
            target_path = output_dir / target
            if not target_path.exists():
                errors.append(f"{path.name}: missing link target {target}")
                continue
            if anchor:
                target_ids = id_map.get(target_path.name, set())
                if anchor not in target_ids:
                    errors.append(f"{path.name}: missing anchor #{anchor} in {target}")

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
        "--output-dir",
        default="docs/generated",
        help="Output directory for generated site.",
    )
    parser.add_argument(
        "--fern-path",
        dest="fern_paths",
        action="append",
        default=[],
        help="Fern file/directory to document (repeatable).",
    )
    parser.add_argument(
        "--c-path",
        dest="c_paths",
        action="append",
        default=[],
        help="C header file/directory to document (repeatable).",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate generated site links and missing public C API docs.",
    )
    parser.add_argument(
        "--open",
        action="store_true",
        dest="open_docs",
        help="Open generated docs index after writing.",
    )
    args = parser.parse_args()

    repo_root = Path.cwd()
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    fern_paths = args.fern_paths or ["examples", "docs/doctests"]
    c_paths = args.c_paths or ["include", "runtime/fern_runtime.h", "runtime/fern_gc.h"]

    fern_modules = collect_fern_modules(fern_paths, repo_root)
    if not fern_modules:
        print("error: no Fern modules found for documentation", file=sys.stderr)
        return 1

    c_header_files = generate_c_docs.resolve_inputs(c_paths)
    if not c_header_files:
        print("error: no C headers found for documentation", file=sys.stderr)
        return 1
    c_headers = [generate_c_docs.parse_header(path, repo_root) for path in c_header_files]

    if args.check:
        c_errors = generate_c_docs.validate_docs(c_headers)
        if c_errors:
            for message in c_errors:
                print(f"error: {message}", file=sys.stderr)
            print(f"error: {len(c_errors)} undocumented public C declarations", file=sys.stderr)
            return 1

    fern_html = generate_docs.render_html(fern_modules, ", ".join(fern_paths))
    c_html = generate_c_docs.render_html(c_headers, c_paths)
    c_symbols = sum(len(header.functions) for header in c_headers)
    index_html = build_index_html(fern_paths, c_paths, len(fern_modules), len(c_headers), c_symbols)

    fern_path = output_dir / "fern-docs.html"
    c_path = output_dir / "c-api.html"
    index_path = output_dir / "index.html"

    fern_path.write_text(fern_html, encoding="utf-8")
    c_path.write_text(c_html, encoding="utf-8")
    index_path.write_text(index_html, encoding="utf-8")
    print(f"Generated docs: {index_path}")

    if args.check:
        link_errors = validate_site_links(output_dir, [index_path, fern_path, c_path])
        if link_errors:
            for message in link_errors:
                print(f"error: {message}", file=sys.stderr)
            print(f"error: {len(link_errors)} broken docs links/anchors", file=sys.stderr)
            return 1

    if args.open_docs:
        open_output(index_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
