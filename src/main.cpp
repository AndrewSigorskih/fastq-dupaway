#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "constants.hpp"
#include "comparator.hpp"
#include "fastaview.hpp"
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
    uint hammdist = 2;
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
                                              "Supported value range is [500 <-> 10240 (10 Gb)]\n"
                                              "Actual memory usage may slightly exceed this value.\n"
                                              "NB: The 'fast' deduplication mode does not support strict memory limitation.")
        ("format", po::value<string>(), "input file format: fastq (default) or fasta.")
        ("compare-seq", po::value<string>(), "Sequence comparison mode for deduplication step.\n"
                                             "Supported options:\n"
                                             "\ttight (default): compare sequences directly, sequences of different lengths are considered different.\n"
                                             "\tloose: compare sequences directly, sequences of different lengths are considered duplicates if shorter"
                                             " sequence exactly matches with prefix of longer sequence.\n"
                                             "\ttail-hamming: An experimental option that considers a pair of sequences as duplicates"
                                             " if those differ by no more than a set number of mismatches at their respective ends"
                                             " (default 2). Sequences of different lengths will not be compared.")
        ("distance", po::value<uint>(&opts.hammdist), "A threshold value for 'tail-hamming' distance calculation. Should be a non-negative integer."
                                                      " Default value is 2.")
        ("fast", po::bool_switch(&hash_opt), "Use hash-based approach instead of sequence-based.\n"
                                             "In this mode the program will run significantly faster, however no memory limit can be set"
                                             " and only complete duplicates will be filtered out.")
        ("unordered", po::bool_switch(&opts.unordered), "This option is supported only by 'fast' mode for paired inputs.\n"
                                                        "Enable this flag if reads in your paired input files are not synchronized"
                                                        " (i.e. the reads order determined by read IDs does not match).\n"
                                                        "If this option is enabled, both input files will be sorted by read IDs before deduplication.")
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
            else if (value == "tail-hamming")
                opts.ctype = ComparatorType::CT_HAMMING;
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
            if ((value >= 500L) && (value <= 10240L))
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
        std::cerr << "Unknown error occured during arguments parsing!\n";
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

    try {

        bool paired = static_cast<bool>(opts.mode & Modes::PAIRED);

        FileUtils::TemporaryDirectory tempdir;

        BaseComparator* comp = makeComparator(opts.ctype, paired, opts.hammdist);

        if (opts.mode == Modes::BASE) {
            // seq, single, fastq
            SeqDupRemover<FastqView> remover(opts.memLimit, comp, &tempdir);
            remover.filterSE(opts.input_1, opts.output_1);

        } else if (opts.mode == Modes::FASTA) {
            // seq, single, fasta
            SeqDupRemover<FastaView> remover(opts.memLimit, comp, &tempdir);
            remover.filterSE(opts.input_1, opts.output_1);

        } else if (opts.mode == Modes::PAIRED) {
            // seq, paired, fastq
            SeqDupRemover<FastqView> remover(opts.memLimit, comp, &tempdir);
            remover.filterPE(opts.input_1, opts.input_2,
                             opts.output_1, opts.output_2);

        } else if (opts.mode == (Modes::FASTA | Modes::PAIRED)) {
            // seq, paired, fasta
            SeqDupRemover<FastaView> remover(opts.memLimit, comp, &tempdir);
            remover.filterPE(opts.input_1, opts.input_2,
                             opts.output_1, opts.output_2);

        } else if (opts.mode == Modes::HASH) {
            // hash, single, fastq
            //HashDupRemover<FastqViewWithId> remover(opts.memLimit, &tempdir); // slight optimization
            HashDupRemover<FastqView> remover(opts.memLimit, &tempdir);
            remover.filterSE(opts.input_1, opts.output_1);

        } else if (opts.mode == (Modes::HASH | Modes::FASTA)) {
            // hash, single, fasta
            //HashDupRemover<FastaViewWithId> remover(opts.memLimit, &tempdir); // slight optimization
            HashDupRemover<FastaView> remover(opts.memLimit, &tempdir);
            remover.filterSE(opts.input_1, opts.output_1);

        } else if (opts.mode == (Modes::HASH | Modes::PAIRED)) {
            // hash, paired, fastq
            HashDupRemover<FastqViewWithId> remover(opts.memLimit, &tempdir);
            remover.filterPE(opts.input_1, opts.input_2,
                             opts.output_1, opts.output_2,
                             opts.unordered);

        } else if (opts.mode == (Modes::HASH | Modes::PAIRED | Modes::FASTA)) {
            // hash, paired, fasta
            HashDupRemover<FastaViewWithId> remover(opts.memLimit, &tempdir);
            remover.filterPE(opts.input_1, opts.input_2,
                             opts.output_1, opts.output_2,
                             opts.unordered);
        } else {
            std::cerr << "Unknown mode!\n";
        }

        if (comp)
            delete comp;

    } catch (const std::exception& exc) {
        std::cerr << "An error occured:\n";
        std::cerr << exc.what() << '\n';
        return 1;
    }
    catch(...)
    {
        std::cerr << "Unknown error occured during program execution!\n";
        return 1;
    }

    return 0;
}
