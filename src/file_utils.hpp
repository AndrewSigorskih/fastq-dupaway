#pragma once
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

using std::string;
namespace FS = std::filesystem;

namespace FileUtils
{
    void _generate_random_name(char* buf, int len);
    bool _fileHasExt(const char* filename, const char* ext=".gz");
    void _decompress_gz(const char*, const char*);
    void _compress_gz(const char*, const char*);
    void create_random_dir(char* buf, int len, uint n_tries = 100);

    class TemporaryDirectory
    {
    public:
        TemporaryDirectory();
        ~TemporaryDirectory();
        inline const char* name() const { return this->m_name; }
        inline const string& input1() const { return this->m_input1; }
        inline const string& input2() const { return this->m_input2; }
        inline const string& output1() const { return this->m_output1; }
        inline const string& output2() const { return this->m_output2; }
        void set_files(const string&);
        void set_files(const string&, const string&);
        void save_output(const string&);
        void save_output(const string&, const string&);
    private:
        char* m_name;
        string m_input1, m_input2, m_output1, m_output2;
    };
}

template<class filehandle>
void check_fstream_ok(filehandle& stream, const char* filename)
{
    if (!stream)
    {
        std::cerr << "Cannot open file ";
        std::cerr.write(filename, strlen(filename));
        std::cerr << std::endl;
        throw std::runtime_error("File does not exist or cannot be opened!");
    }
}
