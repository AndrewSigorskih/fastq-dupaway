import subprocess
from pathlib import Path

def test_exe_available(exe_path: Path):
    assert exe_path.exists()


def test_help(exe_path: Path):
    result = subprocess.run(
        [str(exe_path), "-h"],
        capture_output=True,
        text=True
    )

    assert result.returncode == 1

    assert result.stderr.startswith("Supported options:"), "Help message is not printed to stderr!"
