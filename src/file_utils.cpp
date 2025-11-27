#include "file_utils.hpp"
#include "constants.hpp"

#include <boost/format.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/copy.hpp>

namespace FileUtils
{

    void _generate_random_name(char* buf, int len)
    {
        static const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        static const size_t max_index = (sizeof(charset) - 2); // skipping \0 at the end
        std::default_random_engine rng(std::random_device{}());
        std::uniform_int_distribution<> dist(0, max_index);
        auto randchar = [&]() -> char { return charset[ dist(rng) ]; };
        std::generate_n( buf, len, randchar );
    }

    void create_random_dir(char* buf, int len, uint n_tries)
    {
        while (true)
        {
            _generate_random_name(buf, len);
            if (FS::create_directory(buf)) 
                break;
            n_tries--;
            if (!n_tries)
            {
                std::cerr << "Could not create directory with randomly-generated name in " << n_tries << " tries!" << std::endl;
                throw std::runtime_error("Number of tries exhausted.");
            }
        }
    }

    bool _fileHasExt(const char* filename, const char* ext)
    {
        FS::path filePath = filename;
        if (filePath.extension() == ext)
            return true;
        return false;
    }


// InputFile classes //

    InputFileTXT::InputFileTXT(const char* infilename)
    {
        m_infile.open(infilename);
        check_fstream_ok<std::ifstream>(m_infile, infilename);
    }

    InputFileGZ::InputFileGZ(const char* infilename)
    {
        m_infile.open(infilename, std::ios_base::in | std::ios_base::binary);
        check_fstream_ok<std::ifstream>(m_infile, infilename);

        m_instream.push(boost::iostreams::gzip_decompressor());
        m_instream.push(m_infile);
    }


// InputFile factory //

    I_InputFile* openInputFile(const char* infilename)
    {
        if (_fileHasExt(infilename, ".gz"))
        {
            return new InputFileGZ(infilename);
        } else {
            return new InputFileTXT(infilename);
        }
    }


// OutputFile classes //

    OutputFileTXT::OutputFileTXT(const char* outfilename)
    {
        m_outfile.open(outfilename);
        check_fstream_ok<std::ofstream>(m_outfile, outfilename);
    }

    OutputFileGZ::OutputFileGZ(const char* outfilename)
    {
        m_outstream.push(boost::iostreams::gzip_compressor());
        m_outstream.push(boost::iostreams::file_sink(outfilename, std::ofstream::binary));
    }


// OutputFile factory //

    I_OutputFile* openOutputFile(const char* outfilename)
    {
        if (_fileHasExt(outfilename, ".gz"))
        {
            return new OutputFileGZ(outfilename);
        } else {
            return new OutputFileTXT(outfilename);
        }
    }

// UniversalOutputFile class
    UniversalOutputFile::UniversalOutputFile(const char* outfilename)
    {
        if (_fileHasExt(outfilename, ".gz"))
        {
            m_outstream.push(boost::iostreams::gzip_compressor());
            m_outstream.push(boost::iostreams::file_sink(outfilename, std::ofstream::binary));
        } else {
            m_outstream.push(boost::iostreams::file_sink(outfilename));
        }
    }


// ClusterFile class

    // opens filepath <base_filename>.clusters for write
    void ClusterFile::open(const char* base_filename)
    {
        m_file.open((boost::format("%1%.clusters") % base_filename).str());
    }

    void ClusterFile::write_cluster_head(const char* start, ssize_t n)
    {
        m_file.write(start, n);
    }

    void ClusterFile::write_cluster_item(const char* start, ssize_t n)
    {
        m_file.write("--", 2);
        m_file.write(start, n);
    }

// TemporaryDirectory class //

    TemporaryDirectory::TemporaryDirectory()
    {
        m_name = (char*)malloc(sizeof(char)*(constants::DIRNAME_LEN + 1));
        m_name[constants::DIRNAME_LEN] = '\0';
        create_random_dir(m_name, constants::DIRNAME_LEN);

        m_sorted1 = (boost::format("%1%/data.sorted1") % m_name).str();
        m_sorted2 = (boost::format("%1%/data.sorted2") % m_name).str();
    }

    TemporaryDirectory::~TemporaryDirectory()
    {
        FS::remove_all(m_name);
        free(m_name);
    }

}
// These functions were used before but are no longer needed //

[[deprecated]]
void FileUtils::_decompress_gz(const char* infilename, const char* outfilename)
{
    std::ifstream infile(infilename, std::ios_base::in | std::ios_base::binary);
    std::ofstream outfile(outfilename);
    check_fstream_ok<std::ifstream>(infile, infilename);
    check_fstream_ok<std::ofstream>(outfile, outfilename);

    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    inbuf.push(boost::iostreams::gzip_decompressor());
    inbuf.push(infile);
    boost::iostreams::copy(inbuf, outfile);
}

[[deprecated]]
void FileUtils::_compress_gz(const char* infilename, const char* outfilename)
{
    std::ifstream infile(infilename);
    std::ofstream outfile(outfilename, std::ios_base::out | std::ios_base::binary);
    check_fstream_ok<std::ifstream>(infile, infilename);
    check_fstream_ok<std::ofstream>(outfile, outfilename);

    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    inbuf.push(boost::iostreams::gzip_compressor());
    inbuf.push(infile);
    boost::iostreams::copy(inbuf, outfile);

    boost::iostreams::close(inbuf);
}

[[deprecated]]
void FileUtils::_move_file_smart(const char* infilename, const char* outfilename)
{
    try {
        // cannot "rename" files between different mounts/filesystems
        // if files are on a same mount we can hardlink one to another, otherwise get error
        // trying to std::rename file between different mounts does not throw any error for some reason
        // probably just can create hardlink and remove infile here but I went a bit safer
        FS::create_hard_link(infilename, outfilename);
        std::remove(outfilename);
        std::rename(infilename, outfilename);
    }
    catch (const FS::filesystem_error& e){
        // cannot rename file -> just copy it between filesystems
        std::ifstream infile(infilename);
        std::ofstream outfile(outfilename);
        check_fstream_ok<std::ifstream>(infile, infilename);
        check_fstream_ok<std::ofstream>(outfile, outfilename);

        outfile << infile.rdbuf();

        infile.close(); outfile.close();
        std::remove(infilename);
    }
}
