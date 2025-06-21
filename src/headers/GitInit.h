#pragma once

#include <string>

class GitInit {
public:
    explicit GitInit(const std::string& gitDir = ".git");

    // Main entry point to initialize the repository
    void run();

private:
    std::string gitDir;

    void createDirectory(const std::string& path);
    void createFile(const std::string& path, const std::string& content);
    bool directoryExists(const std::string& path);
    bool isDirectory(const std::string& path);
};
