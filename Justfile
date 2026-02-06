# Fern task runner (Just)
# Keep Makefile as the build backend while exposing a modern command UX.

set shell := ["bash", "-euo", "pipefail", "-c"]

default:
    @just --list

debug:
    make debug

release:
    make release

test:
    make test

check:
    make check

release-package:
    make release-package

benchmark-report:
    make benchmark-report

style:
    make style

clean:
    make clean
