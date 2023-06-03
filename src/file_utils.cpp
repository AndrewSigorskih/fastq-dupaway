#include "file_utils.hpp"

void generate_random_name(char* buf, int len)
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
        generate_random_name(buf, len);
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

void create_random_file(char* buf, int len, uint n_tries)
{
    while (true)
    {
        generate_random_name(buf, len);
        if (!FS::exists(buf))
        {   // just create empty file
            std::ofstream file{buf};
            break;
        }
        n_tries--;
        if (!n_tries)
        {
            std::cerr << "Could not create file with randomly-generated name in " << n_tries << " tries!" << std::endl;
            throw std::runtime_error("Number of tries exhausted.");
        }
    }
}

uint64_t countLines(const char* infilename)
{
    std::ifstream input{infilename};
    check_fstream_ok(input, infilename);
    uint64_t linesCount = 0;
    input.unsetf(std::ios_base::skipws);
    linesCount = std::count(
                    std::istream_iterator<char>(input),
                    std::istream_iterator<char>(), 
                    '\n');
    return linesCount;
}

bool fileHasExt(const char* filename, 
                const char* ext)
{
    FS::path filePath = filename;
    if (filePath.extension() == ext)
        return true;
    return false;
}