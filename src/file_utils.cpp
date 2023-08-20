#include "file_utils.hpp"
#include "constants.hpp"

#include <boost/format.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

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

bool FileUtils::_fileHasExt(const char* filename, const char* ext)
{
    FS::path filePath = filename;
    if (filePath.extension() == ext)
        return true;
    return false;
}

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

FileUtils::TemporaryDirectory::TemporaryDirectory()
{
    m_name = (char*)malloc(sizeof(char)*(constants::DIRNAME_LEN + 1));
    m_name[constants::DIRNAME_LEN] = '\0';
    FileUtils::create_random_dir(m_name, constants::DIRNAME_LEN);
}

FileUtils::TemporaryDirectory::~TemporaryDirectory()
{
    FS::remove_all(m_name);
    free(m_name);
}

void FileUtils::TemporaryDirectory::clear_inputs()
{
    std::remove(this->m_input1.c_str());
    std::cout << "called inp file removal!" << std::endl;
    std::cin.get();
    this->m_input1.clear();
    if (this->m_input2.size() > 0)
    {
        std::remove(this->m_input2.c_str());
        this->m_input2.clear();
    }
}

void FileUtils::TemporaryDirectory::set_files(const string& input)
{
    m_input1 = (boost::format("%1%/data_1.in") % m_name).str();
    m_output1 = (boost::format("%1%/data_1.out") % m_name).str();

    if (FileUtils::_fileHasExt(input.c_str()))
    {
        FileUtils::_decompress_gz(input.c_str(), m_input1.c_str());
    } else {
        FS::create_symlink(FS::absolute(input.c_str()), m_input1.c_str());
    }
}

void FileUtils::TemporaryDirectory::set_files(const string& input1,
                                              const string& input2)
{
    this->set_files(input1);

    m_input2 = (boost::format("%1%/data_2.in") % m_name).str();
    m_output2 = (boost::format("%1%/data_2.out") % m_name).str();

    if (FileUtils::_fileHasExt(input2.c_str()))
    {
        FileUtils::_decompress_gz(input2.c_str(), m_input2.c_str());
    } else {
        FS::create_symlink(FS::absolute(input2.c_str()), m_input2.c_str());
    }
}

void FileUtils::TemporaryDirectory::save_output(const string& output)
{
    if (FileUtils::_fileHasExt(output.c_str()))
    {
        FileUtils::_compress_gz(this->m_output1.c_str(), output.c_str());
    } else {
        std::rename(this->m_output1.c_str(), output.c_str());
    }
}

void FileUtils::TemporaryDirectory::save_output(const string& output1,
                                                const string& output2)
{
    this->save_output(output1);

    if (FileUtils::_fileHasExt(output2.c_str()))
    {
        FileUtils::_compress_gz(this->m_output2.c_str(), output2.c_str());
    } else {
        std::rename(this->m_output2.c_str(), output2.c_str());
    }
}
