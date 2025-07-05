#pragma once
#include <string>
#include <vector>
#include <filesystem>

class BranchException : public std::exception {
public:
    explicit BranchException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class Branch {
public:
    // Constructor: initialize and load current HEAD info
    Branch();

    // Create a new branch pointing to the current commit
    bool createBranch(const std::string& branchName);

    // Switch to the given branch (i.e., update HEAD to point to refs/heads/<branchName>)
    bool checkout(const std::string& branchName);

    // Get the name of the current branch (from HEAD)
    std::string getCurrentBranch() const;

    // Get list of all local branches (i.e., files in .git/refs/heads/)
    bool listBranches() const;

    // Get the commit hash where a branch currently points
    std::string getCurrentBranchHash() const;

    // Update the commit hash a branch points to
    bool updateBranchHead(const std::string& branchName, const std::string& newHash);
    bool deleteBranch(const std::string& branchName);
    bool renameBranch(const std::string& oldName, const std::string& newName);
    std::string getBranchHash(const std::string& branchName) const;
    std::vector<std::string> getAllBranches() const;

private:
    std::string gitDir = ".git/";
    std::string headsDir = ".git/refs/heads/";
    std::string headFile = ".git/HEAD";
};
