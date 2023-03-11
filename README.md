# fastq-dupaway

fastq-dupaway is a program for memory-efficient deduplication of single-end and paired-end FASTQ and FASTA files (both plain-text and gzip-compressed).<br>
The main advantage is two duplication-removing algoritms implemented:
* By default, the hashtable-based approach is used. This mode removes only exact duplicates, however it can easily work with situations when reads in paired-end files are not "perfectly synchronised", i.e. the order and amount of reads in first file do not match those of the second file;
* The sequence-based approach requires paired-end files to be in perfect sync, however [work in progress] this mode allows user to specify number of mismatches N; pairs of reads that do not differ by more than N mismatches from any other pair in dataset will be considered duplicated and thus removed.


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
Download source code from oficial site, configure bootstrap.sh and install:
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
fastq-dupaway -i INPUT-1 [-u INPUT-2] -o OUTPUT-1 [-p OUTPUT-2] [-m MEMORY-LIMIT] [--format fasta|fastq] [--no-sort]
```

The only two required arguments are names of input and output files. If only one pair of files was provided, program will run in single-end mode; If both seconda input and second output filenames were provided, program will run in paired-end mode instead. Complete list of options with explanations is listed below:

```
  -h [ --help ]          Produce help message and exit
  -i [ --input-1 ] arg   First input file (required)
  -u [ --input-2 ] arg   Second input file (optional, enables paired-end mode)
  -o [ --output-1 ] arg  First output file (required)
  -p [ --output-2 ] arg  Second output file (optional, required for paired-end 
                         mode)
  -m [ --mem-limit ] arg Memory limit in kilobytes for sorting (default 2000000
                         ~ 2Gb).
                         Values less than 1000000 or greater than 10000000 will
                         be discarded as unrealistic.
                         Note that actual memory usage for default 
                         hashtable-based deduplication step may exceed this 
                         value.
  --format arg           input file format: fastq (default) or fasta
  --no-sort              Do not sort input files by id; this option is for hashtable-based
                         mode only.
```

## Additional information

Input filenames ending with ".gz" will be treated as binary files compressed by gzip program. If output filenames provided also end with ".gz", output files will be compressed as well.
