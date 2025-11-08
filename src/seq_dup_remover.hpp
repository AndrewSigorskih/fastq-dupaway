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
    SeqDupRemover(ssize_t memlimit, BaseComparator* comparator, TemporaryDirectory* tempdir, bool write_clusters) 
        : m_memlimit(memlimit), m_comparator(comparator), m_tempdir(tempdir), m_write_clusters(write_clusters)
    {
        // perform longest duplicate save only for loose mode to save perfomance
        if (LooseComparator* tmp = dynamic_cast<LooseComparator*>(comparator); tmp != nullptr)
            m_loose_comp = true;
    }
    ~SeqDupRemover() {}
    void filterSE(const string&, const string&);
    void filterPE(const string&, const string&,
                  const string&, const string&);
private:
    void impl_filterSE(const char*, const char*);
    void impl_filterPE(const char*, const char*,
                       const char*, const char*);
private:
    ssize_t m_memlimit;
    BaseComparator* m_comparator;
    TemporaryDirectory* m_tempdir;
    bool m_loose_comp = false;
    bool m_write_clusters = false;
};

template<class T>
void SeqDupRemover<T>::filterSE(const string& infile,
                                const string& outfile)
{
    {   // sort input file in a scope so all buffers deallocate
        ExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
        sorter.sort(infile.c_str(), m_tempdir->sorted_left().c_str());
    }

    // deduplicate file
    this->impl_filterSE(m_tempdir->sorted_left().c_str(), outfile.c_str());
    
}

template<class T>
void SeqDupRemover<T>::impl_filterSE(const char* infile,
                                     const char* outfile)
{
    std::unique_ptr<FileUtils::I_OutputFile> output_file{FileUtils::openOutputFile(outfile)};
    FileUtils::ClusterFile clusters_file;
    if (m_write_clusters)
        clusters_file.open(outfile);

    T obj;
    BufferedInput<T> buffer(m_memlimit);
    buffer.set_file(infile);
    obj = buffer.next();
    this->m_comparator->set_seq(obj.seq(), obj.seq_len());
    output_file->write(obj.start(), obj.size());
    if (m_write_clusters)
        clusters_file.write_cluster_head(obj.start(), obj.id_len());

    while (!buffer.eof())
    {
        while (!buffer.block_end())
        {
            obj = buffer.next();
            if (!(this->m_comparator->compare(obj.seq(), obj.seq_len())))
            {  // compare returns false -> seqs are different
                this->m_comparator->set_seq(obj.seq(), obj.seq_len());
                output_file->write(obj.start(), obj.size());
                if (m_write_clusters)
                    clusters_file.write_cluster_head(obj.start(), obj.id_len());
            } else {
                if (m_loose_comp && (this->m_comparator->left_len() <= obj.seq_len()))
                {
                    // current sequence is a duplicate, but we need to keep the longest one as a reference
                    // this will not affect tight or hamming modes
                    this->m_comparator->set_seq(obj.seq(), obj.seq_len());
                }

                if (m_write_clusters)
                    clusters_file.write_cluster_item(obj.start(), obj.id_len());
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
    {  // sort input files in a scope so all buffers deallocate
        PairedExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
        sorter.sort(infile1.c_str(),
                    infile2.c_str(),
                    m_tempdir->sorted_left().c_str(),
                    m_tempdir->sorted_right().c_str());
    }

    this->impl_filterPE(m_tempdir->sorted_left().c_str(),
                        m_tempdir->sorted_right().c_str(),
                        outfile1.c_str(),
                        outfile2.c_str());
}

template<class T>
void SeqDupRemover<T>::impl_filterPE(const char* infile1,
                                     const char* infile2,
                                     const char* outfile1,
                                     const char* outfile2)
{
    std::unique_ptr<FileUtils::I_OutputFile> output_file1{FileUtils::openOutputFile(outfile1)};
    std::unique_ptr<FileUtils::I_OutputFile> output_file2{FileUtils::openOutputFile(outfile2)};
    FileUtils::ClusterFile clusters_file1, clusters_file2;
    if (m_write_clusters)
    {
        clusters_file1.open(outfile1);
        clusters_file2.open(outfile2);
    }

    T left, right;
    BufferedInput<T> left_buffer(m_memlimit/2), right_buffer(m_memlimit/2);
    left_buffer.set_file(infile1);
    right_buffer.set_file(infile2);
    left = left_buffer.next();
    right = right_buffer.next();
    this->m_comparator->set_seq(left.seq(), left.seq_len(),
                                right.seq(), right.seq_len());

    output_file1->write(left.start(), left.size());
    output_file2->write(right.start(), right.size());
    if (m_write_clusters)
    {
        clusters_file1.write_cluster_head(left.start(), left.id_len());
        clusters_file2.write_cluster_head(right.start(), right.id_len());
    }

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
                output_file1->write(left.start(), left.size());
                output_file2->write(right.start(), right.size());
                if (m_write_clusters)
                {
                    clusters_file1.write_cluster_head(left.start(), left.id_len());
                    clusters_file2.write_cluster_head(right.start(), right.id_len());
                }
            } else {

                if ( m_loose_comp \
                    && (this->m_comparator->left_len() <= left.seq_len()) \
                    && (this->m_comparator->right_len() <= right.seq_len()))
                {
                    // current pair is a duplicate, but we need to keep the longest one as a reference
                    // this will not affect tight or hamming modes
                    this->m_comparator->set_seq(left.seq(), left.seq_len(),
                                                right.seq(), right.seq_len());
                }

                if (m_write_clusters)
                {
                    clusters_file1.write_cluster_item(left.start(), left.id_len());
                    clusters_file2.write_cluster_item(right.start(), right.id_len());
                }

            }
        }
        left_buffer.refresh();
        right_buffer.refresh();
    }
}
