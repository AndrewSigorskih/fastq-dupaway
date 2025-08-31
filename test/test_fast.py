import subprocess
import filecmp

import pytest


def test_single_fast(tmp_path, exe_path, tests_path):
    if not exe_path.exists():
        pytest.fail("fastq-dupaway binary not found in current directory!")
    
    input_file = tests_path / "inputs" / "single_fast.fa"
    expected_output = tests_path / "expected" / "single_fast.fa"
    output_file = tmp_path / "single_fast.fa"

    result = subprocess.run(
        [str(exe_path), "-i", str(input_file), "-o", str(output_file), "--format", "fasta", "--fast"],
        capture_output=True,
        text=True
    )

    assert result.returncode == 0, f"fastq-dupaway failed: {result.stderr}"

    assert output_file.exists(), "Output file was not created."

    files_match = filecmp.cmp(output_file, expected_output, shallow=False)
    assert files_match, f"Output file {output_file} does not match expected {expected_output}"


def test_paired_fast(tmp_path, exe_path, tests_path):
    if not exe_path.exists():
        pytest.fail("fastq-dupaway binary not found in current directory!")
    
    input_file_1 = tests_path / "inputs" / f"paired_fast_r1.fa"
    input_file_2 = tests_path / "inputs" / f"paired_fast_r2.fa"

    output_file_1 = tmp_path / f"paired_fast_r1.fa"
    output_file_2 = tmp_path / f"paired_fast_r2.fa"

    expected_output_1 = tests_path / "expected" / f"paired_fast_r1.fa"
    expected_output_2 = tests_path / "expected" / f"paired_fast_r2.fa"

    result = subprocess.run(
        [str(exe_path), "-i", str(input_file_1), "-u", str(input_file_2),
         "-o", str(output_file_1), "-p", str(output_file_2), "--format", "fasta", "--fast"],
        capture_output=True,
        text=True
    )

    assert result.returncode == 0, f"fastq-dupaway failed: {result.stderr}"

    for output, expected in zip(
        (output_file_1, output_file_2),
        (expected_output_1, expected_output_2)
    ):
        assert output.exists(), f"Output file {output} was not created!"
        files_match = filecmp.cmp(output, expected, shallow=False)
        assert files_match, f"Output file {output} does not match expected {expected}"
