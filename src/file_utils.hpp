#pragma once
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

namespace FS = std::filesystem;

void generate_random_name(char* buf, int len);
void create_random_dir(char* buf, int len, uint n_tries = 100);
void create_random_file(char* buf, int len, uint n_tries = 100);
uint64_t countLines(const char* infilename);
bool fileHasExt(const char* filename, const char* ext=".gz");

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
