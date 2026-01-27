# Fern

> A statically-typed, functional language with Python aesthetics that compiles to single binaries.

**Status:** 🚧 In active design phase - language specification in progress

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

We're currently designing the language specification. See [design.md](design.md) for the complete language design.

**Next steps:**
- [ ] Finalize language specification
- [ ] Implement lexer and parser
- [ ] Build type system
- [ ] QBE code generation
- [ ] Standard library

## Documentation

- [Language Design](design.md) - Complete specification
- [Development Guidelines](CLAUDE.md) - For contributors

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

[License to be determined]

## Contributing

Currently in design phase. Contributions to the language design are welcome via issues and discussions.