#pragma once
#include <string>
#include <memory.h>
#include "GitObjectStorage.hpp"
class GitRepository{
  private :
  std::string gitDir;
      // std::unique_ptr<GitObjectStore> objectStore;
public :
    GitRepository(const std::string& root = ".git");
    void init(const std::string &path);
    // For blob/tree:
    std::string writeObject(GitObjectType type, const std::string& path , const bool &write);

    // For commit:
    std::string writeObject(GitObjectType type,
        const std::string& currentHash,
                             const std::string& parentHash,
                             const std::string& commitMessage,
                             const std::string& author = "");

    // For tag:
    std::string writeObject(GitObjectType type,
        const std::string& targetHash,
                             const std::string& targetType,
                             const std::string& tagName,
                             const std::string& tagMessage,
                             const std::string& taggerLine);
     std::string readObject(const GitObjectType type , const std::string& hash);

     std::string readObjectRaw(const std::string &path);

};
