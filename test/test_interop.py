import subprocess
import csv
import os
import shutil
import pytest
from vcd_to_csv import vcd_to_csv


def get_interop_exe():
    exe = os.environ.get("INTEROP_EXE")
    if not exe:
        pytest.skip("INTEROP_EXE not set")
    return exe


def parse_csv(path):
    with open(path) as f:
        return list(csv.reader(f))


def values_equal(a, b):
    """Compare two CSV cell values, using numeric comparison for floats."""
    if a == b:
        return True
    try:
        return float(a) == pytest.approx(float(b), rel=1e-9)
    except (ValueError, TypeError):
        return False


def test_interop(tmp_path):
    exe = get_interop_exe()

    vcd_path = str(tmp_path / "interop.vcd")
    cpp_csv_path = str(tmp_path / "cpp_output.csv")
    py_csv_path = str(tmp_path / "py_output.csv")

    # Run the C++ interop executable to produce VCD and reference CSV
    result = subprocess.run(
        [exe, vcd_path, cpp_csv_path],
        capture_output=True, text=True
    )
    assert result.returncode == 0, f"interop failed: {result.stderr}"

    # Convert VCD to CSV using pyvcd
    vcd_to_csv(vcd_path, py_csv_path)

    cpp_rows = parse_csv(cpp_csv_path)
    py_rows = parse_csv(py_csv_path)

    # Compare headers
    assert cpp_rows[0] == py_rows[0], (
        f"Header mismatch:\n  C++:   {cpp_rows[0]}\n  pyvcd: {py_rows[0]}"
    )

    # Compare row count
    assert len(cpp_rows) == len(py_rows), (
        f"Row count mismatch: C++ has {len(cpp_rows)}, pyvcd has {len(py_rows)}"
    )

    # Compare data rows with numeric tolerance for floats
    for i, (cpp_row, py_row) in enumerate(zip(cpp_rows[1:], py_rows[1:]), 1):
        assert len(cpp_row) == len(py_row), (
            f"Row {i}: column count mismatch"
        )
        for j, (c, p) in enumerate(zip(cpp_row, py_row)):
            assert values_equal(c, p), (
                f"Row {i}, col {j} ({cpp_rows[0][j]}): "
                f"C++={c!r} != pyvcd={p!r}"
            )


def test_interop_gtkwave(tmp_path):
    exe = get_interop_exe()

    if not shutil.which("gtkwave"):
        pytest.skip("gtkwave not found")

    vcd_path = str(tmp_path / "interop.vcd")
    cpp_csv_path = str(tmp_path / "cpp_output.csv")
    gtk_csv_path = str(tmp_path / "gtkwave_output.csv")

    # Run the C++ interop executable to produce VCD and reference CSV
    result = subprocess.run(
        [exe, vcd_path, cpp_csv_path],
        capture_output=True, text=True
    )
    assert result.returncode == 0, f"interop failed: {result.stderr}"

    # Run GTKWave with TCL export script
    tcl_script = os.path.join(os.path.dirname(__file__), "gtkwave_export.tcl")
    env = os.environ.copy()
    env["GTKWAVE_CSV_OUT"] = gtk_csv_path

    # Use xvfb-run for headless operation if available, otherwise run directly
    if shutil.which("xvfb-run"):
        cmd = ["xvfb-run", "gtkwave", vcd_path, "-S", tcl_script]
    else:
        cmd = ["gtkwave", vcd_path, "-S", tcl_script]

    result = subprocess.run(
        cmd, capture_output=True, text=True, env=env, timeout=30
    )
    assert result.returncode == 0, f"gtkwave failed: {result.stderr}"

    # Compare CSVs
    cpp_rows = parse_csv(cpp_csv_path)
    gtk_rows = parse_csv(gtk_csv_path)

    assert cpp_rows[0] == gtk_rows[0], (
        f"Header mismatch:\n  C++:     {cpp_rows[0]}\n  gtkwave: {gtk_rows[0]}"
    )
    assert len(cpp_rows) == len(gtk_rows), (
        f"Row count mismatch: C++ has {len(cpp_rows)}, gtkwave has {len(gtk_rows)}"
    )

    for i, (cpp_row, gtk_row) in enumerate(zip(cpp_rows[1:], gtk_rows[1:]), 1):
        assert len(cpp_row) == len(gtk_row), (
            f"Row {i}: column count mismatch"
        )
        for j, (c, g) in enumerate(zip(cpp_row, gtk_row)):
            assert values_equal(c, g), (
                f"Row {i}, col {j} ({cpp_rows[0][j]}): "
                f"C++={c!r} != gtkwave={g!r}"
            )
