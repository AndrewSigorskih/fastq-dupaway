from pathlib import Path

import pytest

@pytest.fixture(scope="session")
def exe_path():
    return Path("fastq-dupaway").absolute()

@pytest.fixture(scope="session")
def tests_path():
    return Path(__file__).parent.resolve()
