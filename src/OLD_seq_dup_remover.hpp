#pragma once
#include <string>

#include "file_utils.hpp"
#include "external_sort.hpp"

using std::string;

template<class T, int LinesPerObj>
class SeqDupRemover
{
public:
    SeqDupRemover(uint64_t memLimit) : m_memLimit(memLimit)
    {
        m_tempdir[DIRNAME_LEN] = '\0';
        create_random_dir(m_tempdir, DIRNAME_LEN);
    }
    void FilterSE(const string& infile,
                  const string& outfile);
    void FilterPE(const string& infile1,
                  const string& infile2,
                  const string& outfile1,
                  const string& outfile2);
    ~SeqDupRemover()
    {
        FS::remove_all(m_tempdir);
        std::cout << "destructor\n";
    }

private:
    char m_tempdir[DIRNAME_LEN + 1];
    uint64_t m_objCount, m_memLimit;
private:
    void seqFilterSE(const char* infile,
                     const char* outfile);
    void seqFilterPE(const char* infile1,
                     const char* infile2,
                     const char* outfile1,
                     const char* outfile2);
};

template<class T, int LinesPerObj>
void SeqDupRemover<T, LinesPerObj>::FilterSE(const string& infile,
                                             const string& outfile)
{
    string infilename = infile;
    // TODO gzipped output? deal with it HERE
    uint64_t linesNum = countLines(infilename.c_str());
    m_objCount = (linesNum / LinesPerObj);
    // sort file
    string tmp = (boost::format("%1%/data.sorted") % m_tempdir).str();
    uint64_t fileSize = static_cast<uint64_t>(
                            FS::file_size(infilename.c_str()));
    
    uint64_t bucketSize = (fileSize > m_memLimit) \
                            ? (m_objCount / (fileSize / m_memLimit + 1)) \
                            :  m_objCount;
    ExternalSorter<T, LinesPerObj> sorter(bucketSize, linesNum);
    sorter(infilename1.c_str(), tmp.c_str());
    infilename = std::move(tmp);
    // filter file
    string outfilename = (boost::format("%1%/data.out") % m_tempdir).str();
    seqFilterSE(infilename.c_str(), outfilename.c_str());
    // TODO gzip if output is gz else 
    std::rename(outfilename.c_str(), outfile.c_str());
}

template<class T, int LinesPerObj>
void SeqDupRemover<T, LinesPerObj>::seqFilterSE(const char* infile,
                                                const char* outfile)
{
    std::ifstream if1{infile};
    std::ofstream of1{outfile};
    check_fstream_ok<std::ifstream>(if1, infile);
    check_fstream_ok<std::ofstream>(of1, outfile);

    T obj;
    string previous;

    while (!if1.eof())
    {
        if1 >> obj >> std::ws;
            
        if (obj.seq() != previous)
        {
            previous = obj.seq();
            of1 << obj << '\n';
        }
    }
}