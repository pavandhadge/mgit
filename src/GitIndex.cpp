#include <fstream>
#include <iostream>
#include "headers/HashUtils.hpp"
#include "headers/ZlibUtils.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include "headers/GitIndex.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitMerge.hpp"
#include "headers/GitObjectTypesClasses.hpp"

IndexEntry IndexManager::gitIndexEntryFromPath(const std::string &path) {
    IndexEntry newEntry;

    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: File does not exist at path: " << path << "\n";
        return newEntry;
    }

    newEntry.path = path;
    newEntry.mode = std::filesystem::is_directory(path) ? "040000" : "100644";
    
    // Set default values for merge-related fields
    newEntry.base_hash = "0000000000000000000000000000000000000000";
    newEntry.their_hash = "0000000000000000000000000000000000000000";
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
        BlobObject blob(".git");
        newEntry.hash = blob.writeObject(path, true);

        curr_file.close();
    } else if (std::filesystem::is_directory(path)) {
        // Create tree object
        TreeObject subTree(".git");
        newEntry.hash = subTree.writeObject(path);
    } else {
        std::cerr << "Unsupported file type at path: " << path << "\n";
        return newEntry;
    }

    return newEntry;
}

bool IndexManager::readIndex() {
    std::string path = ".git/index";
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: INDEX file does not exist at path: " << path << "\n";
        return false;
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
        index.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        index.open(path);

        entries.clear();
        pathToIndex.clear();
        conflictMarkers.clear();

        std::string line;
        size_t i = 0;
        bool anyValid = false;
        while (std::getline(index, line)) {
            int conflict_state_int;
            std::istringstream iss(line);
            IndexEntry entry;
            if (!(iss >> entry.mode >> entry.path >> entry.hash >> entry.base_hash >> 
                  entry.their_hash >> conflict_state_int >> entry.conflict_marker)) {
                std::cerr << "Warning: Malformed index line skipped: " << line << "\n";
                continue;
            }
            entry.conflict_state = static_cast<ConflictState>(conflict_state_int);
            if (entry.hash.length() != 40 || entry.base_hash.length() != 40 || 
                entry.their_hash.length() != 40) {
                std::cerr << "Warning: Invalid hash length for path '" << entry.path << "'. Skipping entry.\n";
                continue;
            }
            entries.push_back(entry);
            pathToIndex[entry.path] = i++;
            anyValid = true;
        }
        index.close();
        if (!anyValid) {
            std::cerr << "Warning: Index file not in mgit format. Reinitializing as empty index.\n";
            std::ofstream clearIndex(path, std::ios::trunc);
            clearIndex.close();
            entries.clear();
            pathToIndex.clear();
            conflictMarkers.clear();
        }
        return true;
    } catch (const std::ios_base::failure& e) {
        std::cerr << "I/O error while reading INDEX file: " << e.what() << "\n";
        if (index.is_open()) index.close();
        // Reinitialize as empty index
        std::cerr << "Warning: Index file not in mgit format. Reinitializing as empty index.\n";
        std::ofstream clearIndex(path, std::ios::trunc);
        clearIndex.close();
        entries.clear();
        pathToIndex.clear();
        conflictMarkers.clear();
        return true;
    }
}

void IndexManager::addOrUpdateEntry(const IndexEntry& entry) {
    auto it = pathToIndex.find(entry.path);
    if (it != pathToIndex.end()) {
        entries[it->second] = entry;
    } else {
        entries.push_back(entry);
        pathToIndex[entry.path] = entries.size() - 1;
    }
}

const std::vector<IndexEntry>& IndexManager::getEntries() const {
    return entries;
}

void IndexManager::printEntries() const {
    for (const auto& entry : entries) {
        std::cout << entry.mode << " " << entry.path << " " << entry.hash << " "
                  << entry.base_hash << " " << entry.their_hash << " "
                  << static_cast<int>(entry.conflict_state) << " "
                  << entry.conflict_marker << "\n";
    }
}

void IndexManager::recordConflict(const std::string& path, const IndexEntry& base, 
                                 const IndexEntry& ours, const IndexEntry& theirs) {
    IndexEntry conflictEntry = ours;
    conflictEntry.base_hash = base.hash;
    conflictEntry.their_hash = theirs.hash;
    conflictEntry.conflict_state = ConflictState::UNRESOLVED;
    //
    addOrUpdateEntry(conflictEntry);
    
    // Create conflict marker
    BlobObject blobObj(".git");
    std::string baseContent = base.hash.empty() ? "" : blobObj.readObject(base.hash).content;
    std::string ourContent = ours.hash.empty() ? "" : blobObj.readObject(ours.hash).content;
    std::string theirContent = theirs.hash.empty() ? "" : blobObj.readObject(theirs.hash).content;
    
    ConflictMarker marker = {
        baseContent,
        ourContent,
        theirContent
    };
    
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
        [](const IndexEntry& entry) { return entry.conflict_state == ConflictState::UNRESOLVED; });
}

std::vector<std::string> IndexManager::getConflictingFiles() const {
    std::vector<std::string> result;
    for (const auto& entry : entries) {
        if (entry.conflict_state == ConflictState::UNRESOLVED) {
            result.push_back(entry.path);
        }
    }
    return result;
}

bool IndexManager::isConflicted(const std::string& path) const {
    auto it = pathToIndex.find(path);
    if (it != pathToIndex.end()) {
        return entries[it->second].conflict_state == ConflictState::UNRESOLVED;
    }
    return false;
}

bool IndexManager::resolveConflict(const std::string& path, const std::string& hash) {
    try {
        auto it = pathToIndex.find(path);
        if (it == pathToIndex.end()) {
            std::cerr << "Error: Path not found in index: " << path << "\n";
            return false;
        }
        IndexEntry& entry = entries[it->second];
        entry.hash = hash;
        entry.conflict_state = ConflictState::RESOLVED;
        entry.conflict_marker.clear();
        std::string markerPath = path + ".mgit-conflict";
        if (std::filesystem::exists(markerPath)) {
            std::filesystem::remove(markerPath);
        }
        BlobObject blobObj(".git");
        std::string content = blobObj.readObject(hash).content;
        std::ofstream file(path);
        file << content;
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "resolveConflict failed: " << e.what() << std::endl;
        return false;
    }
}

void IndexManager::abortMerge() {
    // Remove all conflict markers
    for (const auto& entry : entries) {
        if (entry.conflict_state == ConflictState::UNRESOLVED) {
            std::string markerPath = entry.path + ".mgit-conflict";
            if (std::filesystem::exists(markerPath)) {
                std::filesystem::remove(markerPath);
            }
        }
    }
    
    // Remove conflict entries from index
    entries.erase(std::remove_if(entries.begin(), entries.end(), 
        [](const IndexEntry& entry) { return entry.conflict_state == ConflictState::UNRESOLVED; }), 
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

void IndexManager::addConflictMarker(const std::string& path, const ConflictMarker& marker) {
    conflictMarkers[path] = marker;
}

std::optional<ConflictMarker> IndexManager::getConflictMarker(const std::string& path) const {
    auto it = conflictMarkers.find(path);
    if (it != conflictMarkers.end()) {
        return it->second;
    }
    return std::nullopt;
}

void IndexManager::writeIndex() {
    std::string path = ".git/index";
    std::ofstream index;
    try {
        index.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        index.open(path);

        for (const auto& entry : entries) {
            index << entry.mode << " " << entry.path << " " << entry.hash << " " 
                  << entry.base_hash << " " << entry.their_hash << " " 
                  << static_cast<int>(entry.conflict_state) << " " 
                  << entry.conflict_marker << "\n";
        }

        index.close();
    } catch (const std::ios_base::failure& e) {
        std::cerr << "I/O error while writing INDEX file: " << e.what() << "\n";
        if (index.is_open()) index.close();
    }
}

std::vector<std::pair<std::string, std::string>> IndexManager::computeStatus() {
    std::vector<std::pair<std::string, std::string>> changeRecords;

    // Step 1: Load index
    readIndex();

    std::unordered_map<std::string, std::string> indexedHashes;
    for (const auto& entry : entries) {
        indexedHashes[entry.path] = entry.hash;
    }

    std::unordered_set<std::string> visited;

    for (const auto& file : std::filesystem::recursive_directory_iterator(".", std::filesystem::directory_options::skip_permission_denied)) {
        std::string pathStr = file.path().string();

        // 🔥 Skip .git folder and its contents
        if (pathStr == ".git" ||
                    pathStr.substr(0, 5) == ".git/" ||
                    pathStr.find("/.git/") != std::string::npos ||
                    pathStr.find("\\.git\\") != std::string::npos) {
                    continue;
                }

        if (!file.is_regular_file()) continue;

        visited.insert(pathStr);

        auto it = indexedHashes.find(pathStr);
        if (it == indexedHashes.end()) {
            changeRecords.push_back({"untracked:", pathStr});
            continue;
        }

        BlobObject obj(".git");
        std::string currHash =obj.writeObject(pathStr, false);

        if (currHash != it->second) {
            changeRecords.push_back({"modified:", pathStr});
        }

        // Mark as handled
        indexedHashes.erase(it);
    }

    // Remaining = deleted
    for (const auto& [missingPath, _] : indexedHashes) {
        changeRecords.push_back({"deleted:", missingPath});
    }

    return changeRecords;
}
