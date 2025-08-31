import subprocess
from pathlib import Path

import pytest

def test_exe_available(exe_path: Path):
    assert exe_path.exists(), "fastq-dupaway binary should be present in current directory!"


def test_help(exe_path: Path):
    if not exe_path.exists():
        pytest.fail("fastq-dupaway binary not found in current directory!")

    result = subprocess.run(
        [str(exe_path), "-h"],
        capture_output=True,
        text=True
    )

    assert result.returncode == 1

    assert result.stderr.startswith("fastq-dupaway V"), "Help message is not printed to stderr!"
