#include "headers/GitObjectStorage.hpp"
#include "headers/HashUtils.hpp"
#include "headers/ZlibUtils.hpp"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

GitObjectStorage::GitObjectStorage(const std::string &gitDir)
    : gitDir(gitDir) {}

bool GitObjectStorage::writeObject(const std::string &hash,
                                   const std::string &content) {
  try {
    if (hash.empty()) {
      throw StorageException("Hash cannot be empty");
    }
    if (content.empty()) {
      throw StorageException("Content cannot be empty");
    }

    std::string objectPath = getObjectPath(hash);
    std::filesystem::create_directories(
        std::filesystem::path(objectPath).parent_path());

    std::ofstream objectFile(objectPath);
    if (!objectFile.is_open()) {
      throw StorageException("Failed to create object file: " + objectPath);
    }

    objectFile << content;
    objectFile.close();

    return true;
  } catch (const std::exception &e) {
    std::cerr << "writeObject failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitObjectStorage::deleteObject(const std::string &hash) {
  try {
    if (hash.empty()) {
      throw StorageException("Hash cannot be empty");
    }

    std::string objectPath = getObjectPath(hash);
    if (!std::filesystem::exists(objectPath)) {
      throw StorageException("Object does not exist: " + hash);
    }

    if (!std::filesystem::remove(objectPath)) {
      throw StorageException("Failed to delete object: " + hash);
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "deleteObject failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitObjectStorage::objectExists(const std::string &hash) {
  try {
    if (hash.empty()) {
      return false;
    }

    std::string objectPath = getObjectPath(hash);
    return std::filesystem::exists(objectPath);
  } catch (const std::exception &e) {
    std::cerr << "objectExists failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitObjectStorage::validateHash(const std::string &hash) {
  try {
    if (hash.empty()) {
      throw StorageException("Hash cannot be empty");
    }

    if (hash.size() != 40) {
      throw StorageException("Invalid hash length: " +
                             std::to_string(hash.size()));
    }

    if (!std::all_of(hash.begin(), hash.end(), ::isxdigit)) {
      throw StorageException("Invalid hash format");
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "validateHash failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitObjectStorage::cleanupOrphanedObjects() {
  try {
    std::string objectsDir = gitDir + "/objects";
    if (!std::filesystem::exists(objectsDir)) {
      return true;
    }

    // TODO: Implement orphaned object cleanup
    // This would involve checking which objects are referenced by commits/trees
    // and removing unreferenced objects

    return true;
  } catch (const std::exception &e) {
    std::cerr << "cleanupOrphanedObjects failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitObjectStorage::compressObjects() {
  try {
    std::string objectsDir = gitDir + "/objects";
    if (!std::filesystem::exists(objectsDir)) {
      return true;
    }

    // TODO: Implement object compression
    // This would involve compressing loose objects into pack files

    return true;
  } catch (const std::exception &e) {
    std::cerr << "compressObjects failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitObjectStorage::validateObjectIntegrity(const std::string &hash) {
  try {
    if (!validateHash(hash)) {
      return false;
    }

    std::string objectPath = getObjectPath(hash);
    if (!std::filesystem::exists(objectPath)) {
      throw StorageException("Object file does not exist: " + hash);
    }

    std::ifstream objectFile(objectPath);
    if (!objectFile.is_open()) {
      throw StorageException("Failed to open object file: " + objectPath);
    }

    std::string content;
    std::getline(objectFile, content);
    objectFile.close();

    if (content.empty()) {
      throw StorageException("Object file is empty: " + hash);
    }

    // TODO: Add more integrity checks (e.g., verify content matches hash)

    return true;
  } catch (const std::exception &e) {
    std::cerr << "validateObjectIntegrity failed: " << e.what() << std::endl;
    return false;
  }
}

std::string GitObjectStorage::readObject(const std::string &hash) {
  try {
    std::string path =
        gitDir + "/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    if (!std::filesystem::exists(path)) {
      throw std::runtime_error("Error: Blob file not found: " + path);
    }

    std::ifstream blobFile(path, std::ios::binary);
    if (!blobFile.is_open()) {
      throw std::runtime_error("Error: Cannot open blob file: " + path);
    }

    std::vector<char> compressedData((std::istreambuf_iterator<char>(blobFile)),
                                     std::istreambuf_iterator<char>());
    blobFile.close();

    std::string decompressedData = decompressZlib(compressedData);
    return decompressedData;
  } catch (const std::exception &e) {
    std::cerr << "Exception occurred at GitObjectStorage::readObject: "
              << e.what() << std::endl;
    return "";
  }
}

std::string GitObjectStorage::objectTypeToString(GitObjectType type) {
  switch (type) {
  case GitObjectType::Blob:
    return "blob";
  case GitObjectType::Tree:
    return "tree";
  case GitObjectType::Commit:
    return "commit";
  case GitObjectType::Tag:
    return "tag";
  default:
    return "unknown";
  }
}

GitObjectType
GitObjectStorage::parseGitObjectTypeFromString(const std::string &typeStr) {
  if (typeStr == "blob")
    return GitObjectType::Blob;
  if (typeStr == "tree")
    return GitObjectType::Tree;
  if (typeStr == "commit")
    return GitObjectType::Commit;
  if (typeStr == "tag")
    return GitObjectType::Tag;
  return GitObjectType::Unknown;
}

std::string GitObjectStorage::writeObject(const std::string &content) {
  try {
    if (content.empty()) {
      throw StorageException("Content cannot be empty");
    }

    std::string hash = hash_sha1(content);
    std::string compressed = compressZlib(content);

    std::string objDir = gitDir + "/objects/" + hash.substr(0, 2);
    if (!std::filesystem::exists(objDir)) {
      std::filesystem::create_directories(objDir);
    }

    std::string objPath = objDir + "/" + hash.substr(2);
    std::filesystem::create_directories(
        std::filesystem::path(objPath).parent_path());
    std::ofstream outFile(objPath, std::ios::binary);
    if (!outFile.is_open()) {
      throw StorageException("Failed to create object file: " + objPath);
    }

    outFile.write(compressed.data(), compressed.size());
    outFile.close();

    return hash;
  } catch (const std::exception &e) {
    std::cerr << "writeObject failed: " << e.what() << std::endl;
    return "";
  }
}

std::string GitObjectStorage::getObjectPath(const std::string &hash) const {
  // Standard git object storage path: .git/objects/xx/yyyy... where xx are
  // first 2 chars
  if (hash.size() < 2)
    return "";
  return gitDir + "/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
}
