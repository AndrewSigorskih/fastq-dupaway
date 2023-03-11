#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "fastq.hpp"
#include "dup_remover.hpp"

enum class fileFormat: int { fastq, fasta };
namespace po = boost::program_options;
using std::string;

class InvalidFileFormatException : public std::exception 
{
public:
    const char * what () const throw() 
    { return "only \"fastq\" or \"fasta\" file formats are supported!"; }
};

class InvalidPEArgs : public std::exception 
{
public:
    const char * what () const throw() 
    { return "both input-2 and output-2 arguments are required for paired-end mode!"; }
};

struct Options
{
    bool no_sort=false, synced=false;
    fileFormat format = fileFormat::fastq;
    uint64_t memLimit = 2000000000ul;
    string input_1, input_2, output_1, output_2;
};

bool parse_args(int argc, char** argv, Options& opts)
{
    try
    {
        // Configure options here
        po::options_description desc ("Supported options");
        desc.add_options()
        ("help,h", "Produce help message and exit")
        ("input-1,i", po::value<string>(&opts.input_1)->required(), "First input file (required)")
        ("input-2,u", po::value<string>(&opts.input_2), "Second input file (optional, enables paired-end mode)")
        ("output-1,o", po::value<string>(&opts.output_1)->required(), "First output file (required)")
        ("output-2,p", po::value<string>(&opts.output_2), "Second output file (optional, required for paired-end mode)")
        ("mem-limit,m", po::value<int64_t>(), "Memory limit in kilobytes for sorting (default 2000000 ~ 2Gb).\n"
                                               "Values less than 1000000 or greater than 10000000 will be discarded as unrealistic.\n"
                                               "Note that actual memory usage for default hashtable-based deduplication step may exceed this value.")
        ("format", po::value<string>(), "input file format: fastq (default) or fasta")
        ("no-sort", po::bool_switch(&opts.no_sort), "Do not sort input files by id; this option is ignored if \"synced\" mode is on")
        ("synced", po::bool_switch(&opts.synced), "[This option is subject to change soon] In paired-end mode, assume reads in input files are synchronized by IDs")
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

        // file format check
        if (vm.count("format"))
        {
            string value = vm["format"].as<string>();
            if (value == "fastq")
                ;
            else if (value == "fasta")
                opts.format = fileFormat::fasta;
            else
                throw InvalidFileFormatException();
        }

        // memory limit safe check
        if (vm.count("mem-limit"))
        {
            int64_t value = vm["mem-limit"].as<int64_t>();
            if ((value >= 1000000l) && (value <= 10000000l))
                opts.memLimit = static_cast<uint64_t>(value) * 1000l;
        }
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
    catch(...)
    {
        std::cerr << "Unknown error!" << "\n";
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
    
    // verbose info
    std::cout << "Input file(s): " << opts.input_1;
    if (opts.input_2.size())
        std::cout << " and " << opts.input_2;
    std::cout << '\n';

    std::cout << "Output file(s): " << opts.output_1;
    if (opts.output_2.size())
        std::cout << " and " << opts.output_2;
    std::cout << '\n';

    std::cout <<"no-sort: " << opts.no_sort << '\n';

    switch (opts.format)
    {
        case fileFormat::fastq:
            std::cout << "fastq\n";
            break;
        case fileFormat::fasta:
            std::cout << "fasta\n";
            break;
        default:
            std::cerr << "WRONG FILEFORMAT\n";
    }

    std:: cout << "mem limit: " << opts.memLimit << '\n';
    // actual logic
    // TODO: implement seq dup remover as separate class
    // TODO: implement fasta support
    // TODO: implement gzipped files support
    switch (opts.format)
    {
        case fileFormat::fastq:
        {
            DupRemover<FastQEntry, 4> deduper(opts.no_sort, opts.synced, opts.memLimit);
            // SE or PE
            if (opts.input_2.size()) // paired inputs provided -> PE mode
            {
                deduper.FilterPE(
                    opts.input_1,
                    opts.input_2,
                    opts.output_1,
                    opts.output_2);
            } else { // SE mode
                deduper.FilterSE(opts.input_1, opts.output_1);
            }
            break;
        }
        case fileFormat::fasta:
        {
            std::cerr << "fasta file format not implemented yet!\n";
            break;
        }
        default:
            break;
    }
    return 0;
}