# fastq-dupaway

fastq-dupaway is a program for efficient deduplication of single-end and paired-end NGS data (FASTQ or single-line FASTA DNA sequence files, plain-text or gzip-compressed).

fastq-dupaway offers two main working modes depending on user's needs:

* the "sequence-based" mode runs with user-defined RAM usage upper limit, and allows the processing of huge NGS datasets on resource-limited machines. For this mode, several types of duplicate definition are available.
* the "fast" mode allows to process large files significantly faster in exchange for unbounded RAM usage and limited deduplication logic (only direct duplicated will be filtered out). For paired-end input, this mode can be additionally triggered to process input files with unsynchronized order of reads.


# Table of contents
1. [Installation](#installation-main)
    1. [Using Docker image](#installation-docker-pull)
    2. [Using local docker build](#installation-docker-build)
    3. [Using Bioconda](#installation-bioconda)
    4. [Building from source](#installation-build-source)
2. [Usage](#usage-main)
    1. [Running Docker image](#usage-docker)
    2. [Program options](#usage-options)
3. [Detailed explanation of program options and algorithms](#explanation)
4. [Additional information](#extra-info)
5. [Running tests](#tests)
6. [How to cite](#cite-info)


## Installation <a name="installation-main"></a>

### Using Docker image (recommended) <a name="installation-docker-pull"></a>

#### Pull image from Docker Hub

```bash
docker pull asigorskikh/fastq-dupaway:latest
docker tag asigorskikh/fastq-dupaway:latest fastq-dupaway
# check that everything went as expected (should print help message and exit):
docker run -it --rm fastq-dupaway --help
```

#### Or clone this repository and build the image <a name="installation-docker-build"></a>

```bash
docker build -t fastq-dupaway .
# check that everything went as expected (should print help message and exit):
docker run -it --rm fastq-dupaway --help
```

### Alternatively, fastq-dupaway can be installed as a bioconda package <a name="installation-bioconda"></a>

```bash
conda create -n fq-dpw -c bioconda -c conda-forge fastq-dupaway
conda activate fq-dpw
fastq-dupaway --help
```

### Manual installation (using conda or system-level boost installation)

The only dependencies are [Boost](https://www.boost.org/) and zlib. This program was developed and tested using Boost libraries version 1.81.0.
<br>
You will also need build tools: g++ compiler version 9 or higher and make.

#### How to install BOOST

<details>
<summary>Click to expand</summary>

In order to install Boost <u>from source</u>, you will need sudo access.<br>
Download source code from official site, configure bootstrap.sh and install:

```bash
export BOOST_ROOT=/usr/local
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${BOOST_ROOT}/lib

wget https://archives.boost.io/release/1.81.0/source/boost_1_81_0.tar.gz
tar xvf boost_1_81_0.tar.gz
cd boost_1_81_0
# The following commands will save boost headers under /usr/local/include/boost
# and compiled boost binaries (needed ones only) under /usr/local/lib
./bootstrap.sh --prefix=${BOOST_ROOT}
sudo ./b2 install --with-iostreams --with-program_options --build-dir=/tmp/build-boost
```

Alternativaly, you can easily install Boost via <u>conda</u>.<br>
You will also need g++ and make installed in your conda environment as well.

```bash
conda create -n gxx-boost -c conda-forge gxx make boost=1.82.0
conda activate gxx-boost
# adjust conda path if you are using conda installation different from Miniconda 3
export BOOST_ROOT=~/miniconda3/envs/gxx-boost
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${BOOST_ROOT}/lib
```

</details>

#### Setting up shell variables

After installing Boost, <ins>set up shell variables</ins>:

* Create shell variable <i>BOOST_ROOT</i> pointing the desired root directory of your boost installation.<br>
Boost installer will place header-only libraries and built shared object files in include/ and lib/ folders under this root directory, respectively.<br>
If you are using conda, assign this variable to your conda env path (see example above in the Boost installation section).

* If your LD_LIBRARY_PATH variable does not contain path to built boost shared object files, add that path to it (you will probably need to set this in your .bashrc file as well for further usage):<br>

```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/your/custom/path/
```

### Clone this repo and build executable

```bash
cd fastq-dupaway
make
make clean
# check that everything went as expected (should print help message and exit):
./fastq-dupaway --help
```

### (Optional) Build using CMake

CMake can help you detect correct Boost installation in case you have multiple on your system.

```bash
cd fastq-dupaway
mkdir build
# if you installed boost using conda, either set BOOST_ROOT variable
# or give cmake a hint where to look using this flag:
# cmake -B build . -DCMAKE_PREFIX_PATH=$CONDA_PREFIX
cmake -B build .
cd build
make
# check that everything went as expected (should print help message and exit):
./fastq-dupaway --help
```


## Usage <a name="usage-main"></a>

**NB**: fastq-dupaway requires **a lot of disk space** (~2 times the input size on average, depends on --mem-limit option value) while running in "sequence-based" mode; that is the cost of limited RAM usage algorithm. The "fast" mode is not disk-intensive and can be used when strict RAM limitation is not in priority.

### Running Docker image <a name="usage-docker"></a>

fastq-dupaway Docker image **creates a folder with temporary files in current working directory** when running. It is highly advised to set container working directory to the externally mounted volume (using `-w` option of `docker run`), otherwise container's filesystem may be overfilled.
This is done deliberately to prevent creating potentially large temporary files in standard tmp space.

Mount volume with your data directories while running docker image:

```bash

docker run -it --rm -v [your data directory]:[workdir-name] -w [workdir-name] fastq-dupaway \
        -i /data/inputs/input.fastq -o /data/outputs/output.fastq <other options>
```

### Program options <a name="usage-options"></a>

```
fastq-dupaway -i INPUT-1 [-u INPUT-2] -o OUTPUT-1 [-p OUTPUT-2] \
        [-m MEMORY-LIMIT] [--format fasta|fastq] \
        ([--compare-seq MODE] | [--fast [--unordered]])
```

The only two required arguments are names of input and output files. If only INPUT-1 and OUTPUT-1 files was provided, the program will treat input as single-ended; If both INPUT-2 and OUTPUT-2 filenames were provided as well, program will treat inputs as paired-ended instead. 

The program supports two main deduplication algorithms, further referred to as "modes":

* a streaming "sequence-based" mode that allows setting an upper limit for memory usage and fine-tuning sequence comparison logic but is also disk-usage-intensive. It is enabled by default.

* a "fast" mode that runs faster and is not limited by rate of disk read/write operations. However, it does not allow managing memory limit and only removes direct duplicates.

Several options can only be used with one of the two modes.<br>
Complete list of options with explanations is listed in the table below.

Option|Value|Mode|Description
---|---|---|---
-h/--help|-|-|Produce help message and exit.
-v/--verbose|-|Both|Report run summary after program execution.
-i/--input-1|string|Both|First input file (required).
-u/--input-2|string|Both|Second input file (optional, enables paired-end mode).
-o/--output-1|string|Both|First output file (required).
-p/--output-2|string|Both|Second output file (required for paired-end mode).
-m/--mem-limit|integer in range [500, 10240]|sequence-based|Memory limit in megabytes (default 2048 = 2Gb).<br>"fast" mode does not support this option.
--format|either "fastq" (default) or "fasta"|Both|Input file format.
--compare-seq|string (see description)|sequence-based|Sequence comparison logic for sequence-based mode.<br>Supported values:<br>- "tight" (default): compare sequences directly, sequences of different lengths are considered different.<br>- "loose":  compare sequences directly, sequences of different lengths are considered duplicates if shorter sequence exactly matches with prefix of longer sequence. Outputs of this mode will be similar to those of "fastuniq" program.<br>- "tail-hamming": An experimental option that considers a pair of sequences as duplicates if those differ by no more than a set number of mismatches at their respective ends. Sequences of different lengths will not be compared.
--distance|non-negative integer|sequence-based (tail-hamming only)|A threshold value for Hamming distance calculation. Default value is 2.
--write-clusters|-|sequence-based|\<Advanced\> Write ids of identified duplicate clusters to a file using id of a preserved read as a name of each cluster.Resulting file is written in addition to main output and is named \<output-file\>.clusters (2 cluster files are written in case of paired mode). 
--fast|-|fast (enables)|Use faster hash-based approach instead of sequence-based. In this mode the program will run significantly faster, however no memory limit can be set and only complete duplicates will be filtered out.
--unordered|-|fast (paired inputs only)|\<Advanced\> Use this flag if reads in your paired input files are not synchronized (i.e. the order in which reads appear (determined by read IDs) and/or the number of reads differs between two input files). If this option is enabled, both input files will be sorted by read IDs before deduplication, and reads with unmatched IDs will be skipped.


## Detailed explanation of program options and algorithms <a name="explanation"></a>

Please refer to the [extended manual](doc/algorithm.md) page.


## Additional information <a name="extra-info"></a>

* Input filenames ending with ".gz" will be treated as binary files compressed by gzip program. If output filenames provided also end with ".gz", output files will be compressed as well.

* The original idea behind this program was to create a tool that would bring the same results as fastuniq program, but would be feasible to run on non-HPC systems (small servers, personal machines) even when dealing with large datasets (hundreds of gigabytes) with reasonable time penalty.

* If you are experienceing unexpected results, first of all check if the last line in your input is terminated with a newline character ('\n'). Absence of newline terminator at the end of input file will affect program behaviour.

## Running tests <a name="tests"></a>

Correctness of the program's algorithm is tested by running a set of tests that can be found in the `test/` folder. In order to run tests, first create a Python virtual environment using venv or conda (recommended Python version is 3.12) and install required packages:

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements-test.txt
```

Then run tests using pytest:

```bash
pytest -v test/
```

## How to cite <a name="cite-info"></a>

To cite fastq-dupaway in publications, please use

<i>Sigorskikh, A.I., Kompaniets, M.A., Ilnitskiy, I.S. et al. Fastq-dupaway: a fast and memory-efficient tool for deduplication of single- and paired-end NGS data. Sci Rep 15, 45303 (2025).</i> [https://doi.org/10.1038/s41598-025-28948-w](https://doi.org/10.1038/s41598-025-28948-w)
