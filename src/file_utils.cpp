#include "file_utils.hpp"
#include "constants.hpp"

#include <boost/format.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>


void FileUtils::_generate_random_name(char* buf, int len)
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

void FileUtils::create_random_dir(char* buf, int len, uint n_tries)
{
    while (true)
    {
        FileUtils::_generate_random_name(buf, len);
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

bool FileUtils::_fileHasExt(const char* filename, const char* ext)
{
    FS::path filePath = filename;
    if (filePath.extension() == ext)
        return true;
    return false;
}


// InputFile classes //

FileUtils::InputFileTXT::InputFileTXT(const char* infilename)
{
    m_infile.open(infilename);
    check_fstream_ok<std::ifstream>(m_infile, infilename);
}

FileUtils::InputFileGZ::InputFileGZ(const char* infilename)
{
    m_infile.open(infilename, std::ios_base::in | std::ios_base::binary);
    check_fstream_ok<std::ifstream>(m_infile, infilename);

    m_instream.push(boost::iostreams::gzip_decompressor());
    m_instream.push(m_infile);
}


// InputFile factory //

FileUtils::I_InputFile* FileUtils::openInputFile(const char* infilename)
{
    if (FileUtils::_fileHasExt(infilename, ".gz"))
    {
        return new FileUtils::InputFileGZ(infilename);
    } else {
        return new FileUtils::InputFileTXT(infilename);
    }
}


// OutputFile classes //

FileUtils::OutputFileTXT::OutputFileTXT(const char* outfilename)
{
    m_outfile.open(outfilename);
    check_fstream_ok<std::ofstream>(m_outfile, outfilename);
}

FileUtils::OutputFileGZ::OutputFileGZ(const char* outfilename)
{
    m_outstream.push(boost::iostreams::gzip_compressor());
    m_outstream.push(boost::iostreams::file_sink(outfilename, std::ofstream::binary));
}


// OutputFile factory //

FileUtils::I_OutputFile* FileUtils::openOutputFile(const char* outfilename)
{
    if (FileUtils::_fileHasExt(outfilename, ".gz"))
    {
        return new FileUtils::OutputFileGZ(outfilename);
    } else {
        return new FileUtils::OutputFileTXT(outfilename);
    }
}


// TemporaryDirectory class //

FileUtils::TemporaryDirectory::TemporaryDirectory()
{
    m_name = (char*)malloc(sizeof(char)*(constants::DIRNAME_LEN + 1));
    m_name[constants::DIRNAME_LEN] = '\0';
    FileUtils::create_random_dir(m_name, constants::DIRNAME_LEN);

    m_sorted1 = (boost::format("%1%/data.sorted1") % m_name).str();
    m_sorted2 = (boost::format("%1%/data.sorted2") % m_name).str();
}

FileUtils::TemporaryDirectory::~TemporaryDirectory()
{
    FS::remove_all(m_name);
    free(m_name);
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
