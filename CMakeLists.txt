cmake_minimum_required(VERSION 3.13)

project(mgit)

set(CMAKE_CXX_STANDARD 17) # Enable the C++17 standard

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)

add_executable(mgit ${SOURCE_FILES})

target_include_directories(mgit PRIVATE src/headers src/utils ${SQLite3_INCLUDE_DIRS})

target_link_libraries(mgit PRIVATE OpenSSL::Crypto)
target_link_libraries(mgit PRIVATE ZLIB::ZLIB)
target_link_libraries(mgit PRIVATE SQLite::SQLite3)
