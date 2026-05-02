# fastq-dupaway change log

## [ Planned ]

- [TODO] Increase user-defined upper memory limit to at least 20G
- [TODO] Further optimize size of output write buffers

## [ 1.5 ] - May 3rd, 2026

- Added "write-clusters" option for sequence-based modes
- Added "verbose" option to print summary after program execution
- Added output streaming for gzipped files
- Overall optimization of outputs saving logic, skipping creation of some intermediate files
- Switched to use fixed name for internal chunks subdir
- Introduced an increased size buffer for disk write operations, reducing number of separate writev calls from 40% to 4600% in some cases
- Adjusted container's entrypoint to allow user/workflow engine use custom workdir
- Slightly reduced image size

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

- Added CMake build support
- Various small optimizations and bug fixes

## [ 1.1 ] - 2024

- Added "fast" and "tail-hamming" modes

## [ 1.0 ] - 2023

- Proof-of-concept external-sort algorithm fully functional
- "tight" and "loose" modes finalized
