cmake_minimum_required(VERSION 3.13)

# Use vcpkg toolchain directly
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "/home/pavan/programming/mgit/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
endif()

project(mgit)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)

add_subdirectory(external/CLI11) # 👈 This adds CLI11 to your build


add_executable(mgit ${SOURCE_FILES})

target_include_directories(mgit PRIVATE src/headers src/utils)

target_link_libraries(mgit PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    SQLite::SQLite3
    CLI11::CLI11)
