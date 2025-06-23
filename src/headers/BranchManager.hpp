#pragma once
#include <string>
#include <vector>

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
    std::vector<std::string> listBranches() const;

    // Get the commit hash where a branch currently points
    std::string getCurrentBranchHash() const;

    // Update the commit hash a branch points to
    bool updateBranchHead(const std::string& branchName, const std::string& newHash);

    std::string getBranchHash(const std::string& branchName) const ;
private:
    std::string gitDir = ".git/";
    std::string headsDir = ".git/refs/heads/";
    std::string headFile = ".git/HEAD";


};
