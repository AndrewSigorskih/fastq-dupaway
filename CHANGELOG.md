# fastq-dupaway change log

## [ 1.5 ] - Dec ?, 2025

- Added "write-clusters" option for sequence-based modes
- Added output streaming for gzipped files
- Overall optimization of outputs saving
- Switched to use fixed name for internal chunks subdir
- [TODO] increase user-defined upper memory limit to 20G

## [ 1.4 ] - Sep 4, 2025

- Finalized "unordered" mode
- Added top-level behavorial tests using pytest
- Created project Docker Hub repo

## [ 1.3 ] - Jul 27, 2025

- Added input streaming for gzipped files
- Dockerfile: Changed Boost source download from jfrog to archives.boost.io
- Dockerfile: Required Boost source archive to match known checksum
- Small CLI improvements

## [ 1.2 ] - Feb 9, 2025 

- Added CMake recipies
- Various small optimizations and bug fixes

## [ 1.1 ] - 2024

- Added "fast" and "tail-hamming" modes

## [ 1.0 ] - 2023

- Proof-of-concept external-sort algorithm fully functional
- "tight" and "loose" modes finalized
