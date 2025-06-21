#include "headers/GitInit.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

GitInit::GitInit(const std::string& gitDir) : gitDir(gitDir) {}

void GitInit::run() {
    try {
        if (std::filesystem::exists(gitDir)) {
            if (!std::filesystem::is_directory(gitDir)) {
                throw std::runtime_error(gitDir + " exists but is not a directory.");
            }
            std::cout << "Reinitialized existing Git repository in " << gitDir << "/\n";
        } else {
            createDirectory(gitDir);
            std::cout << "Initialized empty Git repository in " << gitDir << "/\n";
        }

        createDirectory(gitDir + "/objects");
        createDirectory(gitDir + "/refs/heads");
        createFile(gitDir + "/HEAD", "ref: refs/heads/main\n");

    } catch (const std::exception& e) {
        std::cerr << "Init error: " << e.what() << "\n";
        throw; // allow caller to catch if needed
    }
}

bool GitInit::directoryExists(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

bool GitInit::isDirectory(const std::string& path) {
    return std::filesystem::is_directory(path);
}

void GitInit::createDirectory(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        if (!std::filesystem::create_directories(path)) {
            throw std::runtime_error("Failed to create directory: " + path);
        }
    }
}

void GitInit::createFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to create file: " + path);
    }
    file << content;
    file.close();
}
