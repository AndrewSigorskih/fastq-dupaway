#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <boost/functional/hash.hpp>
// TODO maybe change hash_combine to https://stackoverflow.com/a/72073933
// or https://www.biostars.org/p/184993/#185003

#include "bufferedinput.hpp"
#include "external_sort.hpp"
#include "file_utils.hpp"

using std::string;
using FileUtils::TemporaryDirectory;
struct setRecordHash;
struct setRecordPairHash;
const long ONE_MIL = 1000L * 1000L;

class setRecord
{
public:
    setRecord() {}
    setRecord(const char*, ssize_t);
    bool operator==(const setRecord&) const;
    friend setRecordHash;
private:
    ssize_t m_seq_len;
    std::vector<uint64_t> m_hash;
};

class setRecordPair
{
public:
    setRecordPair() {}
    setRecordPair(const char*, ssize_t, const char*, ssize_t);
    bool operator==(const setRecordPair&) const;
    friend setRecordPairHash;
private:
    ssize_t m_l_len, m_r_len;
    std::vector<uint64_t> m_l_hash, m_r_hash;
};

struct setRecordHash
{
    std::size_t operator()(const setRecord &item) const
    {
        std::size_t seed = item.m_hash.size();
        for (auto& i: item.m_hash)
            boost::hash_combine(seed, i);
        return seed;
    }
};

struct setRecordPairHash
{
    uint64_t operator()(const setRecordPair &item) const
    {
        std::size_t seed = item.m_l_hash.size();
        for (auto& i: item.m_l_hash)
            boost::hash_combine(seed, i);

        boost::hash_combine(seed, item.m_r_hash.size());
        for (auto& i: item.m_r_hash)
            boost::hash_combine(seed, i);

        return seed;
    }
};

using hashed_set = std::unordered_set<setRecord, setRecordHash>;
using paired_hashed_set = std::unordered_set<setRecordPair, setRecordPairHash>;

template<class T>
class HashDupRemover
{
public:
    HashDupRemover(ssize_t memlimit, TemporaryDirectory* tempdir, bool verbose)
        : m_memlimit(memlimit), m_tempdir(tempdir), m_verbose(verbose) {}
    ~HashDupRemover() {}
    void filterSE(const string&, const string&);
    void filterPE(const string&, const string&,
                  const string&, const string&,
                  bool);
private:
    void impl_filterSE(const char*, const char*);
    void impl_filterPE(const char*, const char*,
                       const char*, const char*);
    void impl_filterPE_unordered(const char*, const char*,
                                 const char*, const char*);
private:
    ssize_t             m_memlimit;
    TemporaryDirectory* m_tempdir;
    bool                m_verbose;
};


template<class T>
void HashDupRemover<T>::filterSE(const string& infile,
                                 const string& outfile)
{
    // deduplicate file
    this->impl_filterSE(infile.c_str(), outfile.c_str());
}

template<class T>
void HashDupRemover<T>::impl_filterSE(const char* infilename,
                                      const char* outfilename)
{
    // std::unique_ptr<FileUtils::I_OutputFile> output_file{FileUtils::openOutputFile(outfilename)};
    FileUtils::UniversalOutputFile output_file{outfilename};

    T obj;
    hashed_set records;
    records.reserve(ONE_MIL);  // TODO optimize this value?
    BufferedInput<T> buffer(5L * constants::HUNDRED_MB);
    size_t tot_reads = 0ul, dup_reads = 0ul;

    buffer.set_file(infilename);
    obj = buffer.next();
    tot_reads++;

    // output_file->write(obj.start(), obj.size());
    output_file.write(obj.start(), obj.size());
    records.insert(std::move(setRecord(obj.seq(), obj.seq_len()-1)));

    while (!buffer.eof())
    {
        while (!buffer.block_end())
        {
            obj = buffer.next();
            setRecord record(obj.seq(), obj.seq_len()-1);
            tot_reads++;
            auto it = records.find(record);
            if (it == records.end())
            {   
                // output_file->write(obj.start(), obj.size());
                output_file.write(obj.start(), obj.size());
                records.insert(std::move(record));
            } else {
                dup_reads++;
            }
        }
        buffer.refresh();
    }

    if (m_verbose)
        std::cout << tot_reads << " reads processed, out of which " << dup_reads << " duplicates were removed.\n";
}

template<class T>
void HashDupRemover<T>::filterPE(const string& infile1,
                                 const string& infile2,
                                 const string& outfile1,
                                 const string& outfile2,
                                 bool unordered_flag)
{
    string infilename1 = infile1;
    string infilename2 = infile2;

    // sort both files by ID if needed
    if (unordered_flag)
    {
        // sort first file
        {
            ExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
            sorter.sort(infilename1.c_str(), m_tempdir->sorted_left().c_str());
        }

        // sort second file
        {
            ExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
            sorter.sort(infilename2.c_str(), m_tempdir->sorted_right().c_str());
        }

        infilename1 = m_tempdir->sorted_left();
        infilename2 = m_tempdir->sorted_right();
    }

    // deduplicate 2 files
    if (unordered_flag)
    {
        this->impl_filterPE_unordered(infilename1.c_str(),
                                      infilename2.c_str(),
                                      outfile1.c_str(),
                                      outfile2.c_str());
    } else {
        this->impl_filterPE(infilename1.c_str(),
                            infilename2.c_str(),
                            outfile1.c_str(),
                            outfile2.c_str());
    }
}

template<class T>
void HashDupRemover<T>::impl_filterPE(const char* infile1,
                                      const char* infile2,
                                      const char* outfile1,
                                      const char* outfile2)
{
    // std::unique_ptr<FileUtils::I_OutputFile> output_file1{FileUtils::openOutputFile(outfile1)};
    // std::unique_ptr<FileUtils::I_OutputFile> output_file2{FileUtils::openOutputFile(outfile2)};
    FileUtils::UniversalOutputFile output_file1{outfile1};
    FileUtils::UniversalOutputFile output_file2{outfile2};

    T left, right;
    paired_hashed_set records;
    records.reserve(ONE_MIL);  // TODO optimize this value?
    BufferedInput<T> left_buffer(5L * constants::HUNDRED_MB), right_buffer(5L * constants::HUNDRED_MB);
    size_t tot_reads = 0ul, dup_reads = 0ul;

    left_buffer.set_file(infile1);
    right_buffer.set_file(infile2);
    left = left_buffer.next();
    right = right_buffer.next();
    tot_reads++;

    // output_file1->write(left.start(), left.size());
    // output_file2->write(right.start(), right.size());
    output_file1.write(left.start(), left.size());
    output_file2.write(right.start(), right.size());
    records.insert(
        std::move(
            setRecordPair(left.seq(), left.seq_len()-1,
                          right.seq(), right.seq_len()-1)
        )
    );

    while (!left_buffer.eof() && !right_buffer.eof())
    {
        while (!left_buffer.block_end() && !right_buffer.block_end())
        {
            left = left_buffer.next();
            right = right_buffer.next();
            setRecordPair record(left.seq(), left.seq_len()-1,
                                 right.seq(), right.seq_len()-1);
            tot_reads++;
            auto it = records.find(record);
            if (it == records.end())
            {
                // output_file1->write(left.start(), left.size());
                // output_file2->write(right.start(), right.size());
                output_file1.write(left.start(), left.size());
                output_file2.write(right.start(), right.size());
                records.insert(std::move(record));
            } else {
                dup_reads++;
            }
        }
        left_buffer.refresh();
        right_buffer.refresh();
    }

    if (m_verbose)
        std::cout << tot_reads << " read pairs processed, out of which " << dup_reads << " duplicates were removed.\n";
}

template<class T>
void HashDupRemover<T>::impl_filterPE_unordered(const char* infile1,
                                                const char* infile2,
                                                const char* outfile1,
                                                const char* outfile2)
{
    // std::unique_ptr<FileUtils::I_OutputFile> output_file1{FileUtils::openOutputFile(outfile1)};
    // std::unique_ptr<FileUtils::I_OutputFile> output_file2{FileUtils::openOutputFile(outfile2)};
    FileUtils::UniversalOutputFile output_file1{outfile1};
    FileUtils::UniversalOutputFile output_file2{outfile2};

    T left, right;
    paired_hashed_set records;
    records.reserve(ONE_MIL);  // TODO optimize this value?
    BufferedInput<T> left_buffer(5L * constants::HUNDRED_MB), right_buffer(5L * constants::HUNDRED_MB);
    size_t tot_reads = 0ul, dup_reads = 0ul, unmatch_reads = 0ul;

    left_buffer.set_file(infile1);
    right_buffer.set_file(infile2);
    left = left_buffer.next();
    right = right_buffer.next();

    while (!left_buffer.eof() && !right_buffer.eof())
    {
        while (!left_buffer.block_end() && !right_buffer.block_end())
        {
            int cmp = left.cmp(right);
            if (cmp < 0)
            {
                left = left_buffer.next();
                unmatch_reads++;
            } else if (cmp > 0) {
                right = right_buffer.next();
                unmatch_reads++;
            } else {
                // tags are equal, we can proceed
                setRecordPair record(left.seq(), left.seq_len()-1,
                                 right.seq(), right.seq_len()-1);
                tot_reads++;
                auto it = records.find(record);
                if (it == records.end())
                {
                    // output_file1->write(left.start(), left.size());
                    // output_file2->write(right.start(), right.size());
                    output_file1.write(left.start(), left.size());
                    output_file2.write(right.start(), right.size());
                    records.insert(std::move(record));
                } else {
                    dup_reads++;
                }
                left = left_buffer.next();
                right = right_buffer.next();
            }
        }
        if (left_buffer.block_end())
            left_buffer.refresh();
        if (right_buffer.block_end())
            right_buffer.refresh();
    }

    // check 2 last records
    int cmp = left.cmp(right);
    if (cmp < 0)
    {
        left = left_buffer.next();
        unmatch_reads++;
    } else if (cmp > 0) {
        right = right_buffer.next();
        unmatch_reads++;
    } else {
        setRecordPair record(left.seq(), left.seq_len()-1,
                            right.seq(), right.seq_len()-1);
        tot_reads++;
        auto it = records.find(record);
        if (it == records.end())
        {
            // output_file1->write(left.start(), left.size());
            // output_file2->write(right.start(), right.size());
            output_file1.write(left.start(), left.size());
            output_file2.write(right.start(), right.size());
        } else {
            dup_reads++;
        }
    }

    if (m_verbose)
    {
        std::cout << tot_reads << " valid read pairs processed, out of which " << dup_reads << " duplicates were removed.\n";
        std::cout << unmatch_reads << " Non-matching entries from both files were skipped.\n";
    }
}
