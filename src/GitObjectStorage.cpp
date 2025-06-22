#include "headers/GitObjectStorage.hpp"
#include "headers/ZlibUtils.hpp"
#include "headers/HashUtils.hpp"
#include <exception>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

GitObjectStorage::GitObjectStorage(const std::string &gitDir):gitDir(gitDir){

}

GitObjectType GitObjectStorage::identifyType(const std::string &hash){
    std::string content = GitObjectStorage::readObject(hash);
    size_t nullIdx = content.find('\0');
    if (nullIdx == std::string::npos) {
        std::cerr << "Invalid object format\n";
        return GitObjectType::Unknown;
    }
    std::string header = content.substr(0, nullIdx);
    return GitObjectStorage::parseGitObjectTypeFromString (header.substr(0, header.find(' ')));

}
std::string GitObjectStorage::writeObject(const std::string &content ){

    try {


        std::string hash = hash_sha1(content);

        std::string compressed = compressZlib(content);

        std::string objDir = ".git/objects/" + hash.substr(0, 2);

        if (!std::filesystem::exists(".git/objects")) {
            throw std::runtime_error("Error: .git/objects directory does not exist. Did you run `init`?\n");

        }

        std::error_code ec;
        if (!std::filesystem::exists(objDir) && !std::filesystem::create_directories(objDir, ec)) {
            throw std::runtime_error("Error: Failed to create directory " + objDir + ": " + ec.message() + "\n");

        }

        std::string objPath = objDir + "/" + hash.substr(2);

        std::ofstream outFile(objPath, std::ios::binary);
        if (!outFile.is_open()) {
            throw std::runtime_error("Error: Failed to write blob file: " + objPath + "\n");

        }

        outFile.write(compressed.data(), compressed.size());
        outFile.close();
        return hash;
}catch(const std::exception &e){
    std::cout<<"exception occured at GitObjectStorage::WriteObject : "<<e.what()<<std::endl;
    return "";
}
}
    std::string GitObjectStorage::readObject(const std::string& hash){

        try {
            std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
            if (!std::filesystem::exists(path)) {
                throw std::runtime_error("Error: Blob file not found: " +path +"\n");

            }

            std::ifstream blobFile(path, std::ios::binary);
            if (!blobFile.is_open()) {
                throw std::runtime_error("Error: Cannot open blob file: "  +path  +"\n");

            }

            std::vector<char> compressedData((std::istreambuf_iterator<char>(blobFile)),
                                            std::istreambuf_iterator<char>());
            blobFile.close();

            std::string decompressedData = decompressZlib(compressedData);

            return decompressedData ;
        }catch(const std::exception &e){
            std::cout<<"exception occured at GitObjectStorage::readObject : "<<e.what()<<std::endl;
            return "";
        }
   }


  std::string GitObjectStorage::objectTypeToString(GitObjectType type) {
      switch (type) {
          case GitObjectType::Blob: return "blob";
          case GitObjectType::Tree: return "tree";
          case GitObjectType::Commit: return "commit";
          case GitObjectType::Tag: return "tag";
          default: return "unknown";
      }
  }

  GitObjectType GitObjectStorage::parseGitObjectTypeFromString(const std::string& typeStr) {
      if (typeStr == "blob") return GitObjectType::Blob;
      if (typeStr == "tree") return GitObjectType::Tree;
      if (typeStr == "commit") return GitObjectType::Commit;
      if (typeStr == "tag") return GitObjectType::Tag;
      return GitObjectType::Unknown;
  }
