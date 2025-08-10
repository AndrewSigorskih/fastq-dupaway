#!/bin/sh

# A simple entrypoint wrapper that ensures that /data exists and is a mounted volume
EXECUTABLE="fastq-dupaway"
USAGE="Usage:\n\tdocker run -it --rm -v [Full path to your root data directory]:/data fastq-dupaway:latest [OPTIONS]"

set -e

# just print help and exit
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    exec "${EXECUTABLE}" "--help"
fi

# Check that /data is mounted
if [ ! -d /data ]; then
    echo "ERROR: /data directory does not exist." >&2
    exit 1
fi

if ! mountpoint -q /data; then
    echo "ERROR: /data is not a mounted volume.\n${USAGE}" >&2
    exit 1
fi

# run fastq-dupaway with user-provided arguments
cd /data
exec "${EXECUTABLE}" "$@"
