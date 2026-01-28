# Fern

> A statically-typed, functional language with Python aesthetics that compiles to single binaries.

**Status:** 🚧 In active development - Milestone 2 (Parser) in progress, 36/36 tests passing

## What is Fern?

Fern is a programming language designed to make both **fast CLI tools** (<1 MB) and **full-stack applications** (2-4 MB) with the same elegant syntax. No runtime dependencies, no crashes, predictable behavior.

```fern
# Clean, readable syntax
content <- read_file("config.txt")
config <- parse_config(content)
validated <- validate(config)
Ok(process(validated))
```

## Design Philosophy

- **Readability first** - Python-like indentation, no braces
- **Functional-first** - Immutable by default, pure functions
- **No crashes** - No null, no exceptions, no panics - only Result types
- **Predictable** - Explicit error handling, no hidden control flow
- **Dual-purpose** - Same code for CLI tools and servers
- **Single binary** - Deploy one file, no dependencies

## Key Features

- **Static types** with inference - safety without verbosity
- **Pattern matching** - exhaustiveness checking catches bugs
- **Actor concurrency** - Erlang/Elixir-style lightweight processes
- **Embedded database** - libSQL included (SQLite-compatible)
- **Helpful errors** - compiler guides you to solutions
- **Doc tests** - examples in docs are automatically tested

## Current Status

We're implementing the compiler using test-driven development. See [DESIGN.md](DESIGN.md) for the complete language specification.

**Completed:**
- ✅ Milestone 0: Project Setup
  - Build system and test infrastructure  
  - Safety libraries (arena, strings, collections)
  - Development workflow and pre-commit hooks
  - CI/CD pipeline
- ✅ Milestone 1: Lexer (core complete)
  - All keywords, operators, literals
  - Critical `<-` operator working
  - 23/23 tests passing

**In Progress:**
- 🚧 Milestone 2: Parser
  - Core parser complete (36/36 tests passing)
  - Expression parsing with operator precedence
  - Statement parsing (let, return)
  - Function calls, binary/unary operators
  - Remaining: function definitions, blocks, type annotations

**Next:**
- Complete Milestone 2 (Parser)
- Milestone 3: Type System
- See [ROADMAP.md](ROADMAP.md) for full plan

## Autonomous Development (Ralph Loop)

We're using an autonomous agent system called **Ralph Loop** for systematic development:
- Two agents (IMPLEMENTER and CONTROLLER) work together
- Communication via ROADMAP.md
- Strict TDD: tests first, implementation second, verification third
- See [RALPH.md](RALPH.md) for architecture details

## Documentation

- [Language Design](DESIGN.md) - Complete specification
- [Implementation Roadmap](ROADMAP.md) - Development plan with Ralph Loop status
- [Ralph Loop System](RALPH.md) - Autonomous development architecture
- [Decision Log](DECISIONS.md) - Architectural decisions
- [Development Guidelines](CLAUDE.md) - For AI-assisted development

## Inspiration

Fern takes inspiration from the best features of:
- **Gleam** - Type system, simplicity
- **Elixir** - Pattern matching, actors, pragmatic design
- **Roc** - Result binding, backpassing clarity
- **Zig** - Comptime, defer, minimalism
- **Python** - Readability, aesthetics
- **Go** - Single binary deployment

## Philosophy in Action

**Replace your infrastructure:**
```fern
# No Redis, no RabbitMQ, no separate database
fn main() -> Result((), Error):
    db = sql.open("app.db")?              # Embedded database
    cache = spawn(cache_actor)            # In-memory cache (actor)
    queue = spawn(job_queue)              # Job queue (actor)
    
    http.serve(8080, handler(db, cache, queue))
    
# One 3.5MB binary, no external dependencies
```

**CLI tools stay tiny:**
```fern
# Fast, small, no runtime
fn main() -> Result((), Error):
    data <- read_file("data.csv")
    processed <- process(data)
    write_file("output.csv", processed)
    
# 600KB binary, <5ms startup
```

## License

MIT License - see [LICENSE](LICENSE) for details.

**What this means:** You can use Fern for anything (personal, commercial, proprietary) without restrictions. We want Fern to be as widely useful as possible.

## Contributing

We're actively implementing the compiler using AI-assisted test-driven development. See [CLAUDE.md](CLAUDE.md) for development workflow and guidelines.

To get started:
```bash
# Install git hooks
./scripts/install-hooks.sh

# Build and test
make test

# See ROADMAP.md for current tasks
```