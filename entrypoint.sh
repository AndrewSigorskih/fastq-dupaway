#!/bin/sh

# A simple entrypoint wrapper that ensures that /data exists and is a mounted volume
EXECUTABLE="fastq-dupaway"
USAGE="Usage:\n\tdocker run -it --rm -v [Full path to your root data directory]:[workdir-name] -w [workdir-name] fastq-dupaway:latest [OPTIONS]"
WORKDIR="$(pwd)"

ROOT_DEV=$(stat -c %d /)
WORK_DEV=$(stat -c %d "$WORKDIR")

set -e

# just print help and exit
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    exec "${EXECUTABLE}" "--help"
fi

# Check that workdir is mounted
if [ "$ROOT_DEV" = "$WORK_DEV" ]; then
    echo "WARNING: Working directory is not on a mounted volume."
    echo "Temporary files may overfill the container filesystem."
fi
#   echo "ERROR: Working directory is not a mounted volume."
#   echo ""
#   echo "Please run the container with a mounted working directory, e.g.:"
#   echo "  docker run -v \$(pwd):/work -w /work your-image"
#   echo ""
#   echo "Or configure your workflow engine (e.g. Nextflow) to use a work directory."
#   exit 1

# run fastq-dupaway with user-provided arguments
exec "${EXECUTABLE}" "$@"
