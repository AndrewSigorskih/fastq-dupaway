import subprocess
import filecmp

import pytest


ARGS = ("--format", "fasta", "--fast", "--unordered")


@pytest.mark.parametrize(
    "filename",
    [
        "shuffled",
        "skewed",
        "deletion",
        "interleaved",
        "not_overlapped"
    ],
)
def test_unordered(tmp_path, exe_path, tests_path, filename):
    input_file_1 = tests_path / "inputs" / f"unordered_{filename}_r1.fa"
    input_file_2 = tests_path / "inputs" / f"unordered_{filename}_r2.fa"

    output_file_1 = tmp_path / f"unordered_{filename}_r1.fa"
    output_file_2 = tmp_path / f"unordered_{filename}_r2.fa"

    expected_output_1 = tests_path / "expected" / f"unordered_{filename}_r1.fa"
    expected_output_2 = tests_path / "expected" / f"unordered_{filename}_r2.fa"

    result = subprocess.run(
        [str(exe_path), "-i", str(input_file_1), "-u", str(input_file_2),
         "-o", str(output_file_1), "-p", str(output_file_2), *ARGS],
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
