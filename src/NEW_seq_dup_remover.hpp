#pragma once
#include <string>
#include "bufferedinput.hpp"
#include "external_sort.hpp"
#include "file_utils.hpp"
#include "paired_external_sort.hpp"
using std::string;

template<class T>
class SeqDupRemover
{
public:
    SeqDupRemover(ssize_t memlimit) : m_memlimit(memlimit)
    {
        m_tempdir[DIRNAME_LEN] = '\0';
        create_random_dir(m_tempdir, DIRNAME_LEN);
    }
    ~SeqDupRemover() { FS::remove_all(m_tempdir); }
    void filterSE(const string& infile,
                  const string& outfile);
    void filterPE(const string& infile1,
                  const string& infile2,
                  const string& outfile1,
                  const string& outfile2);
private:
    ssize_t m_memlimit;
    char m_tempdir[DIRNAME_LEN + 1];
};

template<class T>
void SeqDupRemover<T>::filterSE(const string& infile,
                                const string& outfile)
{
    string infilename = infile;
    // TODO gzipped output? deal with it HERE
    string sorted = (boost::format("%1%/data.sorted") % m_tempdir).str();
    string filtered = (boost::format("%1%/data.out") % m_tempdir).str();
    {   // sort input file in a scope so all buffers deallocate
        ExternalSorter<T> sorter(m_memlimit);
        sorter.sort(infilename.c_str(), sorted.c_str());
    }
    // deduplicate file

}
