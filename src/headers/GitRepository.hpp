#pragma once
#include "GitConfig.hpp"
#include "GitHead.hpp"
#include "GitIndex.hpp"
#include "GitMerge.hpp"
#include "GitObjectStorage.hpp"
#include <cstring> // Replaced memory.h with cstring
#include <memory>  // For unique_ptr
#include <mutex>   // For thread safety
#include <optional>
#include <stdexcept> // For exceptions
#include <string>
#include <unordered_set>
#include <vector>

class GitRepository {
private:
  std::string gitDir;
  std::unique_ptr<GitMerge> merge; // Use smart pointer for better ownership
  std::mutex mergeMutex;           // For thread safety
  void ensureMergeInitialized();

public:
  // Helper methods
  std::string createMergeCommit(const std::string &message,
                                const std::string &author,
                                const std::string &currentCommit,
                                const std::string &targetCommit);
  bool resolveConflicts();

public:
  bool reportStatus(bool shortFormat = false, bool showUntracked = true);
  GitRepository(const std::string &root = ".git");
  bool init(const std::string &path);
  // For blob/tree:
  std::string writeObject(GitObjectType type, const std::string &path,
                          const bool &write);

  // For commit:
  std::string writeObject(GitObjectType type, const CommitData &data);

  // For tag:
  std::string writeObject(GitObjectType type, const TagData &data);
  std::string readObject(const GitObjectType type, const std::string &hash);

  std::string readObjectRaw(const std::string &path);
  void indexHandler(const std::vector<std::string> &paths = {"."});

  bool CreateBranch(const std::string &branchName);
  bool listbranches(const std::string &branchName);
  std::string getCurrentBranch() const;

  bool changeCurrentBranch(const std::string &targetBranch, bool createflag);

  std::string getHashOfBranchHead(const std::string &branchName);

  bool deleteBranch(const std::string &branchName);

  bool renameBranch(const std::string &oldName, const std::string &newName);
  bool isFullyMerged(const std::string &branchName);
  bool createCommit(const std::string &message, const std::string &author);
  std::unordered_set<std::string>
  logBranchCommitHistory(const std::string &branchName);
  std::string findCommonAncestor(const std::string &commitA,
                                 const std::string &commitB);
  bool gotoStateAtPerticularCommit(const std::string &hash);
  bool exportHeadAsZip(const std::string &branchName,
                       const std::string &outputZipPath);

  // Merge operations
  bool mergeBranch(const std::string &targetBranch);
  bool abortMerge();
  std::vector<std::string> getConflictingFiles();
  bool isConflicted(const std::string &path);
  bool resolveConflict(const std::string &path, const std::string &hash);
  std::optional<ConflictMarker> getConflictMarker(const std::string &path);
  bool reportMergeConflicts(const std::string &targetBranch);

  // Push all new objects and refs to remote .git directory (by name or path)
  bool push(const std::string &remote);
  // Pull all new objects and refs from remote .git directory (by name or path)
  bool pull(const std::string &remote);
};
