#pragma once

#include <string>

class GitInit {
public:
    explicit GitInit(const std::string& gitDir = ".git");

    // Main entry point to initialize the repository
    bool run();

private:
    std::string gitDir;

    bool createDirectory(const std::string& path);
    bool createFile(const std::string& path, const std::string& content);
    bool directoryExists(const std::string& path);
    bool isDirectory(const std::string& path);
};
