#pragma once

#include <string>

class gitHead {
private:
  std::string gitDir;
  std::string branch;
  std::string branchHeadHash;

public:
  explicit gitHead(const std::string &gitDir = ".git");
  bool readHead();                          // ✅ Load current HEAD state
  bool updateHead(const std::string &hash); // ✅ Update current branch ref
  bool writeHeadToHeadOfNewBranch(const std::string &branchName); // ✅ Switch branches

  std::string getBranch() const;
  std::string getBranchHeadHash() const;
};
