#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "constants.h"
#include "comparator.hpp"
#include "fastqview.hpp"
#include "seq_dup_remover.hpp"

//#include "fastq.hpp"
//#include "hash_dup_remover.hpp"

namespace po = boost::program_options;
using std::string;

class InvalidFileFormatException : public std::exception 
{
public:
    const char * what () const throw() 
    { return "Only \"fastq\" or \"fasta\" file formats are supported!"; }
};

class InvalidPEArgs : public std::exception 
{
public:
    const char * what () const throw() 
    { return "Both input-2 and output-2 arguments are required for paired-end mode!"; }
};

enum Modes
{
    BASE = 0,
    FASTA = 1,
    PAIRED = 2,
    HASH = 4
};

inline Modes operator | (Modes a, Modes b)
{
    return static_cast<Modes>(static_cast<int>(a) | static_cast<int>(b));
}

struct Options
{
    Modes mode = Modes::BASE;
    ssize_t memLimit = constants::TWO_GB;
    string input_1, input_2, output_1, output_2;
};

bool parse_args(int argc, char** argv, Options& opts)
{
    try {
        // Configure options here
        bool hash_opt = false;
        po::options_description desc ("Supported options");
        desc.add_options()
        ("help,h", "Produce help message and exit")
        ("input-1,i", po::value<string>(&opts.input_1)->required(), "First input file (required)")
        ("input-2,u", po::value<string>(&opts.input_2), "Second input file (optional, enables paired-end mode)")
        ("output-1,o", po::value<string>(&opts.output_1)->required(), "First output file (required)")
        ("output-2,p", po::value<string>(&opts.output_2), "Second output file (optional, required for paired-end mode)")
        ("mem-limit,m", po::value<ssize_t>(), "Memory limit in megabytes (default 2048 = 2Gb).\n"
                                              "Supported value range is [100 <-> 10240 (10 Gb)]\n"
                                              "Actual memory usage will slightly exceed this value.\n"
                                              "The hashtable-based deduplication mode does not support strict memory limitation,\n"
                                              "but will most likely not exceed upper bound.")
        ("format", po::value<string>(), "input file format: fastq (default) or fasta")
        ("hashed", po::bool_switch(&hash_opt), "[This option is subject to change soon] Use hash-based approach instead of sequence-based.")
        ;
        // Parse command line arguments
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            std::cerr << desc << "\n";
            return false;
        }
        po::notify(vm);

        // check whether PE mode args passed correctly
        if (vm.count("input-2") ^ vm.count("output-2"))
        { throw InvalidPEArgs(); }

        // paired or single mode
        if (vm.count("input-2"))
        { opts.mode  = (opts.mode | Modes::PAIRED); }

        // file format check
        if (vm.count("format"))
        {
            string value = vm["format"].as<string>();
            if (value == "fastq")
                ;
            else if (value == "fasta")
                opts.mode = (opts.mode | Modes::FASTA);
            else
                throw InvalidFileFormatException(); // TODO better custom exceptions
        }

        // seq-based or hash-based
        if (hash_opt)
        { opts.mode = (opts.mode | Modes::HASH); }

        // memory limit safe check
        if (vm.count("mem-limit"))
        {
            ssize_t value = vm["mem-limit"].as<ssize_t>();
            if ((value >= 100L) && (value <= 10240L))
                opts.memLimit = value * constants::ONE_MB;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "An error occured:\n";
        std::cerr << e.what() << '\n';
        return false;
    }
    catch(...)
    {
        std::cerr << "Unknown error!" << '\n';
        return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    Options opts;
    bool result = parse_args(argc, argv, opts);
    if (!result)
        return 1;
    
    // verbose info, will be deleted
    std::cout << "Input file(s): " << opts.input_1;
    if (opts.input_2.size())
        std::cout << " and " << opts.input_2;
    std::cout << '\n';

    std::cout << "Output file(s): " << opts.output_1;
    if (opts.output_2.size())
        std::cout << " and " << opts.output_2;
    std::cout << '\n';

    std:: cout << "mem limit: " << opts.memLimit << '\n';
    // actual logic
    // TODO switch all exits to exceptions, implement custom message exceptions
    // TODO finish hash-based dup remover and id-sorted object-view classes
    // TODO: implement fasta support
    // TODO: implement gzipped files support

    Comparator* comp = nullptr;

    try {
        if (opts.mode == Modes::BASE) {
            std:: cout << "seq, single, fastq\n";
        } else if (opts.mode == Modes::FASTA) {
            std::cout << "seq, single, fasta\n";
        } else if (opts.mode == Modes::PAIRED) {
            std:: cout << "seq, paired, fastq\n";
        } else if (opts.mode == (Modes::FASTA | Modes::PAIRED)) {
            std::cout << "seq, paired, fasta\n";
        } else if (opts.mode == Modes::HASH) {
            std:: cout << "hash, single, fastq\n";
        } else if (opts.mode == (Modes::HASH | Modes::FASTA)) {
            std::cout << "hash, single, fasta\n";
        } else if (opts.mode == (Modes::HASH | Modes::PAIRED)) {
            std::cout << "hash, paired, fastq\n";
        } else if (opts.mode == (Modes::HASH | Modes::PAIRED | Modes::FASTA)) {
            std::cout << "hash, paired, fasta\n";
        } else {
            std::cerr << "Unknown mode!!!\n";
        }
    } catch (const std::exception& exc) {
        std::cerr << "An error occured:\n";
        std::cerr << exc.what() << '\n';
    }

    if (comp)
        delete comp;
    
    return 0;
}