#pragma once
#include <string>
#include "bufferedinput.hpp"
#include "comparator.hpp"
#include "external_sort.hpp"
#include "file_utils.hpp"
#include "paired_external_sort.hpp"

using std::string;
using FileUtils::TemporaryDirectory;

template<class T>
class SeqDupRemover
{
public:
    SeqDupRemover(ssize_t, BaseComparator*, TemporaryDirectory*);
    ~SeqDupRemover() {}
    void filterSE(const string&, const string&);
    void filterPE(const string&, const string&,
                  const string&, const string&);
private:
    void impl_filterSE(const char*, const char*);
    void impl_filterPE(const char*, const char*,
                       const char*, const char*);
private:
    bool m_loose_comp = false;
    ssize_t m_memlimit;
    BaseComparator* m_comparator;
    TemporaryDirectory* m_tempdir;
};

template<class T>
SeqDupRemover<T>::SeqDupRemover(ssize_t memlimit,
                                BaseComparator* comparator,
                                TemporaryDirectory* tempdir)
{
    m_memlimit = memlimit;
    m_comparator = comparator;
    m_tempdir = tempdir;
    // perform longest duplicate save only for loose mode to save perfomance
    if (LooseComparator* tmp = dynamic_cast<LooseComparator*>(comparator); tmp != nullptr)
    {  
        m_loose_comp = true;
    }
}

template<class T>
void SeqDupRemover<T>::filterSE(const string& infile,
                                const string& outfile)
{
    // gunzip file if needed
    m_tempdir->set_files(infile);

    string sorted = (boost::format("%1%/data.sorted") % m_tempdir->name()).str();
    {   // sort input file in a scope so all buffers deallocate
        ExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
        sorter.sort(m_tempdir->input1().c_str(), sorted.c_str());
    }
    // remove ungzipped input (or input symlink) here
    m_tempdir->clear_inputs();

    // deduplicate file
    this->impl_filterSE(sorted.c_str(), m_tempdir->output1().c_str());
    
    // save output
    m_tempdir->save_output(outfile);
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
            } else if (m_loose_comp && (this->m_comparator->left_len() <= obj.seq_len()))
            {
                // current sequence is a duplicate, but we need to keep the longest one as a reference
               // this will not affect tight or hamming modes
               this->m_comparator->set_seq(obj.seq(), obj.seq_len());
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
    // gunzip files if needed
    m_tempdir->set_files(infile1, infile2);
    string infilename1 = infile1, infilename2 = infile2;
    
    string sorted1 = (boost::format("%1%/data.sorted1") % m_tempdir->name()).str();
    string sorted2 = (boost::format("%1%/data.sorted2") % m_tempdir->name()).str();
    {  // sort input files in a scope so all buffers deallocate
        PairedExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
        sorter.sort(m_tempdir->input1().c_str(),
                    m_tempdir->input2().c_str(),
                    sorted1.c_str(),
                    sorted2.c_str());
    }
    // remove ungzipped inputs (or input symlinks) here
    m_tempdir->clear_inputs();

    this->impl_filterPE(sorted1.c_str(),
                        sorted2.c_str(),
                        m_tempdir->output1().c_str(),
                        m_tempdir->output2().c_str());
    // save outputs
    m_tempdir->save_output(outfile1, outfile2);
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
            {  // current pair differs -> load it as a new ref
                this->m_comparator->set_seq(left.seq(), left.seq_len(),
                                            right.seq(), right.seq_len());
                output1 << left;
                output2 << right;
            } else if ( m_loose_comp \
                    && (this->m_comparator->left_len() <= left.seq_len()) \
                    && (this->m_comparator->right_len() <= right.seq_len()))
            {
                // current pair is a duplicate, but we need to keep the longest one as a reference
               // this will not affect tight or hamming modes
               this->m_comparator->set_seq(left.seq(), left.seq_len(),
                                           right.seq(), right.seq_len());
            }
        }
        left_buffer.refresh();
        right_buffer.refresh();
    }
}
