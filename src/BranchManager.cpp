#include "headers/BranchManager.hpp"
#include <fstream>
#include <iostream>
// #include <filesystem>

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

    // You may choose to load this commit into the working directory here
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
