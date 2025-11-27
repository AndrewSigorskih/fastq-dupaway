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
    void create_random_dir(char* buf, int len, uint n_tries = 100);
    bool _fileHasExt(const char* filename, const char* ext=".gz");
    [[deprecated]] void _decompress_gz(const char* infilename, const char* outfilename);
    [[deprecated]] void _compress_gz(const char* infilename, const char* outfilename);
    [[deprecated]] void _move_file_smart(const char* infilename, const char* outfilename);
    
    // InputFile abstraction
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

    // OutputFile abstraction
    class I_OutputFile
    {
    public:
        virtual ~I_OutputFile() {}
        virtual void write(const char* start, std::streamsize n) = 0;
    };

    class OutputFileTXT : public I_OutputFile
    {
    public:
        OutputFileTXT(const char* outfilename);
        ~OutputFileTXT()                                    { m_outfile.close(); }
        void write(const char* start, std::streamsize n)    { m_outfile.write(start, n); }
    private:
        std::ofstream m_outfile;
    };

    class OutputFileGZ : public I_OutputFile
    {
    public:
        OutputFileGZ(const char* outfilename);
        ~OutputFileGZ()                                     { }
        void write(const char* start, std::streamsize n)    { m_outstream.write(start, n); }
    private:
        boost::iostreams::filtering_ostream m_outstream;
    };

    // OutputFile factory
    I_OutputFile* openOutputFile(const char* outfilename);

    // Test the non-polymorphic class for outputfile TODO remove this comment
    class UniversalOutputFile
    {
    public:
        UniversalOutputFile(const char* outfilename);
        ~UniversalOutputFile()                              { }
        void write(const char* start, std::streamsize n)    { m_outstream.write(start, n); }
    private:
        boost::iostreams::filtering_ostream m_outstream;
    };

    // File for storing clusters of duplicated reads
    class ClusterFile
    {
    public:
        ClusterFile() {}
        ~ClusterFile() { if (m_file.is_open()) { m_file.close(); } }
        void open(const char* base_filename);
        void write_cluster_head(const char* start, ssize_t n);
        void write_cluster_item(const char* start, ssize_t n);
    private:
        std::ofstream m_file;
    };


    // Directory with randomly-generated name
    class TemporaryDirectory
    {
    public:
        TemporaryDirectory();
        ~TemporaryDirectory();
        inline const char* name()           const   { return m_name;    }
        inline const string& sorted_left()  const   { return m_sorted1; }
        inline const string& sorted_right() const   { return m_sorted2; }
    private:
        char* m_name;
        string m_sorted1;
        string m_sorted2;
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
