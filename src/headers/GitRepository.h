#pragma once
#include <string>
#include <memory.h>

class GitRepository{
  private :
  std::string gitDir;
      // std::unique_ptr<GitObjectStore> objectStore;
public :
    GitRepository(const std::string& root = ".git");
    void init(const std::string &path);

};
