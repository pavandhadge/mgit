cmake_minimum_required(VERSION 3.13)

# Set project name and version
project(mgit VERSION 1.0.0)

# Use vcpkg toolchain directly
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "/home/pavan/programming/mgit/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Compiler Flags ---
# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Set compiler flags for Release build
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Set compiler flags for Debug build
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")

# --- Source Files ---
file(GLOB_RECURSE SOURCE_FILES
    "src/*.cpp"
    "src/*.hpp"
)

# --- Dependencies ---
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)

# --- Subdirectories ---
add_subdirectory(external/CLI11)

# --- Executable ---
add_executable(mgit ${SOURCE_FILES})

# --- Include Directories ---
target_include_directories(mgit PRIVATE
    "src/headers"
    "src/utils"
)

# --- Link Libraries ---
target_link_libraries(mgit PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    SQLite::SQLite3
    CLI11::CLI11
)

# --- Installation ---
install(TARGETS mgit
    DESTINATION bin
)

# --- User-friendly messages ---
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
