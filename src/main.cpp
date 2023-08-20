#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "constants.hpp"
#include "comparator.hpp"
#include "fastqview.hpp"
#include "seq_dup_remover.hpp"
#include "hash_dup_remover.hpp"

using std::string;
namespace po = boost::program_options;

enum Modes
{
    BASE = 0,   // Base case: single-end Fastq file, seq-based approach
    FASTA = 1,  // change format to Fasta
    PAIRED = 2, // paired-end input
    HASH = 4    // use hash-based approach instead of seq comparison
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
    ComparatorType ctype = ComparatorType::CT_TIGHT;
    bool unordered = false;
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
                                              "NB: The hash-based deduplication mode does not support strict memory limitation.")
        ("format", po::value<string>(), "input file format: fastq (default) or fasta.")
        ("compare-seq", po::value<string>(), "Sequence comparison mode for deduplication step.\n"
                                             "Supported options:\n"
                                             "\ttight (default): compare sequences directly, sequences of different lengths are considered different.\n"
                                             "\tloose: compare sequences directly, sequences of different lengths are considered duplicates if shorter"
                                             " sequence exactly matches with prefix of longer sequence.")
        ("hashed", po::bool_switch(&hash_opt), "Use hash-based approach instead of sequence-based.\n"
                                               "With this mode the program will run significantly faster, however no memory limit can be set"
                                               " and only complete duplicates will be filtered out.")
        ("unordered", po::bool_switch(&opts.unordered), "This option is supported only by hash mode for paired inputs.\n"
                                                        "Enable this flag if reads in your paired input files are not synchronized"
                                                        " (i.e. the reads order determined by read IDs does not match).\n"
                                                        "If this option is enabled, the both input files will be sorted by read IDs before deduplication.")
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
        { throw std::runtime_error("Both input-2 and output-2 arguments are required for paired-end mode!"); }

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
                throw std::runtime_error("Only \"fastq\" or \"fasta\" file formats are supported!");
        }

        // comparator type
        if (vm.count("compare-seq"))
        {
            string value = vm["compare-seq"].as<string>();
            if (value == "tight")
                ;
            else if (value == "loose")
                opts.ctype = ComparatorType::CT_LOOSE;
            else
                throw std::runtime_error("Unsupported compare-seq type provided!");
        }

        // seq-based or hash-based
        if (hash_opt)
        {
            opts.mode = (opts.mode | Modes::HASH);
            opts.ctype = ComparatorType::CT_NONE;
        }

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

    // actual logic
    // TODO: implement fasta support
    // TODO: implement gzipped files support : create tmp dir here and gunzip files to 
    // it before everythong else?

    try {

        bool paired = static_cast<bool>(opts.mode & Modes::PAIRED); // TODO is it actually needed?

        FileUtils::TemporaryDirectory tempdir;

        // verbose info, will be deleted
        std::cout << "Input file(s): " << opts.input_1;
        if (paired)
            std::cout << " and " << opts.input_2;
        std::cout << '\n';

        std::cout << "Output file(s): " << opts.output_1;
        if (paired)
            std::cout << " and " << opts.output_2;
        std::cout << '\n';

        std:: cout << "mem limit: " << opts.memLimit << '\n';
        // end of verbose info

        BaseComparator* comp = makeComparator(opts.ctype, paired);

        if (opts.mode == Modes::BASE) {
            std:: cout << "seq, single, fastq\n";
            SeqDupRemover<FastqView> remover(opts.memLimit, comp, &tempdir);
            remover.filterSE(opts.input_1, opts.output_1);

        } else if (opts.mode == Modes::FASTA) {
            std::cout << "seq, single, fasta\n";
            std::cerr << "This mode is not properly implemented yet!\n";

        } else if (opts.mode == Modes::PAIRED) {
            std:: cout << "seq, paired, fastq\n";
            SeqDupRemover<FastqView> remover(opts.memLimit, comp, &tempdir);
            remover.filterPE(opts.input_1, opts.input_2,
                             opts.output_1, opts.output_2);

        } else if (opts.mode == (Modes::FASTA | Modes::PAIRED)) {
            std::cout << "seq, paired, fasta\n";
            std::cerr << "This mode is not properly implemented yet!\n";

        } else if (opts.mode == Modes::HASH) {
            std:: cout << "hash, single, fastq\n";
            HashDupRemover<FastqViewWithId> remover(opts.memLimit, &tempdir);
            remover.filterSE(opts.input_1, opts.output_1);

        } else if (opts.mode == (Modes::HASH | Modes::FASTA)) {
            std::cout << "hash, single, fasta\n";
            std::cerr << "This mode is not properly implemented yet!\n";

        } else if (opts.mode == (Modes::HASH | Modes::PAIRED)) {
            std::cout << "hash, paired, fastq\n";
            HashDupRemover<FastqViewWithId> remover(opts.memLimit, &tempdir);
            remover.filterPE(opts.input_1, opts.input_2,
                             opts.output_1, opts.output_2,
                             opts.unordered);

        } else if (opts.mode == (Modes::HASH | Modes::PAIRED | Modes::FASTA)) {
            std::cout << "hash, paired, fasta\n";
            std::cerr << "This mode is not properly implemented yet!\n";

        } else {
            std::cerr << "Unknown mode!\n";
        }

        if (comp)
            delete comp;

    } catch (const std::exception& exc) {
        std::cerr << "An error occured:\n";
        std::cerr << exc.what() << '\n';
    }

    return 0;
}