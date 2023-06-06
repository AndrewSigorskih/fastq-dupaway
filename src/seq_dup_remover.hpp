#pragma once
#include <string>
#include "bufferedinput.hpp"
#include "comparator.hpp"
#include "external_sort.hpp"
#include "file_utils.hpp"
#include "paired_external_sort.hpp"
using std::string;

template<class T>
class SeqDupRemover
{
public:
    SeqDupRemover(ssize_t memlimit, Comparator* comparator) : m_memlimit(memlimit)
    {
        m_comparator = comparator;
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
    void impl_filterSE(const char* infile,
                       const char* outfile);
    void impl_filterPE(const char* infile1,
                       const char* infile2,
                       const char* outfile1,
                       const char* outfile2);
private:
    ssize_t m_memlimit;
    Comparator* m_comparator;
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
    this->impl_filterSE(sorted.c_str(), filtered.c_str());
    // TODO gzip if output is gz else
    std::rename(filtered.c_str(), outfile.c_str());
}

template<class T>
void SeqDupRemover<T>::impl_filterSE(const char* infile,
                                     const char* outfile)
{
    std::ifstream input{infile};
    std::ofstream output{outfile};
    check_fstream_ok<std::ifstream>(input, infile);
    check_fstream_ok<std::ofstream>(output, outfile);

    T obj;
    BufferedInput<T> buffer(m_memlimit);
    buffer.set_file(&input);
    obj = buffer.next();
    this->m_comparator->set_seq(obj.seq(), obj.seq_len());
    output << obj;

    while (!buffer.eof())
    {
        while (!buffer.block_end())
        {
            obj = buffer.next();
            if (!(this->m_comparator->compare(obj.seq(), obj.seq_len())))
            {  // compare returns false -> seqs are different
                this->m_comparator->set_seq(obj.seq(), obj.seq_len());
                output << obj;
            }
        }
        buffer.refresh();
    }
}

template<class T>
void SeqDupRemover<T>::filterPE(const string& infile1,
                                const string& infile2,
                                const string& outfile1,
                                const string& outfile2)
{
    string infilename1 = infile1, infilename2 = infile2;
    // TODO deal with gzipped input here
    string sorted1 = (boost::format("%1%/data.sorted1") % m_tempdir).str();
    string sorted2 = (boost::format("%1%/data.sorted2") % m_tempdir).str();
    string filtered1 = (boost::format("%1%/data.out1") % m_tempdir).str();
    string filtered2 = (boost::format("%1%/data.out2") % m_tempdir).str();
    {
        PairedExternalSorter<T> sorter(m_memlimit);
        sorter.sort(infilename1.c_str(), infilename2.c_str(),
                    sorted1.c_str(), sorted2.c_str());
    }
    this->impl_filterPE(sorted1.c_str(), sorted2.c_str(),
                        filtered1.c_str(), filtered2.c_str());
    // TODO gzip if output is gz else
    std::rename(filtered1.c_str(), outfile1.c_str());
    std::rename(filtered2.c_str(), outfile2.c_str());
}

template<class T>
void SeqDupRemover<T>::impl_filterPE(const char* infile1,
                                     const char* infile2,
                                     const char* outfile1,
                                     const char* outfile2)
{
    std::ifstream input1{infile1};
    std::ifstream input2{infile2};
    std::ofstream output1{outfile1};
    std::ofstream output2{outfile2};
    check_fstream_ok<std::ifstream>(input1, infile1);
    check_fstream_ok<std::ifstream>(input2, infile2);
    check_fstream_ok<std::ofstream>(output1, outfile1);
    check_fstream_ok<std::ofstream>(output2, outfile2);

    T left, right;
    BufferedInput<T> left_buffer(m_memlimit/2), right_buffer(m_memlimit/2);
    left_buffer.set_file(&input1);
    right_buffer.set_file(&input2);
    left = left_buffer.next();
    right = right_buffer.next();
    this->m_comparator->set_seq(left.seq(), left.seq_len(),
                                right.seq(), right.seq_len());
    output1 << left;
    output2 << right;

    while (!left_buffer.eof() && !right_buffer.eof())
    {
        while (!left_buffer.block_end() && !right_buffer.block_end())
        {
            left = left_buffer.next();
            right = right_buffer.next();
            if (!(this->m_comparator->compare(left.seq(), left.seq_len(),
                                              right.seq(), right.seq_len())))
            {
                this->m_comparator->set_seq(left.seq(), left.seq_len(),
                                            right.seq(), right.seq_len());
                output1 << left;
                output2 << right;
            }
        }
        if (left_buffer.block_end())
            left_buffer.refresh();
        if (right_buffer.block_end())
            right_buffer.refresh();
    }
}
