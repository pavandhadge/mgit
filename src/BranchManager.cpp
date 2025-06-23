#include "headers/BranchManager.hpp"
#include "headers/GitCommit.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <string>

BranchManager::BranchManager() {
    loadBranches();
}

void BranchManager::loadBranches() {
    branches.clear();
    std::ifstream in(branchesFile);
    if (in.is_open()) {
        std::string name, hash;
        while (in >> name >> hash) {
            branches[name] = hash;
        }
        in.close();
    }

    std::ifstream headIn(headFile);
    if (headIn.is_open()) {
        getline(headIn, currentBranch);
        headIn.close();
    }
}

void BranchManager::saveBranches() const {
    std::ofstream out(branchesFile);
    if (out.is_open()) {
        for (const auto& [name, hash] : branches) {
            out << name << " " << hash << "\n";
        }
        out.close();
    }

    std::ofstream headOut(headFile);
    if (headOut.is_open()) {
        headOut << currentBranch << "\n";
        headOut.close();
    }
}

void BranchManager::createBranch(const std::string& name) {
    if (branches.find(name) != branches.end()) {
        std::cerr << "Branch already exists.\n";
        return;
    }

    std::ifstream headCommit(".mgit/HEAD_COMMIT");
    std::string hash;
    if (headCommit.is_open()) {
        getline(headCommit, hash);
        headCommit.close();
    }

    if (hash.empty()) {
        std::cerr << "No commit found to base the branch on.\n";
        return;
    }

    branches[name] = hash;
    saveBranches();
    std::cout << "Branch '" << name << "' created at " << hash << "\n";
}

void BranchManager::listBranches() const {
    std::cout << "Branches:\n";
    for (const auto& [name, hash] : branches) {
        if (name == currentBranch) {
            std::cout << "* ";
        } else {
            std::cout << "  ";
        }
        std::cout << name << " (" << hash << ")\n";
    }
}

bool BranchManager::checkoutBranch(const std::string& name) {
    if (branches.find(name) == branches.end()) {
        std::cerr << "Branch does not exist.\n";
        return false;
    }

    std::string targetHash = branches[name];

    std::ofstream headCommit(".mgit/HEAD_COMMIT");
    if (headCommit.is_open()) {
        headCommit << targetHash << "\n";
        headCommit.close();
    }

    currentBranch = name;
    saveBranches();
    return true;
}

void BranchManager::updateHEAD(const std::string& branchName) {
    currentBranch = branchName;
    saveBranches();
}

std::string BranchManager::getCurrentBranch() const {
    return currentBranch;
}

std::string BranchManager::getBranchHead(const std::string& name) const {
    auto it = branches.find(name);
    if (it != branches.end()) {
        return it->second;
    }
    return "";
}

// Branch operations
bool BranchManager::renameBranch(const std::string& oldName, const std::string& newName) {
    if (oldName == newName) {
        std::cerr << "Error: New name is the same as old name.\n";
        return false;
    }

    if (branches.find(oldName) == branches.end()) {
        std::cerr << "Error: Branch '" << oldName << "' does not exist.\n";
        return false;
    }

    if (branches.find(newName) != branches.end()) {
        std::cerr << "Error: Branch '" << newName << "' already exists.\n";
        return false;
    }

    auto commitHash = branches[oldName];
    branches.erase(oldName);
    branches[newName] = commitHash;

    if (oldName == currentBranch) {
        currentBranch = newName;
    }

    saveBranches();
    std::cout << "Successfully renamed branch '" << oldName << "' to '" << newName << "'\n";
    return true;
}

bool BranchManager::mergeBranch(const std::string& branchName) {
    if (branches.find(branchName) == branches.end()) {
        std::cerr << "Error: Branch '" << branchName << "' does not exist.\n";
        return false;
    }

    if (branchName == currentBranch) {
        std::cerr << "Error: Cannot merge branch into itself.\n";
        return false;
    }

    std::string sourceHash = branches[branchName];
    std::string targetHash = branches[currentBranch];

    // TODO: Implement merge logic here
    // This would involve:
    // 1. Finding common ancestor
    // 2. Creating merge commit
    // 3. Updating current branch
    
    std::cout << "Merge of branch '" << branchName << "' into current branch not implemented yet.\n";
    return false;
}

bool BranchManager::resetBranch(const std::string& branchName, const std::string& commitHash) {
    if (branches.find(branchName) == branches.end()) {
        std::cerr << "Error: Branch '" << branchName << "' does not exist.\n";
        return false;
    }

    if (branchName == currentBranch) {
        std::cerr << "Warning: Resetting current branch.\n";
    }

    branches[branchName] = commitHash;
    saveBranches();
    std::cout << "Successfully reset branch '" << branchName << "' to commit " << commitHash << "\n";
    return true;
}

bool BranchManager::rebaseBranch(const std::string& branchName, const std::string& ontoBranchName) {
    if (branches.find(branchName) == branches.end()) {
        std::cerr << "Error: Branch '" << branchName << "' does not exist.\n";
        return false;
    }

    if (branches.find(ontoBranchName) == branches.end()) {
        std::cerr << "Error: Branch '" << ontoBranchName << "' does not exist.\n";
        return false;
    }

    if (branchName == ontoBranchName) {
        std::cerr << "Error: Cannot rebase onto the same branch.\n";
        return false;
    }

    if (branchName == currentBranch) {
        std::cerr << "Warning: Rebasing current branch.\n";
    }

    // TODO: Implement rebase logic here
    // This would involve:
    // 1. Finding common ancestor
    // 2. Creating new commits on top of target branch
    // 3. Updating branch pointer
    
    std::cout << "Rebase of branch '" << branchName << "' onto '" << ontoBranchName << "' not implemented yet.\n";
    return false;
}

// Branch deletion
bool BranchManager::deleteBranch(const std::string& name) {
    if (branches.find(name) == branches.end()) {
        std::cerr << "Error: Branch '" << name << "' does not exist.\n";
        return false;
    }

    if (name == currentBranch) {
        std::cerr << "Error: Cannot delete the currently checked out branch '" << name << "'.\n";
        return false;
    }

    const std::string& commitHash = branches[name];
    if (commitHash.empty()) {
        std::cerr << "Warning: Branch '" << name << "' has no commits.\n";
    }

    branches.erase(name);
    saveBranches();
    std::cout << "Successfully deleted branch '" << name << "'\n";
    return true;
}
