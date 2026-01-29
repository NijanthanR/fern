; Fern outline/navigation support for Tree-sitter
; This file defines what appears in the outline/symbol view

; Function definitions
(function_definition
  name: (identifier) @name) @item

; Type definitions
(type_definition
  name: (identifier) @name) @item

; Newtype definitions
(newtype_definition
  name: (identifier) @name) @item

; Trait definitions
(trait_definition
  name: (identifier) @name) @item

; Impl blocks
(impl_block
  trait: (identifier) @name) @item
