#pragma once
#include <string>
#include <cstring>  // Replaced memory.h with cstring
#include "GitObjectStorage.hpp"
#include "GitMerge.hpp"
#include "GitConfig.hpp"
#include "GitHead.hpp"
#include "GitIndex.hpp"
#include <vector>
#include <unordered_set>
#include <optional>
#include <memory>  // For unique_ptr
#include <stdexcept>  // For exceptions
#include <mutex>  // For thread safety

class GitRepository {
private:
    std::string gitDir;
    std::unique_ptr<GitMerge> merge;  // Use smart pointer for better ownership
    std::mutex mergeMutex;  // For thread safety
    
    // Helper methods
    std::string createMergeCommit(const std::string& message, const std::string& author,
                                const std::string& currentCommit, const std::string& targetCommit);
    bool resolveConflicts(const std::string& targetBranch);
      // std::unique_ptr<GitObjectStore> objectStore;

  // Helper methods
  std::string createMergeCommit(const std::string& message, const std::string& author,
                              const std::string& currentCommit, const std::string& targetCommit);
  bool resolveConflicts(const std::string& targetBranch);

public :
 void reportStatus(bool shortFormat = false, bool showUntracked = true);
    GitRepository(const std::string& root = ".git");
    void init(const std::string &path);
    // For blob/tree:
    std::string writeObject(GitObjectType type, const std::string& path , const bool &write);

    // For commit:
    std::string writeObject(GitObjectType type, const CommitData& data);

    // For tag:
    std::string writeObject(GitObjectType type, const TagData& data);
     std::string readObject(const GitObjectType type , const std::string& hash);

     std::string readObjectRaw(const std::string &path);
 void indexHandler(const std::vector<std::string> &paths={"."});

 void CreateBranch(const std::string &branchName);
 void listbranches(const std::string &branchName);
 std::string getCurrentBranch()const ;

 void changeCurrentBranch(const std::string &targetBranch , bool createflag);

 std::string getHashOfBranchHead(const std::string &branchName);

 bool deleteBranch(const std::string &branchName);

 bool renameBranch(const std::string& oldName, const std::string& newName);
 bool isFullyMerged(const std::string& branchName);
 bool createCommit( const std::string& message,const std::string& author);
 std::unordered_set<std::string> logBranchCommitHistory(const std::string &branchName);
    bool gotoStateAtPerticularCommit(const std::string& hash);
    bool exportHeadAsZip(const std::string& branchName, const std::string& outputZipPath);

    // Merge operations
    bool mergeBranch(const std::string& targetBranch);
    bool abortMerge();
    std::vector<std::string> getConflictingFiles();
    bool isConflicted(const std::string& path);
    void resolveConflict(const std::string& path, const std::string& hash);
    std::optional<ConflictMarker> getConflictMarker(const std::string& path);
    void reportMergeConflicts(const std::string& targetBranch);
};
