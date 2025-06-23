#pragma once
#include <string>
#include <map>

class BranchManager {
public:
    BranchManager();
    void createBranch(const std::string& name);
    void listBranches() const;
    bool checkoutBranch(const std::string& name);
    std::string getCurrentBranch() const;
    void updateHEAD(const std::string& branchName);
    std::string getBranchHead(const std::string& name) const;

private:
    std::string mgitDir = ".mgit/";
    std::string branchesFile = ".mgit/branches";
    std::string headFile = ".mgit/HEAD";
    std::map<std::string, std::string> branches;
    std::string currentBranch;

    void loadBranches();
    void saveBranches() const;
};
