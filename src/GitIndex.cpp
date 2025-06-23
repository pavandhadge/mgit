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
#include "headers/GitIndex.hpp"
#include "headers/GitObjectStorage.hpp"
// #include <pair>
IndexEntry IndexManager::gitIndexEntryFromPath(const std::string &path) {
    IndexEntry newEntry;

    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: File does not exist at path: " << path << "\n";
        return newEntry;
    }

    newEntry.path = path;

    std::string hash;
    std::string mode;

    if (std::filesystem::is_regular_file(path)) {
        // Read content
        std::ifstream curr_file(path, std::ios::binary);
        if (!curr_file.is_open()) {
            std::cerr << "Unable to open the file: " << path << std::endl;
            return newEntry;
        }

        std::ostringstream data;
        data << curr_file.rdbuf();
        std::string content = data.str();

        // Set mode and write blob
        mode = "100644";
        BlobObject blob;
        bool write = true;
        hash = blob.writeObject(path, write);

        curr_file.close();
    } else if (std::filesystem::is_directory(path)) {
        // Set mode and write tree
        mode = "040000";
        TreeObject subTree;
        hash = subTree.writeObject(path);
    } else {
        std::cerr << "Unsupported file type at path: " << path << "\n";
        return newEntry;
    }

    newEntry.mode = mode;
    newEntry.hash = hash;
    return newEntry;
}

void IndexManager::readIndex() {
    std::string path = ".git/INDEX";
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: INDEX file does not exist at path: " << path << "\n";
        return;
    }

    std::ifstream index;
    try {
        index.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        index.open(path);

        entries.clear();
        pathToIndex.clear();

        std::string line;
        size_t i = 0;
        while (std::getline(index, line)) {
            std::istringstream iss(line);
            IndexEntry entry;

            if (!(iss >> entry.mode >> entry.path >> entry.hash)) {
                std::cerr << "Warning: Malformed index line skipped: " << line << "\n";
                continue;
            }

            if (entry.hash.length() != 40) {
                std::cerr << "Warning: Invalid hash length for path '" << entry.path << "'. Skipping entry.\n";
                continue;
            }

            entries.push_back(entry);
            pathToIndex[entry.path] = i++;
        }

        index.close();
    } catch (const std::ios_base::failure& e) {
        std::cerr << "I/O error while reading INDEX file: " << e.what() << "\n";
        if (index.is_open()) index.close();
    }
}

void IndexManager::writeIndex() {
    std::ofstream index;
    try {
        index.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        index.open(".git/INDEX", std::ios::out | std::ios::trunc);

        for (const auto& entry : entries) {
            index << entry.mode << " " << entry.path << " " << entry.hash << "\n";
        }

        index.close();
    } catch (const std::ios_base::failure& e) {
        std::cerr << "I/O error while writing INDEX file: " << e.what() << "\n";
        if (index.is_open()) index.close();
    }
}


const std::vector<IndexEntry>& IndexManager::getEntries()const {
    return entries;
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

void IndexManager::printEntries() const {
    std::cout << "Index entries:\n";
    for (const auto& entry : entries) {
        std::cout << "Mode: " << entry.mode
                  << ", Path: " << entry.path
                  << ", Hash (hex): " << binaryToHex(entry.hash) << "\n";
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

        BlobObject obj;
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
