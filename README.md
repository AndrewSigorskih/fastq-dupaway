# fastq-dupaway

fastq-dupaway is a program for efficient deduplication of single-end and paired-end NGS data (FASTQ or single-line FASTA DNA sequence files, plain-text or gzip-compressed).

fastq-dupaway offers two main working modes depending on user's needs:

* the "sequence-based" mode runs with user-defined RAM usage upper limit, and allows the processing of huge NGS datasets on resource-limited machines. For this mode, several types of duplicate definition are available.
* the "hash-based" mode allows to process large files significantly faster in exchange for unbounded RAM usage and limited deduplication logic (only direct duplicated will be filtered out). For paired-end input, this mode can be additionally triggered to process input files with unsynchronized order of reads.

## Installation

The only dependency is Boost. Once it is installed, <ins>export shell variable</ins> <i>BOOST_ROOT</i> pointing to its root directory and then run make.<br>
If you installed Boost from source as admin, it is likely to be <b>/usr/include</b> or any other dir you specified during installation.<br>
Otherwise, if you used conda (miniconda3 in this example) and installed Boost in virtual env <i>ENVNAME</i> the path will be <b>$HOME/miniconda3/envs/ENVNAME/include</b> .

```
export BOOST_ROOT=/PATH/TO/BOOST/ROOT/FOLDER
make
```

#### How to install BOOST

In order to install Boost from source, you will need admin rights.<br>
Download source code from official site, configure bootstrap.sh and install:
```
wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
tar xvf boost_1_81_0.tar.gz
cd boost_1_81_0
# these commands will save compiled library files under /usr/include/boost
./bootstrap.sh --prefix=/usr/
sudo ./b2 install
```

Alternativaly, you can easily install Boost via conda.
```
conda install -c conda-forge boost
```

## Usage

```
fastq-dupaway -i INPUT-1 [-u INPUT-2] -o OUTPUT-1 [-p OUTPUT-2] \
        [-m MEMORY-LIMIT] [--format fasta|fastq] \
        ([--compare-seq MODE] | [--hashed [--unordered]])
```

The only two required arguments are names of input and output files. If only one pair of files was provided, program will run in single-end mode; If both second input and second output filenames were provided, program will run in paired-end mode instead. Complete list of options with explanations is listed below:

Option|Value|Description
---|---|---
-h/--help|-|Produce help message and exit.
-i/--input-1|string|First input file (required).
-u/--input-2|string|Second input file (optional, enables paired-end mode).
-o/--output-1|string|First output file (required).
-p/--output-2|string|Second output file (required for paired-end mode).
-m/--mem-limit|integer in range [500, 10240]|Memory limit in megabytes (default 2048 = 2Gb).<br>The hashtable-based deduplication mode does not support strict memory limitation.
--format|either "fastq" (default) or "fasta"|Input file format.
--compare-seq|string (see description)|Sequence comparison logic for sequence-based mode.<br>Supported values:<br>- "tight" (default): compare sequences directly, sequences of different lengths are considered different.<br>- "loose":  compare sequences directly, sequences of different lengths are considered duplicates if shorter sequence exactly matches with prefix of longer sequence.<br>- "hamming": consider a pair of sequnces as duplicates if their Hamming distance is less or equal than threshold. Sequences of different lengths will not be compared.
--distance|non-negative integer|A threshold value for Hamming distance calculation. Default value is 2.
--hashed|-|Use hash-based approach instead of sequence-based. In this mode the program will run significantly faster, however no memory limit can be set and only complete duplicates will be filtered out.
--unordered|-|This option is supported only by hash mode for paired inputs. Use this flag if reads in your paired input files are not synchronized (i.e. the reads order determined by read IDs does not match). If this option is enabled, both input files will be sorted by read IDs before deduplication.


## Additional information

Input filenames ending with ".gz" will be treated as binary files compressed by gzip program. If output filenames provided also end with ".gz", output files will be compressed as well.
