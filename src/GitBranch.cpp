#include "headers/GitBranch.hpp"
#include "headers/GitHead.hpp"
#include "headers/GitIndex.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
// #include <filesystem>

Branch::Branch(const std::string &gitDirPath)
    : gitDir(gitDirPath + "/"), headsDir(gitDirPath + "/refs/heads/"),
      headFile(gitDirPath + "/HEAD") {}

bool Branch::createBranch(const std::string& branchName){
    try {
        if (branchName.empty()) {
            throw BranchException("Branch name cannot be empty");
        }
        
        std::string branchPath = headsDir + branchName;
        if (std::filesystem::exists(branchPath)) {
            throw BranchException("Branch already exists: " + branchName);
        }
        
        // Get current HEAD hash
        std::string currentHead = getCurrentBranchHash();
        if (currentHead.empty()) {
            std::cerr << "fatal: Not a valid object name: 'main'\n";
            throw BranchException("Cannot create branch: HEAD is empty");
        }
        
        // Create branch file
        std::ofstream branchFile(branchPath);
        if (!branchFile.is_open()) {
            throw BranchException("Failed to create branch file: " + branchPath);
        }
        branchFile << currentHead << std::endl;
        branchFile.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "createBranch failed: " << e.what() << std::endl;
        return false;
    }
}

bool Branch::checkout(const std::string& branchName){
    try {
        gitHead head(gitDir.substr(0, gitDir.size() - 1));
        if (!head.writeHeadToHeadOfNewBranch(branchName)) {
            throw BranchException("Failed to update HEAD for branch: " + branchName);
        }
        return true;
    } catch(const std::exception &e) {
        std::cerr << "checkout failed: " << e.what() << std::endl;
        return false;
    }
}

std::string Branch::getCurrentBranch()const {
    gitHead head(gitDir.substr(0, gitDir.size() - 1));
    return head.getBranch() ;
}

bool Branch::deleteBranch(const std::string &branchName){
    try {
        if (branchName.empty()) {
            throw BranchException("Branch name cannot be empty");
        }
        
        std::string branchPath = headsDir + branchName;
        if (!std::filesystem::exists(branchPath)) {
            throw BranchException("Branch does not exist: " + branchName);
        }
        
        // Check if trying to delete current branch
        std::string currentBranch = getCurrentBranch();
        if (currentBranch == branchName) {
            throw BranchException("Cannot delete current branch: " + branchName);
        }
        
        if (!std::filesystem::remove(branchPath)) {
            throw BranchException("Failed to delete branch: " + branchName);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "deleteBranch failed: " << e.what() << std::endl;
        return false;
    }
}

bool Branch::listBranches() const {
    try {
        std::vector<std::string> branchList;
        for(const auto& entity : std::filesystem::directory_iterator(headsDir) ){
            if(!entity.is_regular_file()){
                continue;
            }
            branchList.push_back(entity.path().filename().string());
        }
        
        std::string currentBranch = getCurrentBranch();
        std::cout << "Available branches:" << std::endl;
        for (const auto& branch : branchList) {
            if (branch == currentBranch) {
                std::cout << "* " << branch << std::endl;
            } else {
                std::cout << "  " << branch << std::endl;
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "listBranches failed: " << e.what() << std::endl;
        return false;
    }
}

bool Branch::renameBranch(const std::string& oldName, const std::string& newName){
    try {
        if (oldName.empty() || newName.empty()) {
            throw BranchException("Branch names cannot be empty");
        }
        
        std::string oldPath = headsDir + oldName;
        std::string newPath = headsDir + newName;
        
        if (!std::filesystem::exists(oldPath)) {
            throw BranchException("Source branch does not exist: " + oldName);
        }
        
        if (std::filesystem::exists(newPath)) {
            throw BranchException("Target branch already exists: " + newName);
        }
        
        std::filesystem::rename(oldPath, newPath); // throws on error
        
        // Update HEAD if renaming current branch
        std::string currentBranch = getCurrentBranch();
        if (currentBranch == oldName) {
            std::ofstream headRefFile(headFile);
            if (!headRefFile.is_open()) {
                throw BranchException("Failed to update HEAD after rename");
            }
            headRefFile << "ref: refs/heads/" << newName << std::endl;
            headRefFile.close();
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "renameBranch failed: " << e.what() << std::endl;
        return false;
    }
}

std::string Branch::getCurrentBranchHash() const{
    gitHead head(gitDir.substr(0, gitDir.size() - 1));
    return head.getBranchHeadHash() ;
    // return hash ;
}

std::string Branch::getBranchHash(const std::string& branchName) const {
    std::string path = headsDir + branchName;
    if (!std::filesystem::exists(path)) return "";

    std::ifstream in(path);
    std::string hash;
    getline(in, hash);
    return hash;
}

bool Branch::updateBranchHead(const std::string& branchName, const std::string& newHash){
    try {
        if (branchName.empty()) {
            throw BranchException("Branch name cannot be empty");
        }
        if (newHash.empty()) {
            throw BranchException("Hash cannot be empty");
        }
        
        std::string path = headsDir + branchName;
        if(!std::filesystem::exists(path)){
            throw BranchException("Branch does not exist: " + branchName);
        }

        std::ofstream branchFile(path);
        if (!branchFile.is_open()) {
            throw BranchException("Failed to open branch file: " + path);
        }
        branchFile << newHash;
        branchFile.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "updateBranchHead failed: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> Branch::getAllBranches() const {
    std::vector<std::string> branches;
    try {
        if (!std::filesystem::exists(headsDir)) {
            return branches;
        }
        for (const auto &entry : std::filesystem::directory_iterator(headsDir)) {
            if (entry.is_regular_file()) {
                branches.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "getAllBranches failed: " << e.what() << std::endl;
    }
    return branches;
}
