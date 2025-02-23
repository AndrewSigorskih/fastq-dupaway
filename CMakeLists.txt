cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(
    fastq-dupaway
    VERSION 1.3
    LANGUAGES CXX
)

find_package(Boost 1.81.0 REQUIRED iostreams program_options)

file(GLOB SOURCES "src/*.cpp")

add_executable(fastq-dupaway ${SOURCES})

target_link_libraries(fastq-dupaway PRIVATE Boost::headers Boost::iostreams Boost::program_options)
