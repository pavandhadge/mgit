#include "headers/GitHead.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

bool gitHead::readHead() {
  try {
    std::string headPath = ".git/HEAD";
    if (!std::filesystem::exists(headPath)) {
      std::cerr << "HEAD file does not exist\n";
      return false;
    }

    std::ifstream head(headPath);
    if (!head.is_open()) {
      std::cerr << "Error opening HEAD file\n";
      return false;
    }

    std::string content;
    std::getline(head, content);
    head.close();

    if (content.rfind("ref: ", 0) != 0) {
      std::cerr << "Invalid HEAD format: " << content << "\n";
      return false;
    }

    std::string refPath = content.substr(5);                // removes "ref: "
    branch = refPath.substr(refPath.find_last_of('/') + 1); // e.g., "master"

    std::string fullBranchPath = ".git/" + refPath;
    if (!std::filesystem::exists(fullBranchPath)) {
      std::cerr << "Branch ref file does not exist: " << fullBranchPath << "\n";
      return false;
    }

    std::ifstream branchFile(fullBranchPath);
    if (!branchFile.is_open()) {
      std::cerr << "Error opening branch ref file\n";
      return false;
    }

    if (!std::getline(branchFile, branchHeadHash)) {
      // Empty branch file: treat as no commit yet
      branchHeadHash.clear();
      branchFile.close();
      return true;
    }
    branchFile.close();

    return true;
  } catch (const std::exception& e) {
    std::cerr << "readHead failed: " << e.what() << std::endl;
    return false;
  }
}

bool gitHead::updateHead(const std::string &newCommitHash) {
  std::string branchName;
  bool headOk = readHead();
  if (headOk) {
    branchName = branch;
  } else {
    std::string headPath = ".git/HEAD";
    if (std::filesystem::exists(headPath)) {
      std::ifstream head(headPath);
      std::string content;
      std::getline(head, content);
      head.close();
      if (content.rfind("ref: ", 0) == 0) {
        std::string refPath = content.substr(5);
        branchName = refPath.substr(refPath.find_last_of('/') + 1);
      } else {
        std::cerr << "Invalid HEAD format: " << content << "\n";
      }
    } else {
      std::cerr << "HEAD file does not exist\n";
    }
  }
  if (branchName.empty()) {
    std::cerr << "Cannot determine branch name to update HEAD.\n";
    return false;
  }
  std::string fullBranchPath = ".git/refs/heads/" + branchName;
  std::ofstream branchFile(fullBranchPath);
  if (!branchFile.is_open()) {
    std::cerr << "Error creating/writing branch file: " << fullBranchPath << "\n";
    return false;
  }
  branchFile << newCommitHash << "\n";
  branchFile.close();
  branch = branchName;
  branchHeadHash = newCommitHash;
  return true;
}

bool gitHead::writeHeadToHeadOfNewBranch(const std::string &branchName) {
  try {
    std::string headPath = ".git/HEAD";
    std::ofstream headFile(headPath, std::ios::trunc);
    if (!headFile.is_open()) {
      std::cerr << "Error writing to HEAD file\n";
      return false;
    }

    headFile << "ref: refs/heads/" << branchName << "\n";
    headFile.close();

    // Update internal state
    branch = branchName;
    branchHeadHash.clear(); // We don't know the new hash yet

    return true;
  } catch (const std::exception& e) {
    std::cerr << "writeHeadToHeadOfNewBranch failed: " << e.what() << std::endl;
    return false;
  }
}

std::string gitHead::getBranch() const { 
  if (branch.empty()) {
    const_cast<gitHead*>(this)->readHead();
  }
  return branch; 
}

std::string gitHead::getBranchHeadHash() const { 
  if (branchHeadHash.empty()) {
    const_cast<gitHead*>(this)->readHead();
  }
  return branchHeadHash; 
}
