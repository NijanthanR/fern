#!/usr/bin/env python3
"""
Generate editor support files from Fern compiler token definitions.

This script reads the token definitions from include/token.h and generates:
1. Tree-sitter grammar (grammar.js)
2. Syntax highlighting files (highlights.scm)

The generated Tree-sitter grammar is then compiled to WASM for use in editors.

Usage:
    python scripts/generate_editor_support.py

Or via make:
    make editor-support
"""

import re
import sys
from pathlib import Path

# Project root (parent of scripts/)
PROJECT_ROOT = Path(__file__).parent.parent

# Source files
TOKEN_HEADER = PROJECT_ROOT / "include" / "token.h"

# Output files
TREE_SITTER_DIR = PROJECT_ROOT / "editor" / "tree-sitter-fern"
GRAMMAR_JS = TREE_SITTER_DIR / "grammar.js"
ZED_HIGHLIGHTS = (
    PROJECT_ROOT / "editor" / "zed-fern" / "languages" / "fern" / "highlights.scm"
)


def parse_tokens(header_path: Path) -> dict:
    """
    Parse token definitions from token.h.

    Returns a dict with categories:
    - keywords: list of keyword strings
    - operators: list of operator strings
    - literals: list of literal token names
    - delimiters: list of delimiter strings
    """
    content = header_path.read_text()

    tokens = {
        "keywords": [],
        "control_flow": [],  # if, else, match, for, return
        "storage": [],  # let, fn, type, newtype, trait, impl
        "visibility": [],  # pub
        "operators": [],
        "delimiters": [],
        "brackets": [],
        "builtin_types": [
            "Int",
            "Float",
            "String",
            "Bool",
            "List",
            "Map",
            "Result",
            "Option",
        ],
        "builtin_constants": ["true", "false"],
        "constructors": ["Ok", "Err", "Some", "None"],
    }

    # Map TOKEN_* to actual keyword/operator strings
    token_to_string = {
        # Keywords
        "TOKEN_LET": "let",
        "TOKEN_FN": "fn",
        "TOKEN_RETURN": "return",
        "TOKEN_IF": "if",
        "TOKEN_ELSE": "else",
        "TOKEN_MATCH": "match",
        "TOKEN_WITH": "with",
        "TOKEN_DO": "do",
        "TOKEN_DEFER": "defer",
        "TOKEN_PUB": "pub",
        "TOKEN_IMPORT": "import",
        "TOKEN_TYPE": "type",
        "TOKEN_TRAIT": "trait",
        "TOKEN_IMPL": "impl",
        "TOKEN_AND": "and",
        "TOKEN_OR": "or",
        "TOKEN_NOT": "not",
        "TOKEN_AS": "as",
        "TOKEN_MODULE": "module",
        "TOKEN_FOR": "for",
        "TOKEN_BREAK": "break",
        "TOKEN_CONTINUE": "continue",
        "TOKEN_IN": "in",
        "TOKEN_DERIVE": "derive",
        "TOKEN_WHERE": "where",
        "TOKEN_NEWTYPE": "newtype",
        "TOKEN_SPAWN": "spawn",
        "TOKEN_SEND": "send",
        "TOKEN_RECEIVE": "receive",
        "TOKEN_AFTER": "after",
        # Operators
        "TOKEN_PLUS": "+",
        "TOKEN_MINUS": "-",
        "TOKEN_STAR": "*",
        "TOKEN_SLASH": "/",
        "TOKEN_PERCENT": "%",
        "TOKEN_POWER": "**",
        "TOKEN_EQ": "==",
        "TOKEN_NE": "!=",
        "TOKEN_LT": "<",
        "TOKEN_LE": "<=",
        "TOKEN_GT": ">",
        "TOKEN_GE": ">=",
        "TOKEN_ASSIGN": "=",
        "TOKEN_BIND": "<-",
        "TOKEN_PIPE": "|>",
        "TOKEN_BAR": "|",
        "TOKEN_ARROW": "->",
        "TOKEN_FAT_ARROW": "=>",
        "TOKEN_QUESTION": "?",
        # Delimiters
        "TOKEN_COMMA": ",",
        "TOKEN_COLON": ":",
        "TOKEN_DOT": ".",
        "TOKEN_DOTDOT": "..",
        "TOKEN_DOTDOTEQ": "..=",
        "TOKEN_ELLIPSIS": "...",
        "TOKEN_UNDERSCORE": "_",
        "TOKEN_AT": "@",
        # Brackets
        "TOKEN_LPAREN": "(",
        "TOKEN_RPAREN": ")",
        "TOKEN_LBRACKET": "[",
        "TOKEN_RBRACKET": "]",
        "TOKEN_LBRACE": "{",
        "TOKEN_RBRACE": "}",
    }

    # Categorize keywords
    control_flow_tokens = {
        "TOKEN_IF",
        "TOKEN_ELSE",
        "TOKEN_MATCH",
        "TOKEN_FOR",
        "TOKEN_RETURN",
        "TOKEN_BREAK",
        "TOKEN_CONTINUE",
        "TOKEN_WITH",
    }
    storage_tokens = {
        "TOKEN_LET",
        "TOKEN_FN",
        "TOKEN_TYPE",
        "TOKEN_NEWTYPE",
        "TOKEN_TRAIT",
        "TOKEN_IMPL",
    }
    visibility_tokens = {"TOKEN_PUB"}

    # All keyword tokens (for categorization)
    keyword_tokens = {
        "TOKEN_LET",
        "TOKEN_FN",
        "TOKEN_RETURN",
        "TOKEN_IF",
        "TOKEN_ELSE",
        "TOKEN_MATCH",
        "TOKEN_WITH",
        "TOKEN_DO",
        "TOKEN_DEFER",
        "TOKEN_PUB",
        "TOKEN_IMPORT",
        "TOKEN_TYPE",
        "TOKEN_TRAIT",
        "TOKEN_IMPL",
        "TOKEN_AND",
        "TOKEN_OR",
        "TOKEN_NOT",
        "TOKEN_AS",
        "TOKEN_MODULE",
        "TOKEN_FOR",
        "TOKEN_BREAK",
        "TOKEN_CONTINUE",
        "TOKEN_IN",
        "TOKEN_DERIVE",
        "TOKEN_WHERE",
        "TOKEN_NEWTYPE",
        "TOKEN_SPAWN",
        "TOKEN_SEND",
        "TOKEN_RECEIVE",
        "TOKEN_AFTER",
    }
    bracket_tokens = {
        "TOKEN_LPAREN",
        "TOKEN_RPAREN",
        "TOKEN_LBRACKET",
        "TOKEN_RBRACKET",
        "TOKEN_LBRACE",
        "TOKEN_RBRACE",
    }
    delimiter_tokens = {
        "TOKEN_COMMA",
        "TOKEN_COLON",
        "TOKEN_DOT",
        "TOKEN_DOTDOT",
        "TOKEN_DOTDOTEQ",
        "TOKEN_ELLIPSIS",
        "TOKEN_AT",
    }
    operator_tokens = {
        "TOKEN_PLUS",
        "TOKEN_MINUS",
        "TOKEN_STAR",
        "TOKEN_SLASH",
        "TOKEN_PERCENT",
        "TOKEN_POWER",
        "TOKEN_EQ",
        "TOKEN_NE",
        "TOKEN_LT",
        "TOKEN_LE",
        "TOKEN_GT",
        "TOKEN_GE",
        "TOKEN_ASSIGN",
        "TOKEN_BIND",
        "TOKEN_PIPE",
        "TOKEN_BAR",
        "TOKEN_ARROW",
        "TOKEN_FAT_ARROW",
        "TOKEN_QUESTION",
    }

    # Find all TOKEN_* in the file and use the mapping
    token_names = re.findall(r"(TOKEN_\w+)", content)

    for token in set(token_names):  # Use set to avoid duplicates
        if token in token_to_string:
            string_val = token_to_string[token]

            # Categorize based on token sets
            if token in control_flow_tokens:
                tokens["control_flow"].append(string_val)
            if token in storage_tokens:
                tokens["storage"].append(string_val)
            if token in visibility_tokens:
                tokens["visibility"].append(string_val)
            if token in keyword_tokens:
                tokens["keywords"].append(string_val)
            elif token in bracket_tokens:
                tokens["brackets"].append(string_val)
            elif token in delimiter_tokens:
                tokens["delimiters"].append(string_val)
            elif token in operator_tokens:
                tokens["operators"].append(string_val)

    return tokens


def generate_zed_highlights(tokens: dict) -> str:
    """Generate Zed highlights.scm content from token data."""

    # Helper to format a list of strings as Tree-sitter list
    def format_list(items: list, indent: str = "  ") -> str:
        return "\n".join(f'{indent}"{item}"' for item in sorted(set(items)))

    output = """\
; Fern syntax highlighting for Tree-sitter
; AUTO-GENERATED by scripts/generate_editor_support.py
; Do not edit manually - run 'make editor-support' to regenerate

; =============================================================================
; Keywords
; =============================================================================

; All keywords
[
{keywords}
] @keyword

; Control flow keywords
[
{control_flow}
] @keyword.control

; Storage/declaration keywords
[
{storage}
] @keyword.storage

; Visibility
[
{visibility}
] @keyword.modifier

; =============================================================================
; Operators
; =============================================================================

[
{operators}
] @operator

; Logical operators (also keywords)
[
  "and"
  "or"
  "not"
] @keyword.operator

; Pipe operator (special highlight)
"|>" @operator

; Question mark operator
"?" @operator

; =============================================================================
; Punctuation
; =============================================================================

; Delimiters
[
{delimiters}
] @punctuation.delimiter

; Brackets
[
{brackets}
] @punctuation.bracket

; =============================================================================
; Types
; =============================================================================

; Built-in types
[
{builtin_types}
] @type.builtin

; Type constructors (capitalized identifiers)
((identifier) @type
  (#match? @type "^[A-Z][A-Za-z0-9_]*$"))

; =============================================================================
; Literals
; =============================================================================

; Boolean literals
[
{builtin_constants}
] @constant.builtin.boolean

; Result/Option constructors
[
{constructors}
] @constructor

; Numeric literals
(integer_literal) @constant.numeric.integer
(float_literal) @constant.numeric.float

; String literals
(string_literal) @string
(interpolated_string) @string
(string_interpolation) @embedded

; =============================================================================
; Comments
; =============================================================================

(line_comment) @comment.line
(block_comment) @comment.block
(doc_comment) @comment.documentation

; =============================================================================
; Functions
; =============================================================================

; Function definitions
(function_definition
  name: (identifier) @function)

; Function calls
(call_expression
  function: (identifier) @function.call)

; Method calls (after dot)
(method_call
  method: (identifier) @function.method)

; =============================================================================
; Variables and Parameters
; =============================================================================

; Parameters in function definitions
(parameter
  name: (identifier) @variable.parameter)

; Generic identifiers (fallback)
(identifier) @variable

; Wildcard pattern
"_" @constant.builtin

; =============================================================================
; Other
; =============================================================================

; Attributes (@doc, etc.)
(attribute) @attribute

; Module paths
(module_path) @module
"""

    return output.format(
        keywords=format_list(tokens["keywords"]),
        control_flow=format_list(tokens["control_flow"]),
        storage=format_list(tokens["storage"]),
        visibility=format_list(tokens["visibility"]),
        operators=format_list(tokens["operators"]),
        delimiters=format_list(tokens["delimiters"]),
        brackets=format_list(tokens["brackets"]),
        builtin_types=format_list(tokens["builtin_types"]),
        builtin_constants=format_list(tokens["builtin_constants"]),
        constructors=format_list(tokens["constructors"]),
    )


def generate_tree_sitter_grammar(tokens: dict) -> str:
    """Generate Tree-sitter grammar.js from token data."""

    # Format keyword list for grammar
    keywords = sorted(set(tokens["keywords"]))
    keyword_list = ", ".join(f'"{kw}"' for kw in keywords)

    # Format operators for precedence
    operators = tokens["operators"]

    grammar = f"""\
/**
 * Tree-sitter grammar for Fern programming language
 * AUTO-GENERATED by scripts/generate_editor_support.py
 * Do not edit manually - run 'make editor-support' to regenerate
 */

module.exports = grammar({{
  name: 'fern',

  extras: $ => [
    /\\s/,  // Whitespace
    $.line_comment,
    $.block_comment,
  ],

  word: $ => $.identifier,

  conflicts: $ => [
    [$.match_arm, $.pipe_expression],
  ],

  rules: {{
    // =============================================================================
    // Top-level
    // =============================================================================

    source_file: $ => repeat($._statement),

    _statement: $ => choice(
      $.function_definition,
      $.let_binding,
      $.expression_statement,
      $.import_statement,
      $.type_definition,
      $.trait_definition,
      $.impl_definition,
    ),

    // =============================================================================
    // Function definitions
    // =============================================================================

    function_definition: $ => seq(
      optional('pub'),
      'fn',
      field('name', $.identifier),
      field('parameters', $.parameter_list),
      optional(seq('->', field('return_type', $.type))),
      ':',
    ),

    parameter_list: $ => seq(
      '(',
      optional(sep1($.parameter, ',')),
      optional(','),
      ')',
    ),

    parameter: $ => seq(
      field('name', $.identifier),
      optional(seq(':', field('type', $.type))),
    ),

    // =============================================================================
    // Statements
    // =============================================================================

    let_binding: $ => seq(
      'let',
      field('pattern', $.pattern),
      optional(seq(':', field('type', $.type))),
      '=',
      field('value', $.expression),
    ),

    expression_statement: $ => $.expression,

    import_statement: $ => seq(
      'import',
      $.module_path,
    ),

    return_statement: $ => prec.left(seq('return', optional($.expression))),

    // =============================================================================
    // Expressions
    // =============================================================================

    expression: $ => choice(
      $.integer_literal,
      $.float_literal,
      $.string_literal,
      $.boolean_literal,
      $.list_literal,
      $.binary_expression,
      $.unary_expression,
      $.call_expression,
      $.method_call,
      $.if_expression,
      $.match_expression,
      $.pipe_expression,
      $.parenthesized_expression,
      $.member_access,
      $.index_access,
      $.question_expression,
      $.identifier,  // Keep identifier last to prefer more specific matches
    ),

    // Literals
    integer_literal: $ => /[0-9][0-9_]*/,

    float_literal: $ => /[0-9][0-9_]*\\.[0-9][0-9_]*/,

    // String literal - handles both plain and interpolated strings
    string_literal: $ => seq(
      '"',
      repeat(choice(
        $.string_content,
        $.string_interpolation,
        $.escape_sequence,
      )),
      '"',
    ),

    string_content: $ => /[^"\\\\{{]+/,

    escape_sequence: $ => /\\\\./,

    string_interpolation: $ => seq(
      '{{',
      $.expression,
      '}}',
    ),

    boolean_literal: $ => choice('true', 'false'),

    list_literal: $ => seq(
      '[',
      optional(sep1($.expression, ',')),
      optional(','),
      ']',
    ),

    // Binary expressions with precedence
    binary_expression: $ => choice(
      prec.left(1, seq($.expression, 'or', $.expression)),
      prec.left(2, seq($.expression, 'and', $.expression)),
      prec.left(3, seq($.expression, choice('==', '!='), $.expression)),
      prec.left(4, seq($.expression, choice('<', '<=', '>', '>='), $.expression)),
      prec.left(5, seq($.expression, choice('+', '-'), $.expression)),
      prec.left(6, seq($.expression, choice('*', '/', '%'), $.expression)),
      prec.left(7, seq($.expression, '**', $.expression)),
    ),

    unary_expression: $ => prec(8, seq(
      choice('not', '-'),
      $.expression,
    )),

    pipe_expression: $ => prec.left(0, seq(
      $.expression,
      '|>',
      $.expression,
    )),

    question_expression: $ => prec.right(9, seq(
      $.expression,
      '?',
    )),

    // Function and method calls
    call_expression: $ => prec(11, seq(
      field('function', $.identifier),
      field('arguments', $.argument_list),
    )),

    method_call: $ => prec(11, seq(
      field('object', $.expression),
      '.',
      field('method', $.identifier),
      field('arguments', $.argument_list),
    )),

    argument_list: $ => seq(
      '(',
      optional(sep1($.expression, ',')),
      optional(','),
      ')',
    ),

    member_access: $ => prec.left(12, seq(
      $.expression,
      '.',
      $.identifier,
    )),

    index_access: $ => prec.left(12, seq(
      $.expression,
      '[',
      $.expression,
      ']',
    )),

    parenthesized_expression: $ => seq('(', $.expression, ')'),

    // Control flow (precedence 1 to allow pipes inside them)
    if_expression: $ => prec.right(1, seq(
      'if',
      field('condition', $.expression),
      ':',
      field('consequence', $.expression),
      optional(seq(
        'else',
        ':',
        field('alternative', $.expression),
      )),
    )),

    match_expression: $ => prec.right(1, seq(
      'match',
      field('value', $.expression),
      ':',
      repeat1($.match_arm),
    )),

    match_arm: $ => seq(
      field('pattern', $.pattern),
      '=>',
      field('expression', $.expression),
    ),

    // =============================================================================
    // Patterns
    // =============================================================================

    pattern: $ => choice(
      $.identifier,
      '_',
      $.integer_literal,
      $.string_literal,
      $.boolean_literal,
      $.constructor_pattern,
      $.list_pattern,
    ),

    constructor_pattern: $ => seq(
      $.type_constructor,
      optional(seq(
        '(',
        sep1($.pattern, ','),
        optional(','),
        ')',
      )),
    ),

    list_pattern: $ => seq(
      '[',
      optional(sep1($.pattern, ',')),
      optional(','),
      ']',
    ),

    // =============================================================================
    // Types
    // =============================================================================

    type: $ => choice(
      $.type_identifier,
      $.generic_type,
      $.function_type,
      $.tuple_type,
    ),

    type_identifier: $ => /[A-Z][a-zA-Z0-9_]*/,

    type_constructor: $ => /[A-Z][a-zA-Z0-9_]*/,

    generic_type: $ => seq(
      $.type_identifier,
      '(',
      sep1($.type, ','),
      optional(','),
      ')',
    ),

    function_type: $ => seq(
      '(',
      optional(sep1($.type, ',')),
      optional(','),
      ')',
      '->',
      $.type,
    ),

    tuple_type: $ => seq(
      '(',
      sep1($.type, ','),
      optional(','),
      ')',
    ),

    // =============================================================================
    // Type definitions
    // =============================================================================

    type_definition: $ => seq(
      optional('pub'),
      'type',
      field('name', $.type_identifier),
      optional($.type_parameters),
      '=',
      field('definition', $.type_variants),
    ),

    type_variants: $ => sep1($.type_variant, '|'),

    type_variant: $ => prec.left(seq(
      $.type_constructor,
      optional($.variant_fields),
    )),

    variant_fields: $ => seq(
      '(',
      sep1($.type, ','),
      optional(','),
      ')',
    ),

    type_parameters: $ => seq(
      '(',
      sep1($.identifier, ','),
      optional(','),
      ')',
    ),

    trait_definition: $ => seq(
      optional('pub'),
      'trait',
      field('name', $.type_identifier),
    ),

    impl_definition: $ => seq(
      'impl',
      field('trait', $.type_identifier),
      'for',
      field('type', $.type_identifier),
    ),

    // =============================================================================
    // Helpers
    // =============================================================================

    identifier: $ => /[a-z_][a-zA-Z0-9_]*/,

    module_path: $ => sep1($.identifier, '.'),

    _indented_block: $ => repeat1(choice(
      $.expression,
      $.let_binding,
      $.return_statement,
    )),

    // Comments
    line_comment: $ => token(seq('#', /.*/)),

    block_comment: $ => token(seq(
      '/*',
      /[^*]*\\*+([^/*][^*]*\\*+)*/,
      '/',
    )),

    doc_comment: $ => token(seq('##', /.*/)),
  }},
}});

// Helper function for comma-separated lists
function sep1(rule, separator) {{
  return seq(rule, repeat(seq(separator, rule)));
}}
"""

    return grammar


def main():
    """Generate all editor support files."""
    print("Generating editor support from compiler token definitions...")

    # Check source file exists
    if not TOKEN_HEADER.exists():
        print(f"Error: {TOKEN_HEADER} not found", file=sys.stderr)
        return 1

    # Parse tokens
    tokens = parse_tokens(TOKEN_HEADER)

    print(f"  Found {len(tokens['keywords'])} keywords")
    print(f"  Found {len(tokens['operators'])} operators")
    print(f"  Found {len(tokens['delimiters'])} delimiters")

    # Generate Tree-sitter grammar (skip if it has manual indentation support)
    TREE_SITTER_DIR.mkdir(parents=True, exist_ok=True)

    # Check if grammar.js has been manually enhanced with indentation support
    skip_grammar = False
    if GRAMMAR_JS.exists():
        existing_grammar = GRAMMAR_JS.read_text()
        if (
            "externals:" in existing_grammar
            or "external scanner" in existing_grammar.lower()
        ):
            print(f"  Skipping {GRAMMAR_JS} (manual indentation support detected)")
            skip_grammar = True

    if not skip_grammar:
        grammar_content = generate_tree_sitter_grammar(tokens)
        GRAMMAR_JS.write_text(grammar_content)
        print(f"  Generated {GRAMMAR_JS}")

    # Generate Zed highlights
    ZED_HIGHLIGHTS.parent.mkdir(parents=True, exist_ok=True)
    highlights_content = generate_zed_highlights(tokens)
    ZED_HIGHLIGHTS.write_text(highlights_content)
    print(f"  Generated {ZED_HIGHLIGHTS}")

    print("Done!")
    print()
    print("Next steps:")
    print("  1. Install Tree-sitter CLI: npm install -g tree-sitter-cli")
    print("  2. Compile grammar: cd editor/tree-sitter-fern && tree-sitter generate")
    print("  3. Build WASM: tree-sitter build --wasm")
    print(
        "  4. Copy to Zed: cp tree-sitter-fern.wasm ../zed-fern/languages/fern/fern.wasm"
    )
    print()
    print("Or run: make editor-support-compile")
    return 0


if __name__ == "__main__":
    sys.exit(main())
