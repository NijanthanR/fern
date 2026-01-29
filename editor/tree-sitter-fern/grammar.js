/**
 * Tree-sitter grammar for Fern programming language
 * Supports indentation-based syntax with external scanner
 */

module.exports = grammar({
  name: "fern",

  externals: ($) => [$._newline, $._indent, $._dedent],

  extras: ($) => [
    /[ \t]/, // Whitespace (but not newlines - those are significant!)
    $.line_comment,
    $.block_comment,
  ],

  word: ($) => $.identifier,

  conflicts: ($) => [
    [$.binary_expression, $.pipe_expression],
    [$.method_call, $.member_access],
    [$.non_pipe_expression, $.expression],
  ],

  rules: {
    // =============================================================================
    // Top-level
    // =============================================================================

    source_file: ($) =>
      seq(
        optional($._newline),
        optional(seq(sep1($._statement, $._newline), optional($._newline))),
      ),

    _statement: ($) =>
      choice(
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

    function_definition: ($) =>
      seq(
        optional("pub"),
        "fn",
        field("name", $.identifier),
        field("parameters", $.parameter_list),
        optional(seq("->", field("return_type", $.type))),
        ":",
        field("body", $._block),
      ),

    _block: ($) =>
      choice(
        // Multiline indented block
        seq($._newline, $._indent, sep1($._statement, $._newline), $._dedent),
        // Inline single expression (must not end with pipe)
        prec.dynamic(
          1,
          seq(/[ \t]+/, alias($.non_pipe_expression, $.expression)),
        ),
      ),

    // Expression that cannot be a pipe expression (to avoid ambiguity in inline blocks)
    non_pipe_expression: ($) =>
      choice(
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
        $.parenthesized_expression,
        $.member_access,
        $.index_access,
        $.question_expression,
        $.identifier,
        $.return_statement,
      ),

    parameter_list: ($) =>
      seq("(", optional(sep1($.parameter, ",")), optional(","), ")"),

    parameter: ($) =>
      seq(
        field("name", $.identifier),
        optional(seq(":", field("type", $.type))),
      ),

    // =============================================================================
    // Statements
    // =============================================================================

    let_binding: ($) =>
      seq(
        "let",
        field("pattern", $.pattern),
        optional(seq(":", field("type", $.type))),
        "=",
        field("value", $.expression),
      ),

    expression_statement: ($) => $.expression,

    import_statement: ($) => seq("import", $.module_path),

    return_statement: ($) => prec.left(seq("return", optional($.expression))),

    // =============================================================================
    // Expressions
    // =============================================================================

    expression: ($) =>
      choice(
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
        $.identifier,
        $.return_statement,
      ),

    // Literals
    integer_literal: ($) => /[0-9][0-9_]*/,

    float_literal: ($) => /[0-9][0-9_]*\.[0-9][0-9_]*/,

    string_literal: ($) =>
      seq('"', repeat(choice(/[^"\\{]+/, /\\./, $.string_interpolation)), '"'),

    string_interpolation: ($) => seq("{", $.expression, "}"),

    boolean_literal: ($) => choice("true", "false"),

    list_literal: ($) =>
      seq("[", optional(sep1($.expression, ",")), optional(","), "]"),

    // Binary expressions with precedence
    binary_expression: ($) => {
      const table = [
        [prec.left, 1, choice("or")],
        [prec.left, 2, choice("and")],
        [prec.left, 3, choice("==", "!=", "<", "<=", ">", ">=")],
        [prec.left, 4, choice("in")],
        [prec.left, 5, choice("+", "-")],
        [prec.left, 6, choice("*", "/", "%")],
        [prec.right, 7, choice("**")],
      ];

      return choice(
        ...table.map(([fn, precedence, operators]) =>
          fn(
            precedence,
            seq(
              field("left", $.expression),
              field("operator", operators),
              field("right", $.expression),
            ),
          ),
        ),
      );
    },

    pipe_expression: ($) =>
      prec.left(
        0,
        seq(field("left", $.expression), "|>", field("right", $.expression)),
      ),

    unary_expression: ($) =>
      prec(
        8,
        seq(
          field("operator", choice("not", "-")),
          field("operand", $.expression),
        ),
      ),

    call_expression: ($) =>
      prec(
        9,
        seq(
          field("function", $.expression),
          field("arguments", $.argument_list),
        ),
      ),

    argument_list: ($) =>
      seq("(", optional(sep1($.expression, ",")), optional(","), ")"),

    method_call: ($) =>
      prec(
        10,
        seq(
          field("object", $.expression),
          ".",
          field("method", $.identifier),
          field("arguments", $.argument_list),
        ),
      ),

    member_access: ($) =>
      prec(
        10,
        seq(field("object", $.expression), ".", field("member", $.identifier)),
      ),

    index_access: ($) =>
      prec(
        10,
        seq(
          field("object", $.expression),
          "[",
          field("index", $.expression),
          "]",
        ),
      ),

    question_expression: ($) =>
      prec(11, seq(field("operand", $.expression), "?")),

    parenthesized_expression: ($) => seq("(", $.expression, ")"),

    // If expressions (right associative to handle nested if-else)
    if_expression: ($) =>
      prec.right(
        seq(
          "if",
          field("condition", $.expression),
          ":",
          field("consequence", $._block),
          optional(seq("else", ":", field("alternative", $._block))),
        ),
      ),

    // Match expressions
    match_expression: ($) =>
      seq(
        "match",
        field("value", $.expression),
        ":",
        $._newline,
        $._indent,
        sep1($.match_arm, $._newline),
        $._dedent,
      ),

    match_arm: ($) =>
      seq(
        field("pattern", $.pattern),
        "=>",
        field("value", $.expression),
        optional(","),
      ),

    // =============================================================================
    // Patterns
    // =============================================================================

    pattern: ($) =>
      choice(
        $.identifier,
        $.integer_literal,
        $.string_literal,
        $.boolean_literal,
        $.list_pattern,
        $.constructor_pattern,
        "_",
      ),

    list_pattern: ($) =>
      seq("[", optional(sep1($.pattern, ",")), optional(","), "]"),

    constructor_pattern: ($) =>
      seq(
        field("constructor", $.type_identifier),
        optional(seq("(", optional(sep1($.pattern, ",")), optional(","), ")")),
      ),

    // =============================================================================
    // Types
    // =============================================================================

    type: ($) =>
      choice($.type_identifier, $.function_type, $.list_type, $.tuple_type),

    type_identifier: ($) => /[A-Z][A-Za-z0-9_]*/,

    function_type: ($) =>
      seq(
        field("parameter_types", $.type_list),
        "->",
        field("return_type", $.type),
      ),

    type_list: ($) => seq("(", optional(sep1($.type, ",")), optional(","), ")"),

    list_type: ($) => seq("List", "[", field("element_type", $.type), "]"),

    tuple_type: ($) => seq("(", sep1($.type, ","), optional(","), ")"),

    // =============================================================================
    // Type definitions
    // =============================================================================

    type_definition: ($) =>
      seq(
        optional("pub"),
        "type",
        field("name", $.type_identifier),
        optional(field("type_parameters", $.type_parameter_list)),
        "=",
        field("definition", $.type_variant_list),
      ),

    type_parameter_list: ($) =>
      seq("[", sep1($.type_identifier, ","), optional(","), "]"),

    type_variant_list: ($) => sep1($.type_variant, "|"),

    type_variant: ($) =>
      seq(
        field("name", $.type_identifier),
        optional(field("fields", $.type_list)),
      ),

    // Trait definitions
    trait_definition: ($) =>
      seq(
        optional("pub"),
        "trait",
        field("name", $.type_identifier),
        optional(field("type_parameters", $.type_parameter_list)),
        ":",
        field("body", $._trait_body),
      ),

    _trait_body: ($) =>
      seq($._newline, $._indent, sep1($.trait_method, $._newline), $._dedent),

    trait_method: ($) =>
      seq(
        "fn",
        field("name", $.identifier),
        field("parameters", $.parameter_list),
        optional(seq("->", field("return_type", $.type))),
      ),

    // Impl definitions
    impl_definition: ($) =>
      seq(
        "impl",
        field("trait", $.type_identifier),
        "for",
        field("type", $.type_identifier),
        ":",
        field("body", $._impl_body),
      ),

    _impl_body: ($) =>
      seq(
        $._newline,
        $._indent,
        sep1($.function_definition, $._newline),
        $._dedent,
      ),

    // =============================================================================
    // Identifiers and paths
    // =============================================================================

    identifier: ($) => /[a-z_][a-zA-Z0-9_]*/,

    module_path: ($) => sep1($.identifier, "."),

    // =============================================================================
    // Comments
    // =============================================================================

    line_comment: ($) => token(seq("#", /.*/)),

    block_comment: ($) => token(seq("/*", /[^*]*\*+([^/*][^*]*\*+)*/, "/")),

    doc_comment: ($) => token(seq("##", /.*/)),
  },
});

// Helper function for comma-separated lists
function sep1(rule, separator) {
  return seq(rule, repeat(seq(separator, rule)));
}
