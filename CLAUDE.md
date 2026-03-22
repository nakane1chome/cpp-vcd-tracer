# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

C++ VCD (Value Change Dump) tracer library for generating waveform trace files viewable in GTKWave and other VCD viewers. BSD 3-Clause licensed.

## Build & Test Commands

```bash
make test              # Build (GCC) and run all tests
make test_clang        # Build (Clang, with fuzzing) and run all tests
make lint              # Run clang-tidy with full compile database (requires build_clang)
make format            # Run clang-format-20 on all source files
make clean             # Remove all build directories
```

Run a single test by name:
```bash
build/test/tests "VCD Top"
```

Pre-commit hooks run clang-format and clang-tidy automatically on staged files.

## Architecture

The library follows a **dependency injection** pattern rather than class hierarchy. Three phases of operation:

1. **Elaboration** — Create module hierarchy and bind signals to it
2. **Header finalization** — Write VCD header, locks the hierarchy
3. **Runtime tracing** — Update signal values and advance time

### Key classes (all in `vcd_tracer` namespace)

- **`top`** — Root trace container. Owns the module tree, manages time updates, writes VCD output. Contains `root` module.
- **`module`** — Hierarchical scope (`$scope` in VCD). Creates child modules and elaborates signals within its scope.
- **`value<T, BIT_SIZE, TRACE_DEPTH, CUR_SEQ>`** — Templated traced signal. Supports bool, integers (signed/unsigned, 8-64 bit), float, double. Optional buffered trace history via TRACE_DEPTH.
- **`value_base`** — Virtual base for type-erased signal access (`set_uint64`, `set_double`, `unknown`, `undriven`).
- **`scope_fn`** — Interface types (function signatures, sequence types) used for dependency injection between classes.

### File layout

- `src/vcd_tracer.hpp` — All class definitions and inline methods
- `src/vcd_tracer.ipp` — Template method implementations (included at end of .hpp)
- `src/vcd_tracer.cpp` — Non-template implementations and explicit template instantiations
- `test/tests.cpp` — Catch2 unit tests
- `test/constexpr_tests.cpp` — Compile-time identifier generator tests
- `test/interop.cpp` + `test/test_interop.py` — Interop tests validated against pyvcd/GTKWave
- `example/signals.cpp` — Complete usage example

## Build System

CMake with C++17. Catch2 v2.x fetched via FetchContent. The `project_warnings` interface library applies strict compiler warnings (`-Werror`). Both the library and test targets link it.

Build variants: default (GCC), clang (with fuzzing/sanitizers), coverage (gcovr).

## Conventions

- `compile_flags.txt` provides `-std=c++17` for clang-tidy and LSP tools without requiring a build
- `make lint` uses the full clang compilation database (`-p build_clang`) for thorough analysis
- Template instantiations for all supported types are explicit in `vcd_tracer.cpp`
- VCD signal states: `known` (normal value), `unknown_x` (outputs "x"), `undriven_z` (outputs "z")
