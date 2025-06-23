#pragma once
#include <string>
#include <memory.h>
#include "GitObjectStorage.hpp"
#include <vector>
class GitRepository{
  private :
  std::string gitDir;
      // std::unique_ptr<GitObjectStore> objectStore;
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
 bool isFullyMerged(std::string branchName);

};
