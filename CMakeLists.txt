cmake_minimum_required(VERSION 3.22)
project(
    fastq-dupaway
    VERSION 1.2
    LANGUAGES CXX
)

find_package(Boost 1.81.0 REQUIRED iostreams program_options)

file(GLOB SOURCES "src/*.cpp")

add_executable(fastq-dupaway ${SOURCES})

target_link_libraries(fastq-dupaway PRIVATE Boost::headers Boost::iostreams Boost::program_options)
