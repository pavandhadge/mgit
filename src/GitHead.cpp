#include "headers/GitHead.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

void gitHead::readHead() {
  std::string headPath = ".git/HEAD";
  if (!std::filesystem::exists(headPath)) {
    std::cerr << "HEAD file does not exist\n";
    return;
  }

  std::ifstream head(headPath);
  if (!head.is_open()) {
    std::cerr << "Error opening HEAD file\n";
    return;
  }

  std::string content;
  std::getline(head, content);
  head.close();

  if (content.rfind("ref: ", 0) != 0) {
    std::cerr << "Invalid HEAD format: " << content << "\n";
    return;
  }

  std::string refPath = content.substr(5);                // removes "ref: "
  branch = refPath.substr(refPath.find_last_of('/') + 1); // e.g., "master"

  std::string fullBranchPath = ".git/" + refPath;
  if (!std::filesystem::exists(fullBranchPath)) {
    std::cerr << "Branch ref file does not exist: " << fullBranchPath << "\n";
    return;
  }

  std::ifstream branchFile(fullBranchPath);
  if (!branchFile.is_open()) {
    std::cerr << "Error opening branch ref file\n";
    return;
  }

  std::getline(branchFile, branchHeadHash);
  branchFile.close();
}

bool gitHead::updateHead(const std::string &newCommitHash) {
  // First, read HEAD to know which branch to update
  readHead();

  std::string fullBranchPath = ".git/refs/heads/" + branch;
  std::ofstream branchFile(fullBranchPath, std::ios::trunc);
  if (!branchFile.is_open()) {
    std::cerr << "Error writing to branch file: " << fullBranchPath << "\n";
    return false;
  }

  branchFile << newCommitHash << "\n";
  branchFile.close();

  // Update internal cache
  branchHeadHash = newCommitHash;
  return true;
}

void gitHead::writeHeadToHeadOfNewBranch(const std::string &branchName) {
  std::string headPath = ".git/HEAD";
  std::ofstream headFile(headPath, std::ios::trunc);
  if (!headFile.is_open()) {
    std::cerr << "Error writing to HEAD file\n";
    return;
  }

  headFile << "ref: refs/heads/" << branchName << "\n";
  headFile.close();

  // Update internal state
  branch = branchName;
  branchHeadHash.clear(); // We don't know the new hash yet
}

std::string gitHead::getBranch() const { return branch; }
std::string gitHead::getBranchHeadHash() const { return branchHeadHash; }
