#pragma once

#include "GitIndex.hpp"
#include "GitObjectStorage.hpp"
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
// #include "GitObjectType.hpp"

enum class ConflictStatus {
  NO_CONFLICT,
  CONTENT_CONFLICT,
  DELETED_IN_OURS,
  DELETED_IN_THEIRS,
  TREE_CONFLICT,
  MODIFIED_IN_BOTH,
  ADDED_IN_BOTH,
  RENAMED_IN_BOTH,
  RENAMED_IN_ONE
};

std::ostream &operator<<(std::ostream &os, const ConflictStatus &status);

class GitMerge {
public:
  // Constructor
  GitMerge(const std::string &gitDir);
  ~GitMerge();

  // Check for conflicts between two branches
  bool checkForConflicts(const std::string &currentBranch,
                         const std::string &targetBranch);

  // Get list of conflicting files
  std::vector<std::string> getConflictingFiles();

  // Get conflict status for a specific file
  ConflictStatus getFileConflictStatus(const std::string &filename);

  // Get detailed conflict information
  std::string getConflictDetails(const std::string &filename);

  // Three-way merge methods
  bool threeWayMerge(const std::string &currentCommit,
                     const std::string &targetCommit,
                     const std::string &commonAncestor);
  std::string mergeTrees(const std::string &currentTree,
                         const std::string &targetTree,
                         const std::string &ancestorTree);

  // Error handling
  class MergeException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

private:
  std::string gitDir;
  GitObjectStorage storage;
  IndexManager index;
  std::vector<std::string> conflicts;
  std::map<std::string, std::string> conflictDetails;
  std::mutex mergeMutex;

  // Helper functions
  bool compareTrees(const std::string &tree1, const std::string &tree2);
  bool compareBlobs(const std::string &blob1, const std::string &blob2);
  void findConflictsInTree(const std::string &tree1, const std::string &tree2);
  void findConflictsInBlob(const std::string &blob1, const std::string &blob2,
                           const std::string &filename);
  void detectFileRenames(const std::string &tree1, const std::string &tree2);
  void detectDirectoryConflicts(const std::string &tree1,
                                const std::string &tree2);

  // Three-way merge helpers
  std::string findCommonAncestor(const std::string &currentCommit,
                                 const std::string &targetCommit);
  std::string getBlobContent(const std::string &hash);
  std::string mergeFileContents(const std::string &baseContent,
                                const std::string &ourContent,
                                const std::string &theirContent);

  // Tree comparison helpers
  void compareTreeEntries(const std::string &tree1, const std::string &tree2,
                          bool recursive = true);

  // Error handling
  void validateCommit(const std::string &commitHash);
  void validateBranch(const std::string &branchName);
  void validatePath(const std::string &path);
  void validateTreeHash(const std::string &treeHash);
  void validateBlobHash(const std::string &blobHash);
};
