#include "headers/GitInit.hpp"
#include "headers/GitHead.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

GitInit::GitInit(const std::string& gitDir) : gitDir(gitDir) {}

bool GitInit::run() {
    try {
        std::string dotGit = gitDir + "/.git";
        if (std::filesystem::exists(dotGit)) {
            if (!std::filesystem::is_directory(dotGit)) {
                throw std::runtime_error(dotGit + " exists but is not a directory.");
            }
            std::cout << "Reinitialized existing Git repository in " << dotGit << "/\n";
        } else {
            createDirectory(dotGit);
            std::cout << "Initialized empty Git repository in " << dotGit << "/\n";
        }

        createDirectory(dotGit + "/objects");
        createDirectory(dotGit + "/refs/heads");
        createFile(dotGit + "/HEAD", "ref: refs/heads/main\n");
        // Create empty index file
        createFile(dotGit + "/index", "");

        gitHead head;
        head.writeHeadToHeadOfNewBranch("main");

        return true;
    } catch (const std::exception& e) {
        std::cerr << "GitInit::run failed: " << e.what() << std::endl;
        return false;
    }
}

bool GitInit::directoryExists(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

bool GitInit::isDirectory(const std::string& path) {
    return std::filesystem::is_directory(path);
}

bool GitInit::createDirectory(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "createDirectory failed: " << e.what() << std::endl;
        return false;
    }
}

bool GitInit::createFile(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to create file: " << path << std::endl;
            return false;
        }
        file << content;
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "createFile failed: " << e.what() << std::endl;
        return false;
    }
}
