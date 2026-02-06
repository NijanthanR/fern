# Building Fern Compiler

## Prerequisites

- C compiler (clang or gcc)
- Just task runner (`just`)
- SQLite development library (`sqlite3`)
- macOS, Linux, or other Unix-like OS

## Quick Start

```bash
# Build the compiler (debug mode)
just debug
# Run tests
just test

# Build release version
just release

# Clean build artifacts
just clean
```

## Build Targets

### Development

- `just debug` - Build debug version with symbols and assertions
- `just test` - Build and run all tests
- `just clean` - Remove all build artifacts

### Production

- `just release` - Build optimized release version

### Installation

- `just install` - Install fern to `/usr/local/bin`
- `just uninstall` - Remove installed fern

### Debugging

- `just memcheck` - Run with Valgrind for memory leak detection

### Code Quality

- `just fmt` - Format code with clang-format

## Project Structure

```
fern/
├── src/         # Compiler source code
│   └── main.c   # Entry point
├── lib/         # Internal libraries (arena, string, etc.)
├── include/     # Header files
├── tests/       # Test suite
├── build/       # Build artifacts (generated)
├── bin/         # Compiled binaries (generated)
└── examples/    # Example Fern programs
```

## Running the Compiler

```bash
# After building
./bin/fern examples/hello.fn
```

## Running Tests

```bash
just test
```

All tests should pass. If any test fails, please report it as a bug.

## Development Workflow

### First Time Setup

```bash
# Install git hooks for automatic quality checks
./scripts/install-hooks.sh
```

This installs a pre-commit hook that automatically:
- Compiles code with strict warnings
- Runs all tests
- Checks for common mistakes (malloc/free, manual unions, etc.)
- Reminds you to update ROADMAP.md

### Daily Development

1. Make changes to source code
2. Run `just test` to verify (or rely on pre-commit hook)
3. Commit changes (pre-commit hook runs automatically)
4. Update ROADMAP.md to track progress

**Note:** The pre-commit hook will prevent commits if tests fail or code doesn't compile.

## Compiler Flags

### Debug Build

- `-std=c11` - C11 standard
- `-Wall -Wextra -Wpedantic -Werror` - All warnings as errors
- `-g` - Debug symbols
- `-O0` - No optimization
- `-DDEBUG` - Debug mode defines

### Release Build

- `-std=c11` - C11 standard
- `-Wall -Wextra -Wpedantic -Werror` - All warnings as errors
- `-O2` - Optimization level 2
- `-DNDEBUG` - Release mode (disables asserts)

## Troubleshooting

### "clang: command not found"

Install clang:
```bash
# macOS
xcode-select --install

# Ubuntu/Debian
sudo apt-get install clang

# Fedora
sudo dnf install clang
```

### "just: command not found"

Install just:
```bash
# macOS
brew install just

# Ubuntu/Debian
sudo apt-get install just

# Fedora
sudo dnf install just
```

### Tests fail

1. Run `just clean` to remove stale build artifacts
2. Run `just test` again
3. If still failing, check the error message and report a bug

### "ld: cannot find -lsqlite3" (or sqlite link errors)

Install SQLite development headers/libraries:
```bash
# macOS
brew install sqlite

# Ubuntu/Debian
sudo apt-get install libsqlite3-dev

# Fedora
sudo dnf install sqlite-devel
```

## Next Steps

See [ROADMAP.md](ROADMAP.md) for the implementation plan and next milestones.
