# Tests

All tests are registered with CTest and can be run with `ctest` from the build directory.

## Unit Tests

### tests.cpp

Catch2 unit tests for the VCD tracer library. Covers:

- **VCD identifier generation** — verifies the printable ASCII sequence, including correct skipping of reserved `#` and `$` characters.
- **Integer value formatting** — binary encoding of integer values to VCD output.
- **Trace buffer** — buffered value change detection and sequence tracking.
- **Module hierarchy** — scope creation and variable registration.
- **Module elaboration** — deferred signal binding via `elaborate()`.
- **Top-level tracing** — full VCD header generation and time-based value dumps.
- **GitHub Issue #5** — regression test for leading-ones compression bug.

### constexpr_tests.cpp

Compile-time (`constexpr`) tests for the VCD identifier generator. Validates that identifier generation is fully constexpr-evaluable at keys 0, 87, 88, and 176 — covering single-character, boundary, and multi-character identifiers.

Also compiled as `relaxed_constexpr_tests` with `CATCH_CONFIG_RUNTIME_STATIC_REQUIRE` to allow debugging constexpr failures at runtime.

## Interoperability Tests

### interop.cpp

C++ program that generates a VCD file and a reference CSV simultaneously. Uses a `data_cycle` template to cycle signals through test values, covering:

- **Signal types**: bool, float, double, and unsigned integers at widths 1, 2, 4, 7, 8, 9, 15, 16, 17, 24, 31, 32, 33, and 64 bits.
- **Pathological values**: `FLT_MIN/MAX`, `DBL_MIN/MAX`, `DBL_EPSILON`, alternating bit patterns (`0x55`, `0xAA`, `0x5A`) at all integer widths.
- **Hierarchy corner cases**: empty module, pass-through module (only sub-scopes), single-char name, digit-starting name (sanitized to `_1st`), special characters (sanitized), duplicate module names at different levels, 8-level deep nesting, module with same name as a signal.

### test_interop.py

Pytest harness with three tests, all using `interop.cpp` output:

- **test_interop** — Converts VCD to CSV using pyvcd (`vcd_to_csv.py`) and compares against the C++ reference CSV. Uses `pytest.approx` for float tolerance.
- **test_interop_gtkwave** — Exports VCD to CSV using GTKWave's TCL scripting (`gtkwave_export.tcl`) and compares against the C++ reference CSV. Skips if GTKWave is not installed.
- **test_interop_gtkwave_screenshot** — Captures a GTKWave screenshot of the VCD waveforms under `xvfb-run`. Skips if GTKWave, xwd, or xvfb-run are not available.

All three tests save artifacts (VCD, CSV files, screenshot) to `build/artifacts/` by default, or to the path specified by the `INTEROP_ARTIFACTS` environment variable.

### Supporting Files

| File | Purpose |
|------|---------|
| `vcd_to_csv.py` | Converts VCD files to CSV using pyvcd. Handles scalar, vector, and real signal types. |
| `gtkwave_export.tcl` | GTKWave TCL batch script that exports all signals to CSV via marker-based value queries. |
| `gtkwave_screenshot.tcl` | GTKWave TCL batch script that captures a screenshot via `xwd` and converts to PNG. |
| `requirements.txt` | Python dependencies: `pyvcd` and `pytest`. |
| `catch_main.cpp` | Shared Catch2 `main()` entry point for unit tests. |
| `CMakeLists.txt` | Build and test registration for all C++ targets and the pytest suite. |
