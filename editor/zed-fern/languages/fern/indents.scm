; Fern indentation rules for Tree-sitter

; Indent after colons (function/if/else/match blocks)
[
  ":"
] @indent

; Dedent on these tokens
[
  "else"
] @outdent
