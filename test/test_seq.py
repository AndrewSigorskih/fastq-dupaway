import subprocess
import filecmp

import pytest


@pytest.mark.parametrize(
    "filename, cli_args",
    [
        ("single_tight.fa", ["--format", "fasta"]),
        ("single_loose.fa", ["--format", "fasta", "--compare-seq", "loose"]),
        ("single_hamming.fa", ["--format", "fasta", "--compare-seq", "tail-hamming", "--distance", "1"]),
    ],
)
def test_single_fasta(tmp_path, exe_path, tests_path, filename, cli_args):
    input_file = tests_path / "inputs" / filename
    expected_output = tests_path / "expected" / filename
    output_file = tmp_path / filename

    # Run CLI process
    result = subprocess.run(
        [str(exe_path), "-i", str(input_file), "-o", str(output_file), *cli_args],
        capture_output=True,
        text=True
    )

    # Ensure it ran successfully
    assert result.returncode == 0, f"fastq-dupaway failed: {result.stderr}"

    # Ensure output file exists
    assert output_file.exists(), "Output file was not created."

    # Compare generated output file with expected output file
    files_match = filecmp.cmp(output_file, expected_output, shallow=False)
    assert files_match, f"Output file {output_file} does not match expected {expected_output}"


@pytest.mark.parametrize(
    "filename, cli_args",
    [
        ("paired_tight", ["--format", "fasta"]),
    ],
)
def test_paired_fasta(tmp_path, exe_path, tests_path, filename, cli_args):
    input_file_1 = tests_path / "inputs" / f"{filename}_r1.fa"
    input_file_2 = tests_path / "inputs" / f"{filename}_r2.fa"

    output_file_1 = tmp_path / f"{filename}_r1.fa"
    output_file_2 = tmp_path / f"{filename}_r2.fa"

    expected_output_1 = tests_path / "expected" / f"{filename}_r1.fa"
    expected_output_2 = tests_path / "expected" / f"{filename}_r2.fa"

    result = subprocess.run(
        [str(exe_path), "-i", str(input_file_1), "-u", str(input_file_2),
         "-o", str(output_file_1), "-p", str(output_file_2), *cli_args],
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


def test_nonmatching_outputs(tmp_path, exe_path, tests_path):
    input_file = tests_path / "inputs" / "single_tight.fa"
    expected_output = tests_path / "expected" / "single_hamming.fa"
    output_file = tmp_path / "single_tight.fa"

    result = subprocess.run(
        [str(exe_path), "-i", str(input_file), "-o", str(output_file), "--format", "fasta"],
        capture_output=True,
        text=True
    )

    assert result.returncode == 0, f"fastq-dupaway failed: {result.stderr}"

    assert output_file.exists(), "Output file was not created."

    files_match = filecmp.cmp(output_file, expected_output, shallow=False)
    assert not files_match, f"Output file {output_file} was expected to not match {expected_output}"
