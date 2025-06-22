#include <fstream>
#include <iostream>
#include "headers/ZlibUtils.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include "headers/GitIndex.hpp"



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
