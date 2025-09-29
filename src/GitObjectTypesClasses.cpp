#include "headers/GitObjectTypesClasses.hpp"
#include "headers/GitIndex.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/HashUtils.hpp"
#include "headers/ZlibUtils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

BlobObject::BlobObject(const std::string &gitDir)
    : GitObjectStorage(gitDir), type(GitObjectType::Blob), content({}) {}

std::string BlobObject::writeObject(const std::string &path,
                                    const bool &write) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Unable to open file: " << path << "\n";
    return "";
  }

  std::ostringstream data;
  data << file.rdbuf();
  std::string content = data.str();
  file.close();

  // Always create the Git object header for hash calculation
  std::string header = "blob " + std::to_string(content.size()) + '\0';
  std::string fullContent = header + content;

  if (write) {
    return GitObjectStorage::writeObject(fullContent);
  } else {
    return hash_sha1(fullContent);
  }
}

BlobData BlobObject::readObject(const std::string &hash) {
  BlobData data;
  std::string decompressed = GitObjectStorage::readObject(hash);

  size_t nullPos = decompressed.find('\0');
  if (nullPos == std::string::npos) {
    std::cerr << "Error: Invalid blob object (missing null byte).\n";
    return data;
  }

  data.content = decompressed.substr(nullPos + 1);
  content = data;
  return data;
}

const BlobData &BlobObject::getContent() const { return content; }

GitObjectType BlobObject::getType() const { return type; }

TreeObject::TreeObject(const std::string &gitDir)
    : GitObjectStorage(gitDir), type(GitObjectType::Tree), content({}) {}

std::string TreeObject::writeObject(const std::string &path) {
  std::vector<TreeEntry> entries;

  if (!std::filesystem::exists(path)) {
    std::cerr << "no such directory to make tree \n";
    return "";
  }

  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    TreeEntry treeEntry;
    treeEntry.filename = entry.path().filename();

    if (std::filesystem::is_regular_file(entry)) {
      treeEntry.mode = "100644";
      BlobObject blob(getGitDir());
      treeEntry.hash = blob.writeObject(entry.path().string(), true);
    } else if (std::filesystem::is_directory(entry)) {
      treeEntry.mode = "040000";
      TreeObject subTree(getGitDir());
      treeEntry.hash = subTree.writeObject(entry.path().string());
    } else {
      continue;
    }

    entries.push_back(treeEntry);
  }

  std::string binaryTreeContent;
  for (const auto &entry : entries) {
    binaryTreeContent += entry.mode + " " + entry.filename;
    binaryTreeContent.push_back('\0');
    for (int i = 0; i < 40; i += 2) {
      std::string bytestr = entry.hash.substr(i, 2);
      char byte = static_cast<char>(std::stoi(bytestr, nullptr, 16));
      binaryTreeContent += byte;
    }
  }

  std::string header =
      "tree " + std::to_string(binaryTreeContent.size()) + '\0';
  std::string full = header + binaryTreeContent;
  content = entries;
  return GitObjectStorage::writeObject(full);
}

std::vector<TreeEntry> TreeObject::readObject(const std::string &hash) {
  std::vector<TreeEntry> entries;
  std::string decompressed = GitObjectStorage::readObject(hash);

  size_t nullPos = decompressed.find('\0');
  if (nullPos == std::string::npos) {
    std::cerr << "Error: Invalid tree object (missing null byte).\n";
    return entries;
  }

  std::string treeBody = decompressed.substr(nullPos + 1);
  size_t i = 0;

  while (i < treeBody.size()) {
    TreeEntry entry;
    // Parse mode
    while (i < treeBody.size() && treeBody[i] != ' ')
      entry.mode += treeBody[i++];
    if (i >= treeBody.size())
      break;
    i++;
    // Parse filename
    while (i < treeBody.size() && treeBody[i] != '\0')
      entry.filename += treeBody[i++];
    if (i >= treeBody.size())
      break;
    i++;
    // Parse 20-byte hash
    if (i + 20 > treeBody.size())
      break;
    std::ostringstream hexHash;
    for (int j = 0; j < 20; ++j) {
      hexHash << std::hex << std::setw(2) << std::setfill('0')
              << (static_cast<unsigned int>(
                     static_cast<unsigned char>(treeBody[i + j])));
    }
    entry.hash = hexHash.str();
    i += 20;

    entries.push_back(entry);
  }

  content = entries;
  return entries;
}

const std::vector<TreeEntry> &TreeObject::getContent() const { return content; }

GitObjectType TreeObject::getType() const { return type; }

void TreeObject::restoreTreeContents(
    const std::string &hash, const std::string &path,
    std::unordered_set<std::string> &treePaths) {
  std::vector<TreeEntry> entities = readObject(hash);

  for (const TreeEntry &entity : entities) {
    std::string fullPath = path + "/" + entity.filename;
    std::string relativePath =
        std::filesystem::relative(fullPath, path).string();
    treePaths.insert(relativePath);

    if (entity.mode == "100644" || entity.mode == "100755") {
      // File (blob)
      BlobObject blobObj(getGitDir());
      BlobData blob = blobObj.readObject(entity.hash);

      std::filesystem::create_directories(
          std::filesystem::path(fullPath).parent_path());

      std::ofstream outFile(fullPath, std::ios::binary);
      outFile << blob.content;
      outFile.close();
    } else if (entity.mode == "040000") {
      // Directory (tree)
      std::filesystem::create_directories(fullPath);
      restoreTreeContents(entity.hash, fullPath, treePaths); // Recursive call
    }
  }
}

bool TreeObject::restoreWorkingDirectoryFromTreeHash(const std::string &hash,
                                                     const std::string &path) {
  std::unordered_set<std::string> treePaths;
  restoreTreeContents(hash, path, treePaths);
  for (auto &entry : std::filesystem::recursive_directory_iterator(path)) {
    std::string relativePath =
        std::filesystem::relative(entry.path(), path).string();
    if (relativePath == ".git")
      continue;
    if (treePaths.find(relativePath) == treePaths.end()) {
      std::filesystem::remove_all(entry.path());
    }
  }
  return true;
}

CommitObject::CommitObject(const std::string &gitDir)
    : GitObjectStorage(gitDir), type(GitObjectType::Commit), content({}) {}

std::string CommitObject::writeObject(const CommitData &data) {
  std::ostringstream commitText;
  commitText << "tree " << data.tree << "\n";
  for (const auto &parent : data.parents) {
    commitText << "parent " << parent << "\n";
  }
  commitText << "author " << data.author << "\n";
  commitText << "committer " << data.committer << "\n";
  commitText << "\n" << data.message << "\n";
  std::string commitContent = commitText.str();
  std::string header = "commit " + std::to_string(commitContent.size()) + '\0';
  std::string fullContent = header + commitContent;
  content = data;
  return GitObjectStorage::writeObject(fullContent);
}

CommitData CommitObject::readObject(const std::string &hash) {
  CommitData data;
  std::string decompressed = GitObjectStorage::readObject(hash);
  size_t nullPos = decompressed.find('\0');
  if (nullPos == std::string::npos) {
    std::cerr << "Error: Invalid commit object (missing null byte).\n";
    return data;
  }
  std::string body = decompressed.substr(nullPos + 1);
  std::istringstream iss(body);
  std::string line;
  bool inMessage = false;
  while (std::getline(iss, line)) {
    if (line.empty()) {
      inMessage = true;
      continue;
    }
    if (inMessage) {
      data.message += line + "\n";
      continue;
    }
    if (line.rfind("tree ", 0) == 0) {
      data.tree = line.substr(5);
    } else if (line.rfind("parent ", 0) == 0) {
      data.parents.push_back(line.substr(7));
    } else if (line.rfind("author ", 0) == 0) {
      data.author = line.substr(7);
    } else if (line.rfind("committer ", 0) == 0) {
      data.committer = line.substr(10);
    }
  }
  if (!data.message.empty() && data.message.back() == '\n') {
    data.message.pop_back();
  }
  content = data;
  return data;
}

const CommitData &CommitObject::getContent() const { return content; }

GitObjectType CommitObject::getType() const { return type; }

TagObject::TagObject(const std::string &gitDir)
    : GitObjectStorage(gitDir), type(GitObjectType::Tag), content({}) {}

std::string TagObject::writeObject(const TagData &data) {
  std::ostringstream tagStream;

  tagStream << "object " << data.objectHash << "\n";
  tagStream << "type " << data.objectType << "\n";
  tagStream << "tag " << data.tagName << "\n";
  tagStream << "tagger " << data.tagger << "\n";
  tagStream << "\n" << data.message << "\n";

  std::string body = tagStream.str(); // cache it
  content = data;

  std::string header = "tag " + std::to_string(body.size()) + '\0';
  std::string fullTag = header + body;

  return GitObjectStorage::writeObject(fullTag);
}

TagData TagObject::readObject(const std::string &hash) {
  TagData tag;
  std::string decompressed = GitObjectStorage::readObject(hash);

  size_t nullPos = decompressed.find('\0');
  if (nullPos == std::string::npos) {
    std::cerr << "Error: Invalid tag object (missing null byte).\n";
    return tag;
  }

  std::string body = decompressed.substr(nullPos + 1);
  std::istringstream iss(body);
  std::string line;
  bool inMessage = false;

  while (std::getline(iss, line)) {
    if (line.empty()) {
      inMessage = true;
      continue;
    }

    if (inMessage) {
      tag.message += line + "\n";
      continue;
    }

    if (line.rfind("object ", 0) == 0) {
      tag.objectHash = line.substr(7);
    } else if (line.rfind("type ", 0) == 0) {
      tag.objectType = line.substr(5);
    } else if (line.rfind("tag ", 0) == 0) {
      tag.tagName = line.substr(4);
    } else if (line.rfind("tagger ", 0) == 0) {
      tag.tagger = line.substr(7);
    }
  }

  // Remove trailing newline from message
  if (!tag.message.empty() && tag.message.back() == '\n') {
    tag.message.pop_back();
  }

  content = tag;
  return tag;
}

const TagData &TagObject::getContent() const { return content; }

GitObjectType TagObject::getType() const { return type; }

bool BlobObject::validateContent(const std::string &content) {
  try {
    if (content.empty()) {
      throw ObjectException("Blob content cannot be empty");
    }

    // Check for null bytes (which are not allowed in blob content)
    if (content.find('\0') != std::string::npos) {
      throw ObjectException("Blob content cannot contain null bytes");
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "BlobObject::validateContent failed: " << e.what()
              << std::endl;
    return false;
  }
}

bool TreeObject::validateEntry(const TreeEntry &entry) {
  try {
    if (entry.filename.empty()) {
      throw ObjectException("Tree entry filename cannot be empty");
    }

    if (entry.hash.empty()) {
      throw ObjectException("Tree entry hash cannot be empty");
    }

    if (entry.hash.size() != 40) {
      throw ObjectException("Invalid tree entry hash length: " +
                            std::to_string(entry.hash.size()));
    }

    if (!std::all_of(entry.hash.begin(), entry.hash.end(), ::isxdigit)) {
      throw ObjectException("Invalid tree entry hash format");
    }

    // Validate mode
    if (entry.mode != "100644" && entry.mode != "100755" &&
        entry.mode != "40000") {
      throw ObjectException("Invalid tree entry mode: " + entry.mode);
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "TreeObject::validateEntry failed: " << e.what() << std::endl;
    return false;
  }
}

bool CommitObject::validateCommit(const std::string &treeHash,
                                  const std::string &message) {
  try {
    if (treeHash.empty()) {
      throw ObjectException("Tree hash cannot be empty");
    }

    if (message.empty()) {
      throw ObjectException("Commit message cannot be empty");
    }

    if (treeHash.size() != 40) {
      throw ObjectException("Invalid tree hash length: " +
                            std::to_string(treeHash.size()));
    }

    if (!std::all_of(treeHash.begin(), treeHash.end(), ::isxdigit)) {
      throw ObjectException("Invalid tree hash format");
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "CommitObject::validateCommit failed: " << e.what()
              << std::endl;
    return false;
  }
}

bool BlobObject::updateContent(const std::string &newContent) {
  try {
    if (!validateContent(newContent)) {
      return false;
    }

    content.content = newContent;

    // Update storage
    GitObjectStorage storage;
    std::string header = "blob " + std::to_string(newContent.size()) + '\0';
    std::string fullContent = header + newContent;
    std::string hash = storage.writeObject(fullContent);
    if (hash.empty()) {
      throw ObjectException("Failed to update blob in storage");
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "BlobObject::updateContent failed: " << e.what() << std::endl;
    return false;
  }
}

bool TreeObject::addEntry(const TreeEntry &entry) {
  try {
    if (!validateEntry(entry)) {
      return false;
    }

    // Check for duplicate filename
    for (const auto &existingEntry : content) {
      if (existingEntry.filename == entry.filename) {
        throw ObjectException("Tree entry already exists: " + entry.filename);
      }
    }

    content.push_back(entry);

    // Re-serialize and update hash
    std::string newHash = writeObject("."); // This will recreate the tree
    if (newHash.empty()) {
      throw ObjectException("Failed to update tree after adding entry");
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "TreeObject::addEntry failed: " << e.what() << std::endl;
    return false;
  }
}

bool TreeObject::removeEntry(const std::string &filename) {
  try {
    if (filename.empty()) {
      throw ObjectException("Filename cannot be empty");
    }

    auto it = std::find_if(content.begin(), content.end(),
                           [&filename](const TreeEntry &entry) {
                             return entry.filename == filename;
                           });

    if (it == content.end()) {
      throw ObjectException("Tree entry not found: " + filename);
    }

    content.erase(it);

    // Re-serialize and update hash
    std::string newHash = writeObject("."); // This will recreate the tree
    if (newHash.empty()) {
      throw ObjectException("Failed to update tree after removing entry");
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "TreeObject::removeEntry failed: " << e.what() << std::endl;
    return false;
  }
}

void TreeObject_getAllFilesRecursive(
    TreeObject *self, const std::string &treeHash,
    const std::string &currentPath, std::map<std::string, std::string> &files) {
  std::vector<TreeEntry> entries = self->readObject(treeHash);
  for (const auto &entry : entries) {
    std::string path = currentPath.empty() ? entry.filename
                                           : currentPath + "/" + entry.filename;
    if (entry.mode == "040000") { // Tree
      TreeObject_getAllFilesRecursive(self, entry.hash, path, files);
    } else { // Blob
      files[path] = entry.hash;
    }
  }
}

void TreeObject::getAllFiles(const std::string &treeHash,
                             std::map<std::string, std::string> &files) {
  TreeObject_getAllFilesRecursive(this, treeHash, "", files);
}

std::string
TreeObject::writeTreeRecursive(const std::vector<IndexEntry> &entries) {
  std::map<std::string, std::vector<IndexEntry>> children;
  std::vector<IndexEntry> files;

  for (const auto &entry : entries) {
    size_t slashPos = entry.path.find('/');
    if (slashPos == std::string::npos) {
      files.push_back(entry);
    } else {
      std::string dir = entry.path.substr(0, slashPos);
      IndexEntry childEntry = entry;
      childEntry.path = entry.path.substr(slashPos + 1);
      children[dir].push_back(childEntry);
    }
  }

  std::string binaryTreeContent;

  // Process files
  for (const auto &file : files) {
    binaryTreeContent += file.mode + " " + file.path;
    binaryTreeContent.push_back('\0');
    std::string binHash = hexToBinary(file.hash);
    binaryTreeContent += binHash;
  }

  // Process directories
  for (const auto &[dirName, childEntries] : children) {
    std::string subTreeHash = writeTreeRecursive(childEntries);
    binaryTreeContent += "040000 " + dirName + "\0";
    std::string binHash = hexToBinary(subTreeHash);
    binaryTreeContent += binHash;
  }

  std::string header =
      "tree " + std::to_string(binaryTreeContent.size()) + '\0';
  std::string full = header + binaryTreeContent;
  return GitObjectStorage::writeObject(full);
}

std::string
TreeObject::writeTreeFromIndex(const std::vector<IndexEntry> &entries) {
  return writeTreeRecursive(entries);
}
