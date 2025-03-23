#pragma once
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

#include <boost/iostreams/filtering_stream.hpp>

using std::string;
namespace FS = std::filesystem;

namespace FileUtils
{
    void _generate_random_name(char* buf, int len);
    bool _fileHasExt(const char* filename, const char* ext=".gz");
    void _decompress_gz(const char* infilename, const char* outfilename);
    void _compress_gz(const char* infilename, const char* outfilename);
    void _move_file_smart(const char* infilename, const char* outfilename);
    void create_random_dir(char* buf, int len, uint n_tries = 100);

    class I_InputFile
    {
    public:
        virtual ~I_InputFile() {}
        virtual bool eof() const = 0;
        virtual std::streamsize gcount() const = 0;
        virtual void read(char* arr, std::streamsize n) = 0;
    };

    class InputFileTXT : public I_InputFile
    {
    public:
        InputFileTXT(const char* infilename);
        ~InputFileTXT()                             { m_infile.close();         }
        bool eof() const                            { return m_infile.eof();    }
        std::streamsize gcount() const              { return m_infile.gcount(); }
        void read(char* arr, std::streamsize n)     { m_infile.read(arr, n);    }
    private:
        std::ifstream m_infile;
    };

    class InputFileGZ : public I_InputFile
    {
    public:
        InputFileGZ(const char* infilename);
        ~InputFileGZ()                             { m_infile.close();           }
        bool eof() const                           { return m_instream.eof();    }
        std::streamsize gcount() const             { return m_instream.gcount(); }
        void read(char* arr, std::streamsize n)    { m_instream.read(arr, n);    }
    private:
        std::ifstream m_infile;
        boost::iostreams::filtering_istream m_instream;
    };

    // InputFile factory
    I_InputFile* openInputFile(const char* infilename);


    class TemporaryDirectory
    {
    public:
        TemporaryDirectory();
        ~TemporaryDirectory();
        inline const char* name()       const  { return m_name;    }
        inline const string& input1()   const  { return *m_input1;  }
        inline const string& input2()   const  { return *m_input2;  }
        inline const string& output1()  const  { return m_output1; }
        inline const string& output2()  const  { return m_output2; }
        void clear_inputs();
        void set_files(const string&);
        void set_files(const string&, const string&);
        void save_output(const string&);
        void save_output(const string&, const string&);
    private:
        char* m_name;
        string const* m_input1;
        string const* m_input2;
        string  m_output1, m_output2;
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
