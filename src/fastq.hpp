#include <iostream>
#include <string>

using std::string;

class FastQEntry
{
public:
    FastQEntry() {};
    FastQEntry(string& id, string& seq, string& field3, string& qual):
        m_id(id), m_seq(seq), m_field3(field3), m_qual(qual) {}
    FastQEntry(FastQEntry&& other);
    FastQEntry& operator=(FastQEntry&& other);
    friend bool operator>(const FastQEntry& left, const FastQEntry& right);
    friend bool operator<(const FastQEntry& left, const FastQEntry& right);
    friend std::istream& operator>>(std::istream& is, FastQEntry& fq);
    friend std::ostream& operator<<(std::ostream& os,const FastQEntry& fq);
    const string& id() const { return m_id; }
    const string& seq() const { return m_seq; }
   
    // dont store third string!
private:
    string m_id;
    string m_seq;
    string m_field3;
    string m_qual;
};

class FastQEntryWithId : FastQEntry
{
    friend bool operator>(const FastQEntryWithId& left, const FastQEntryWithId& right);
    friend bool operator<(const FastQEntryWithId& left, const FastQEntryWithId& right);
    // add string_view to refactor id() ?
    // https://stackoverflow.com/questions/46032307/how-to-efficiently-get-a-string-view-for-a-substring-of-stdstring
};