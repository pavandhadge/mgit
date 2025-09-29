#include "headers/GitMerge.hpp"
#include "headers/GitIndex.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitObjectTypesClasses.hpp"
#include "headers/GitRepository.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>

std::ostream &operator<<(std::ostream &os, const ConflictStatus &status) {
  switch (status) {
  case ConflictStatus::NO_CONFLICT:
    return os << "No conflict";
  case ConflictStatus::CONTENT_CONFLICT:
    return os << "Content conflict";
  case ConflictStatus::DELETED_IN_OURS:
    return os << "Deleted in current branch";
  case ConflictStatus::DELETED_IN_THEIRS:
    return os << "Deleted in target branch";
  case ConflictStatus::TREE_CONFLICT:
    return os << "Directory conflict";
  case ConflictStatus::MODIFIED_IN_BOTH:
    return os << "Modified in both branches";
  case ConflictStatus::ADDED_IN_BOTH:
    return os << "Added in both branches";
  case ConflictStatus::RENAMED_IN_BOTH:
    return os << "Renamed in both branches";
  case ConflictStatus::RENAMED_IN_ONE:
    return os << "Renamed in one branch";
  }
  return os << "Unknown conflict";
}

GitMerge::GitMerge(const std::string &gitDir) : gitDir(gitDir) {
  if (gitDir.empty()) {
    throw MergeException("Git directory path cannot be empty");
  }

  if (!std::filesystem::exists(gitDir)) {
    throw MergeException("Git directory does not exist: " + gitDir);
  }

  storage = std::make_unique<GitObjectStorage>(gitDir);
  index = std::make_unique<IndexManager>(gitDir);
}

GitMerge::~GitMerge() {
  // Ensure resources are properly cleaned up
  storage.reset();
  index.reset();
}

bool GitMerge::validateCommit(const std::string &commitHash) {
  try {
    if (commitHash.empty()) {
      throw MergeException("Commit hash cannot be empty");
    }
    if (commitHash.size() != 40) {
      throw MergeException("Invalid commit hash length: " +
                           std::to_string(commitHash.size()));
    }
    if (!std::all_of(commitHash.begin(), commitHash.end(), ::isxdigit)) {
      throw MergeException("Invalid commit hash format");
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "validateCommit failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitMerge::validateBranch(const std::string &branchName) {
  try {
    if (branchName.empty()) {
      throw MergeException("Branch name cannot be empty");
    }
    if (branchName.find('/') != std::string::npos) {
      throw MergeException("Invalid branch name: " + branchName);
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "validateBranch failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitMerge::validatePath(const std::string &path) {
  try {
    if (path.empty()) {
      throw MergeException("Path cannot be empty");
    }
    if (!std::filesystem::exists(path)) {
      throw MergeException("Path does not exist: " + path);
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "validatePath failed: " << e.what() << std::endl;
    return false;
  }
}

std::string GitMerge::findCommonAncestor(const std::string &currentCommit,
                                         const std::string &targetCommit) {
  validateCommit(currentCommit);
  validateCommit(targetCommit);

  std::lock_guard<std::mutex> lock(mergeMutex);

  GitRepository repo(gitDir);
  std::unordered_set<std::string> currentBranchHistory =
      repo.logBranchCommitHistory(currentCommit);
  std::unordered_set<std::string> targetBranchHistory =
      repo.logBranchCommitHistory(targetCommit);

  std::string commonAncestor = "";
  for (const auto &commit : currentBranchHistory) {
    if (targetBranchHistory.find(commit) != targetBranchHistory.end()) {
      commonAncestor = commit;
      break;
    }
  }

  if (commonAncestor.empty()) {
    throw MergeException("No common ancestor found between commits");
  }
  return commonAncestor;
}

std::string GitMerge::getBlobContent(const std::string &hash) {
  if (hash.empty())
    return "";

  BlobObject blob(gitDir);
  BlobData data = blob.readObject(hash);
  // return data.content;
  if (data.content.empty()) {
    throw std::runtime_error("Failed to read blob content");
  }
  return data.content;
}

std::string GitMerge::mergeFileContents(const std::string &baseContent,
                                        const std::string &ourContent,
                                        const std::string &theirContent) {
  std::stringstream result;

  if (baseContent == ourContent && ourContent == theirContent) {
    return ourContent; // No conflict
  }

  if (baseContent == ourContent) {
    return theirContent; // Their change
  }

  if (baseContent == theirContent) {
    return ourContent; // Our change
  }

  // Conflict - create conflict markers
  result << "<<<<<<< HEAD\n"
         << ourContent << "\n=======\n"
         << theirContent << "\n>>>>>>> " << theirContent << "\n";

  return result.str();
}

bool GitMerge::mergeTrees(const std::string &currentTree,
                          const std::string &targetTree,
                          const std::string &ancestorTree,
                          std::string &mergedTreeHash) {
  try {
    // Get tree objects
    TreeObject tree(gitDir);
    std::vector<TreeEntry> currentEntries = tree.readObject(currentTree);
    std::vector<TreeEntry> targetEntries = tree.readObject(targetTree);
    std::vector<TreeEntry> ancestorEntries = tree.readObject(ancestorTree);

    // Convert to map for easier processing
    std::map<std::string, std::string> currentMap, targetMap, ancestorMap;
    for (const auto &entry : currentEntries) {
      currentMap[entry.filename] = entry.hash;
    }
    for (const auto &entry : targetEntries) {
      targetMap[entry.filename] = entry.hash;
    }
    for (const auto &entry : ancestorEntries) {
      ancestorMap[entry.filename] = entry.hash;
    }

    // Create new tree
    std::stringstream newTree;

    // Process each entry
    for (const auto &[path, currentHash] : currentMap) {
      if (targetMap.find(path) != targetMap.end()) {
        // Both branches modified
        std::string baseHash = ancestorMap[path];
        std::string ourContent = getBlobContent(currentHash);
        std::string theirContent = getBlobContent(targetMap[path]);
        std::string baseContent = getBlobContent(baseHash);

        std::string mergedContent =
            mergeFileContents(baseContent, ourContent, theirContent);
        BlobObject blobObj(gitDir);
        std::string mergedHash = blobObj.writeObject(mergedContent, true);
        newTree << "100644 " << mergedHash << "\0" << path;
      } else if (ancestorMap.find(path) == ancestorMap.end()) {
        // Only current branch modified
        newTree << "100644 " << currentHash << "\0" << path;
      }
    }

    // Add new entries from target branch
    for (const auto &[path, targetHash] : targetMap) {
      if (currentMap.find(path) == currentMap.end()) {
        newTree << "100644 " << targetHash << "\0" << path;
      }
    }

    // Create new tree object
    mergedTreeHash = tree.writeObject(newTree.str());
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Tree merge failed: " << e.what() << "\n";
    return false;
  }
}

bool GitMerge::threeWayMerge(const std::string &currentCommit,
                             const std::string &targetCommit,
                             const std::string &commonAncestor) {
  try {
    // Get tree hashes
    CommitObject commit(gitDir);
    CommitData currentTreeCommit = commit.readObject(currentCommit);
    CommitData targetTreeCommit = commit.readObject(targetCommit);
    CommitData ancestorTreeCommit = commit.readObject(commonAncestor);

    // Merge trees
    std::string mergedTree;
    if (!mergeTrees(currentTreeCommit.tree, targetTreeCommit.tree,
                    ancestorTreeCommit.tree, mergedTree)) {
      throw std::runtime_error("Failed to merge trees");
    }

    // Update index with merged tree
    IndexManager index(gitDir);
    // TODO: Implement updateIndexFromTree method
    // index->updateIndexFromTree(mergedTree);

    return true;
  } catch (const std::exception &e) {
    std::cerr << "Three-way merge failed: " << e.what() << "\n";
    return false;
  }
}

bool GitMerge::checkForConflicts(const std::string &currentBranch,
                                 const std::string &targetBranch) {
  validateBranch(currentBranch);
  validateBranch(targetBranch);

  std::lock_guard<std::mutex> lock(mergeMutex);

  GitRepository repo(gitDir);

  // Get tree hashes for both branches
  std::string currentHead = repo.getHashOfBranchHead(currentBranch);
  std::string targetHead = repo.getHashOfBranchHead(targetBranch);

  if (currentHead.empty() || targetHead.empty()) {
    throw MergeException("One or both branches have no commits");
  }

  // Get tree objects from commits
  CommitObject commitObj(gitDir);
  CommitData currentCommitData = commitObj.readObject(currentHead);
  std::string currentTreeHash = currentCommitData.tree;
  validateTreeHash(currentTreeHash);

  CommitData targetCommitData = commitObj.readObject(targetHead);
  std::string targetTreeHash = targetCommitData.tree;
  validateTreeHash(targetTreeHash);

  // Find common ancestor (for three-way merge)
  // TODO: Implement findCommonAncestor method
  std::string commonAncestor =
      ""; // repo.findCommonAncestor(currentHead, targetHead);
  std::string ancestorTreeHash =
      commonAncestor.empty()
          ? ""
          : storage->readObject(commonAncestor).substr(5, 40);

  if (!ancestorTreeHash.empty()) {
    validateTreeHash(ancestorTreeHash);
  }

  // Find conflicts
  conflicts.clear();
  conflictDetails.clear();

  try {
    compareTreeEntries(currentTreeHash, targetTreeHash, false);
    detectFileRenames(currentTreeHash, targetTreeHash);
    detectDirectoryConflicts(currentTreeHash, targetTreeHash);
  } catch (const std::exception &e) {
    throw MergeException("Failed to detect conflicts: " +
                         std::string(e.what()));
  }

  return !conflicts.empty();
}

bool GitMerge::validateTreeHash(const std::string &treeHash) {
  try {
    if (treeHash.empty()) {
      throw MergeException("Tree hash cannot be empty");
    }
    if (treeHash.size() != 40) {
      throw MergeException("Invalid tree hash length: " +
                           std::to_string(treeHash.size()));
    }
    if (!std::all_of(treeHash.begin(), treeHash.end(), ::isxdigit)) {
      throw MergeException("Invalid tree hash format");
    }
    if (storage->readObject(treeHash).empty()) {
      throw MergeException("Tree object not found: " + treeHash);
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "validateTreeHash failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitMerge::validateBlobHash(const std::string &blobHash) {
  try {
    if (blobHash.empty()) {
      throw MergeException("Blob hash cannot be empty");
    }
    if (blobHash.size() != 40) {
      throw MergeException("Invalid blob hash length: " +
                           std::to_string(blobHash.size()));
    }
    if (!std::all_of(blobHash.begin(), blobHash.end(), ::isxdigit)) {
      throw MergeException("Invalid blob hash format");
    }
    BlobObject blobObj(gitDir);
    if (blobObj.readObject(blobHash).content.empty()) {
      throw MergeException("Blob object not found: " + blobHash);
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "validateBlobHash failed: " << e.what() << std::endl;
    return false;
  }
}

std::vector<std::string> GitMerge::getConflictingFiles() {
  std::vector<std::string> result;
  for (const auto &pair : conflicts) {
    if (pair.second != ConflictStatus::NO_CONFLICT) {
      result.push_back(pair.first);
    }
  }
  return result;
}

ConflictStatus GitMerge::getFileConflictStatus(const std::string &filename) {
  auto it = conflicts.find(filename);
  return it != conflicts.end() ? it->second : ConflictStatus::NO_CONFLICT;
}

std::string GitMerge::getConflictDetails(const std::string &filename) {
  auto it = conflictDetails.find(filename);
  return it != conflictDetails.end() ? it->second : "";
}

bool GitMerge::compareTrees(const std::string &tree1,
                            const std::string &tree2) {
  GitObjectStorage storage(gitDir);

  // Get tree entries
  std::vector<TreeEntry> entries1 = TreeObject(gitDir).readObject(tree1);
  std::vector<TreeEntry> entries2 = TreeObject(gitDir).readObject(tree2);

  // Compare entries
  for (const auto &entry1 : entries1) {
    bool found = false;
    for (const auto &entry2 : entries2) {
      if (entry1.filename == entry2.filename) {
        found = true;
        // If both are directories, compare recursively
        if (entry1.mode == "40000" && entry2.mode == "40000") {
          if (!compareTrees(entry1.hash, entry2.hash)) {
            conflicts[entry1.filename] = ConflictStatus::TREE_CONFLICT;
            conflictDetails[entry1.filename] = "Directory structure differs";
          }
        }
        // If both are files, compare contents
        else if (entry1.mode == "100644" && entry2.mode == "100644") {
          if (!compareBlobs(entry1.hash, entry2.hash)) {
            conflicts[entry1.filename] = ConflictStatus::CONTENT_CONFLICT;
            conflictDetails[entry1.filename] = "File contents differ";
          }
        }
        break;
      }
    }
    if (!found) {
      conflicts[entry1.filename] = ConflictStatus::DELETED_IN_THEIRS;
      conflictDetails[entry1.filename] =
          "File exists in current branch but deleted in target branch";
    }
  }

  // Check for files deleted in current branch
  for (const auto &entry2 : entries2) {
    bool found = false;
    for (const auto &entry1 : entries1) {
      if (entry1.filename == entry2.filename) {
        found = true;
        break;
      }
    }
    if (!found) {
      conflicts[entry2.filename] = ConflictStatus::DELETED_IN_OURS;
      conflictDetails[entry2.filename] =
          "File exists in target branch but deleted in current branch";
    }
  }

  return conflicts.empty();
}

bool GitMerge::compareBlobs(const std::string &blob1,
                            const std::string &blob2) {
  GitObjectStorage storage(gitDir);

  // Get blob contents
  BlobObject blobObj(gitDir);
  std::string content1 = blobObj.readObject(blob1).content;
  std::string content2 = blobObj.readObject(blob2).content;

  // Compare contents
  return content1 == content2;
}

bool GitMerge::findConflictsInTree(const std::string &tree1,
                                   const std::string &tree2) {
  try {
    // TODO: Implement tree conflict detection
    return true;
  } catch (const std::exception &e) {
    std::cerr << "findConflictsInTree failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitMerge::detectFileRenames(const std::string &tree1,
                                 const std::string &tree2) {
  try {
    // TODO: Implement file rename detection
    return true;
  } catch (const std::exception &e) {
    std::cerr << "detectFileRenames failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitMerge::detectDirectoryConflicts(const std::string &tree1,
                                        const std::string &tree2) {
  try {
    // TODO: Implement directory conflict detection
    return true;
  } catch (const std::exception &e) {
    std::cerr << "detectDirectoryConflicts failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitMerge::compareTreeEntries(const std::string &, const std::string &,
                                  bool) {
  try {
    // TODO: Implement this function
    return true;
  } catch (const std::exception &e) {
    std::cerr << "compareTreeEntries failed: " << e.what() << std::endl;
    return false;
  }
}
