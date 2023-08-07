# fastq-dupaway

fastq-dupaway is a program for memory-efficient deduplication of single-end and paired-end FASTQ and FASTA files (both plain-text and gzip-compressed).

## Installation

The only dependency is Boost. Once it is installed, export shell variable <i>BOOST_ROOT</i> pointing to its root directory and then run make.<br>
If you installed Boost from source as admin, it is likely to be <b>/usr/include</b> or any other dir you specified during installation.<br>
Otherwise, if you used conda (miniconda3 in this example) and installed Boost in virtual env <i>ENVNAME</i> the path will be <b>$HOME/miniconda3/envs/ENVNAME/include</b> .

```
export BOOST_ROOT=/PATH/TO/BOOST/ROOT/FOLDER
make
```

### How to install BOOST

In order to install Boost from source, you will need admin rights.<br>
Download source code from official site, configure bootstrap.sh and install:
```
wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
tar xvf boost_1_81_0.tar.gz
cd boost_1_81_0
# next command will save compiled library files under /usr/include/boost
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
        [-m MEMORY-LIMIT] [--format fasta|fastq] [--compare-seq] [--hashed]
```

The only two required arguments are names of input and output files. If only one pair of files was provided, program will run in single-end mode; If both second input and second output filenames were provided, program will run in paired-end mode instead. Complete list of options with explanations is listed below:

```
  -h [ --help ]          Produce help message and exit

  -i [ --input-1 ] arg   First input file (required)

  -u [ --input-2 ] arg   Second input file (optional, enables paired-end mode)

  -o [ --output-1 ] arg  First output file (required)

  -p [ --output-2 ] arg  Second output file (optional, required for paired-end mode)
  -m [ --mem-limit ] arg Memory limit in megabytes (default 2048 = 2Gb).
                         Supported value range is [100 <-> 10240 (10 Gb)]
                         Actual memory usage will slightly exceed this value.
                         The hashtable-based deduplication mode does not 
                         support strict memory limitation,
                         but will most likely not exceed upper bound.

  --format arg           input file format: "fastq" (default) or "fasta".

  --compare-seq          Sequence comparison mode for deduplication step.
                         Supported options:
                         - "tight" (default): compare sequences directly, sequences
                         of different lengths are considered different.
                         - "loose":  compare sequences directly, sequences of 
                         different lengths are considered duplicates if shorter
                         sequence exactly matches with prefix of longer 
                         sequence.
                         
  --hashed               Use hash-based approach instead of sequence-based.
```

## Additional information

Input filenames ending with ".gz" will be treated as binary files compressed by gzip program. If output filenames provided also end with ".gz", output files will be compressed as well.
