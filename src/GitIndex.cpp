#include "headers/GitIndex.hpp"
#include "headers/GitMerge.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitObjectTypesClasses.hpp"
#include "headers/HashUtils.hpp"
#include "headers/ZlibUtils.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

IndexManager::IndexManager(const std::string &gitDir) : gitDir(gitDir) {}

IndexEntry IndexManager::gitIndexEntryFromPath(const std::string &path) {
  IndexEntry newEntry;

  if (!std::filesystem::exists(path)) {
    std::cerr << "Error: File does not exist at path: " << path << "\n";
    return newEntry;
  }

  newEntry.path = path;
  newEntry.mode = std::filesystem::is_directory(path) ? "040000" : "100644";

  // Set default values for merge-related fields
  newEntry.base_hash = std::string(40, '0');
  newEntry.their_hash = std::string(40, '0');
  newEntry.conflict_state = ConflictState::NONE;
  newEntry.conflict_marker = "";

  // Read content for files
  if (std::filesystem::is_regular_file(path)) {
    std::ifstream curr_file(path, std::ios::binary);
    if (!curr_file.is_open()) {
      std::cerr << "Unable to open the file: " << path << std::endl;
      return newEntry;
    }

    std::ostringstream data;
    data << curr_file.rdbuf();
    std::string content = data.str();

    // Create blob object
    BlobObject blob(gitDir);
    newEntry.hash = blob.writeObject(path, true);

    curr_file.close();
  } else if (std::filesystem::is_directory(path)) {
    // Create tree object
    TreeObject subTree(gitDir);
    newEntry.hash = subTree.writeObject(path);
  } else {
    std::cerr << "Unsupported file type at path: " << path << "\n";
    return newEntry;
  }

  return newEntry;
}

bool IndexManager::readIndex() {
  std::string path = gitDir + "/index";
  if (!std::filesystem::exists(path)) {
    // Create an empty index file if it does not exist
    std::ofstream newIndex(path);
    if (!newIndex.is_open()) {
      std::cerr << "Error: Could not create index file at path: " << path
                << "\n";
      return false;
    }
    newIndex.close();
    entries.clear();
    pathToIndex.clear();
    conflictMarkers.clear();
    return true;
  }

  // Check if file is empty
  std::ifstream checkFile(path);
  checkFile.seekg(0, std::ios::end);
  if (checkFile.tellg() == 0) {
    // Empty index file is valid
    entries.clear();
    pathToIndex.clear();
    conflictMarkers.clear();
    checkFile.close();
    return true;
  }
  checkFile.close();

  std::ifstream index;
  try {
    index.exceptions(std::ifstream::badbit); // Only throw on true I/O errors
    index.open(path);

    entries.clear();
    pathToIndex.clear();
    conflictMarkers.clear();

    std::string line;
    size_t i = 0;
    bool anyValid = false;
    while (std::getline(index, line)) {
      int conflict_state_int;
      IndexEntry entry;
      std::vector<std::string> fields;
      std::string field;
      std::istringstream iss(line);
      while (std::getline(iss, field, '\t'))
        fields.push_back(field);
      if (fields.size() != 7) {
        throw std::runtime_error(
            "Index file is not in mgit format or is corrupt");
      }
      entry.mode = fields[0];
      entry.path = fields[1];
      entry.hash = fields[2];
      entry.base_hash = fields[3];
      entry.their_hash = fields[4];
      conflict_state_int = std::stoi(fields[5]);
      entry.conflict_marker = fields[6] == "-" ? "" : fields[6];
      entry.conflict_state = static_cast<ConflictState>(conflict_state_int);
      if (entry.hash.length() != 40 || entry.base_hash.length() != 40 ||
          entry.their_hash.length() != 40) {
        throw std::runtime_error(
            "Index file is not in mgit format or is corrupt");
      }
      entries.push_back(entry);
      pathToIndex[entry.path] = i++;
      anyValid = true;
    }
    index.close();
    if (!anyValid) {
      throw std::runtime_error("Index file is empty or not in mgit format");
    }
    return true;
  } catch (const std::ios_base::failure &e) {
    throw std::runtime_error("I/O error while reading INDEX file");
  }
}

void IndexManager::addOrUpdateEntry(const IndexEntry &entry) {
  auto it = pathToIndex.find(entry.path);
  if (it != pathToIndex.end()) {
    entries[it->second] = entry;
  } else {
    entries.push_back(entry);
    pathToIndex[entry.path] = entries.size() - 1;
  }
}

const std::vector<IndexEntry> &IndexManager::getEntries() const {
  return entries;
}

void IndexManager::printEntries() const {
  for (const auto &entry : entries) {
    std::cout << entry.mode << " " << entry.path << " " << entry.hash << " "
              << entry.base_hash << " " << entry.their_hash << " "
              << static_cast<int>(entry.conflict_state) << " "
              << entry.conflict_marker << "\n";
  }
}

void IndexManager::recordConflict(const std::string &path,
                                  const IndexEntry &base,
                                  const IndexEntry &ours,
                                  const IndexEntry &theirs) {
  IndexEntry conflictEntry = ours;
  conflictEntry.base_hash = base.hash;
  conflictEntry.their_hash = theirs.hash;
  conflictEntry.conflict_state = ConflictState::UNRESOLVED;
  //
  addOrUpdateEntry(conflictEntry);

  // Create conflict marker
  BlobObject blobObj(gitDir);
  std::string baseContent =
      base.hash.empty() ? "" : blobObj.readObject(base.hash).content;
  std::string ourContent =
      ours.hash.empty() ? "" : blobObj.readObject(ours.hash).content;
  std::string theirContent =
      theirs.hash.empty() ? "" : blobObj.readObject(theirs.hash).content;

  ConflictMarker marker = {baseContent, ourContent, theirContent};

  conflictMarkers[path] = marker;

  // Create conflict marker file
  std::string markerPath = path + ".mgit-conflict";
  std::ofstream markerFile(markerPath);
  markerFile << "<<<<<<< HEAD\n";
  markerFile << ourContent;
  markerFile << "=======\n";
  markerFile << theirContent;
  markerFile << ">>>>>>> " << theirs.path;
  markerFile.close();
}

bool IndexManager::hasConflicts() const {
  return std::any_of(entries.begin(), entries.end(),
                     [](const IndexEntry &entry) {
                       return entry.conflict_state == ConflictState::UNRESOLVED;
                     });
}

std::vector<std::string> IndexManager::getConflictingFiles() const {
  std::vector<std::string> result;
  for (const auto &entry : entries) {
    if (entry.conflict_state == ConflictState::UNRESOLVED) {
      result.push_back(entry.path);
    }
  }
  return result;
}

bool IndexManager::isConflicted(const std::string &path) const {
  auto it = pathToIndex.find(path);
  if (it != pathToIndex.end()) {
    return entries[it->second].conflict_state == ConflictState::UNRESOLVED;
  }
  return false;
}

bool IndexManager::resolveConflict(const std::string &path,
                                   const std::string &hash) {
  try {
    auto it = pathToIndex.find(path);
    if (it == pathToIndex.end()) {
      std::cerr << "Error: Path not found in index: " << path << "\n";
      return false;
    }
    IndexEntry &entry = entries[it->second];
    entry.hash = hash;
    entry.conflict_state = ConflictState::RESOLVED;
    entry.conflict_marker.clear();
    std::string markerPath = path + ".mgit-conflict";
    if (std::filesystem::exists(markerPath)) {
      std::filesystem::remove(markerPath);
    }
    BlobObject blobObj(gitDir);
    std::string content = blobObj.readObject(hash).content;
    std::ofstream file(path);
    file << content;
    file.close();
    return true;
  } catch (const std::exception &e) {
    std::cerr << "resolveConflict failed: " << e.what() << std::endl;
    return false;
  }
}

void IndexManager::abortMerge() {
  // Remove all conflict markers
  for (const auto &entry : entries) {
    if (entry.conflict_state == ConflictState::UNRESOLVED) {
      std::string markerPath = entry.path + ".mgit-conflict";
      if (std::filesystem::exists(markerPath)) {
        std::filesystem::remove(markerPath);
      }
    }
  }

  // Remove conflict entries from index
  entries.erase(std::remove_if(entries.begin(), entries.end(),
                               [](const IndexEntry &entry) {
                                 return entry.conflict_state ==
                                        ConflictState::UNRESOLVED;
                               }),
                entries.end());

  // Clear conflict markers map
  conflictMarkers.clear();

  // Rebuild pathToIndex map
  pathToIndex.clear();
  for (size_t i = 0; i < entries.size(); ++i) {
    pathToIndex[entries[i].path] = i;
  }

  // Write updated index
  writeIndex();
}

void IndexManager::addConflictMarker(const std::string &path,
                                     const ConflictMarker &marker) {
  conflictMarkers[path] = marker;
}

std::optional<ConflictMarker>
IndexManager::getConflictMarker(const std::string &path) const {
  auto it = conflictMarkers.find(path);
  if (it != conflictMarkers.end()) {
    return it->second;
  }
  return std::nullopt;
}

void IndexManager::writeIndex() {
  std::string path = gitDir + "/index";
  std::ofstream index;
  try {
    index.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    index.open(path);

    auto sanitize = [](const std::string &s) -> std::string {
      std::string out;
      for (char c : s) {
        if (c != '\n' && c != '\r' && c != '\t')
          out += c;
      }
      return out;
    };

    for (auto entry : entries) {
      entry.base_hash = std::string(40, '0');
      entry.their_hash = std::string(40, '0');
      std::string marker = entry.conflict_marker;
      marker.erase(0, marker.find_first_not_of(" \t\n\r"));
      marker.erase(marker.find_last_not_of(" \t\n\r") + 1);
      if (marker.empty())
        marker = "-";
      std::string mode = sanitize(entry.mode);
      std::string pathf = sanitize(entry.path);
      std::string hash = sanitize(entry.hash).substr(0, 40);
      std::string base_hash = sanitize(entry.base_hash).substr(0, 40);
      std::string their_hash = sanitize(entry.their_hash).substr(0, 40);
      std::string state =
          std::to_string(static_cast<int>(entry.conflict_state));
      std::string mark = sanitize(marker);
      std::string line = mode + "\t" + pathf + "\t" + hash + "\t" + base_hash +
                         "\t" + their_hash + "\t" + state + "\t" + mark;
      index << line << "\n";
    }
    index.close();
  } catch (const std::exception &e) {
    std::cerr << "Error writing index: " << e.what() << std::endl;
  }
}

StatusResult IndexManager::computeStatus(const std::string &headTreeHash) {
  StatusResult result;

  // 1. Get HEAD files
  std::map<std::string, std::string> headFiles;
  if (!headTreeHash.empty()) {
    TreeObject tree(gitDir);
    tree.getAllFiles(headTreeHash, headFiles);
  }

  // 2. Get Index files
  readIndex();
  std::map<std::string, std::string> indexFiles;
  for (const auto &entry : entries) {
    indexFiles[entry.path] = entry.hash;
  }

  // 3. Compare HEAD and Index for staged changes
  std::map<std::string, std::string> headFilesCopy = headFiles;
  for (const auto &[path, hash] : indexFiles) {
    auto it = headFilesCopy.find(path);
    if (it == headFilesCopy.end()) {
      result.staged_changes.push_back({"new file", path});
    } else {
      if (it->second != hash) {
        result.staged_changes.push_back({"modified", path});
      }
      headFilesCopy.erase(it);
    }
  }
  for (const auto &[path, hash] : headFilesCopy) {
    result.staged_changes.push_back({"deleted", path});
  }

  // 4. Compare Index and Working Directory for unstaged changes and untracked
  // files
  std::unordered_set<std::string> visited;
  for (auto it = std::filesystem::recursive_directory_iterator(
           ".", std::filesystem::directory_options::skip_permission_denied);
       it != std::filesystem::recursive_directory_iterator(); ++it) {
    const auto &file = *it;
    if (file.is_directory()) {
      if (file.path().filename() == ".git" ||
          file.path().filename() == ".mgit") {
        it.disable_recursion_pending();
      }
      continue;
    }

    std::string pathStr = file.path().lexically_normal().string();
    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
    if (pathStr.rfind("./", 0) == 0) {
      pathStr = pathStr.substr(2);
    }

    visited.insert(pathStr);

    auto index_it = indexFiles.find(pathStr);
    if (index_it == indexFiles.end()) {
      result.untracked_files.push_back(pathStr);
      continue;
    }

    BlobObject obj(gitDir);
    std::string currHash = obj.writeObject(pathStr, false);

    if (currHash != index_it->second) {
      result.unstaged_changes.push_back({"modified", pathStr});
    }
  }

  // Check for deleted files (in index but not in workdir)
  for (const auto &[path, hash] : indexFiles) {
    if (visited.find(path) == visited.end()) {
      if (!std::filesystem::exists(path)) {
        result.unstaged_changes.push_back({"deleted", path});
      }
    }
  }
  return result;
}

void IndexManager::resetFromTree(const std::string &treeHash) {
  entries.clear();
  pathToIndex.clear();
  conflictMarkers.clear();

  if (treeHash.empty()) {
    writeIndex(); // Write an empty index
    return;
  }

  TreeObject tree(gitDir);
  std::map<std::string, std::string> files;
  tree.getAllFiles(treeHash, files);

  for (const auto &[path, hash] : files) {
    IndexEntry entry;
    entry.mode = "100644"; // Assuming file, which is what getAllFiles returns
    entry.path = path;
    entry.hash = hash;
    entry.base_hash = std::string(40, '0');
    entry.their_hash = std::string(40, '0');
    entry.conflict_state = ConflictState::NONE;
    entry.conflict_marker = "";
    addOrUpdateEntry(entry);
  }
  writeIndex();
}
