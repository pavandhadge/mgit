#include "headers/GitMerge.hpp"
#include "headers/GitRepository.hpp"
#include "headers/GitObjectStorage.hpp"
#include <iostream>
#include <sstream>

std::ostream& operator<<(std::ostream& os, const ConflictStatus& status) {
    switch (status) {
        case ConflictStatus::NO_CONFLICT: return os << "No conflict";
        case ConflictStatus::CONTENT_CONFLICT: return os << "Content conflict";
        case ConflictStatus::DELETED_IN_OURS: return os << "Deleted in current branch";
        case ConflictStatus::DELETED_IN_THEIRS: return os << "Deleted in target branch";
        case ConflictStatus::TREE_CONFLICT: return os << "Directory conflict";
        case ConflictStatus::MODIFIED_IN_BOTH: return os << "Modified in both branches";
        case ConflictStatus::ADDED_IN_BOTH: return os << "Added in both branches";
        case ConflictStatus::RENAMED_IN_BOTH: return os << "Renamed in both branches";
        case ConflictStatus::RENAMED_IN_ONE: return os << "Renamed in one branch";
    }
    return os << "Unknown conflict";
}

GitMerge::GitMerge(const std::string& gitDir) : gitDir(gitDir) {}

bool GitMerge::checkForConflicts(const std::string& currentBranch, const std::string& targetBranch) {
    GitRepository repo(gitDir);
    
    // Get tree hashes for both branches
    std::string currentHead = repo.getHashOfBranchHead(currentBranch);
    std::string targetHead = repo.getHashOfBranchHead(targetBranch);
    
    if (currentHead.empty() || targetHead.empty()) {
        std::cerr << "Error: One or both branches have no commits\n";
        return false;
    }

    // Get tree objects from commits
    GitObjectStorage storage(gitDir);
    std::string currentTreeHash = storage.readObject(GitObjectType::Commit, currentHead).substr(5, 40); // Skip "tree " prefix
    std::string targetTreeHash = storage.readObject(GitObjectType::Commit, targetHead).substr(5, 40);

    // Find common ancestor (for three-way merge)
    std::string commonAncestor = repo.findCommonAncestor(currentHead, targetHead);
    std::string ancestorTreeHash = commonAncestor.empty() ? "" : storage.readObject(GitObjectType::Commit, commonAncestor).substr(5, 40);

    // Find conflicts
    conflicts.clear();
    conflictDetails.clear();
    findConflictsInTree(currentTreeHash, targetTreeHash);
    detectFileRenames(currentTreeHash, targetTreeHash);
    detectDirectoryConflicts(currentTreeHash, targetTreeHash);

    return !conflicts.empty();
}

std::vector<std::string> GitMerge::getConflictingFiles() {
    std::vector<std::string> result;
    for (const auto& pair : conflicts) {
        if (pair.second != ConflictStatus::NO_CONFLICT) {
            result.push_back(pair.first);
        }
    }
    return result;
}

ConflictStatus GitMerge::getFileConflictStatus(const std::string& filename) {
    auto it = conflicts.find(filename);
    return it != conflicts.end() ? it->second : ConflictStatus::NO_CONFLICT;
}

std::string GitMerge::getConflictDetails(const std::string& filename) {
    auto it = conflictDetails.find(filename);
    return it != conflictDetails.end() ? it->second : "";
}

bool GitMerge::compareTrees(const std::string& tree1, const std::string& tree2) {
    GitObjectStorage storage(gitDir);
    
    // Get tree entries
    std::vector<TreeEntry> entries1 = TreeObject().readObject(tree1);
    std::vector<TreeEntry> entries2 = TreeObject().readObject(tree2);

    // Compare entries
    for (const auto& entry1 : entries1) {
        bool found = false;
        for (const auto& entry2 : entries2) {
            if (entry1.filename == entry2.filename) {
                found = true;
                // If both are directories, compare recursively
                if (entry1.mode == "40000" && entry2.mode == "40000") {
                    if (!compareTrees(entry1.hash, entry2.hash)) {
                        conflicts[entry1.filename] = ConflictStatus::TREE_CONFLICT;
                        conflictDetails[entry1.filename] = "Directory structure differs";
                    }
                }
                // If both are files, compare contents
                else if (entry1.mode == "100644" && entry2.mode == "100644") {
                    if (!compareBlobs(entry1.hash, entry2.hash)) {
                        conflicts[entry1.filename] = ConflictStatus::CONTENT_CONFLICT;
                        conflictDetails[entry1.filename] = "File contents differ";
                    }
                }
                break;
            }
        }
        if (!found) {
            conflicts[entry1.filename] = ConflictStatus::DELETED_IN_THEIRS;
            conflictDetails[entry1.filename] = "File exists in current branch but deleted in target branch";
        }
    }

    // Check for files deleted in current branch
    for (const auto& entry2 : entries2) {
        bool found = false;
        for (const auto& entry1 : entries1) {
            if (entry1.filename == entry2.filename) {
                found = true;
                break;
            }
        }
        if (!found) {
            conflicts[entry2.filename] = ConflictStatus::DELETED_IN_OURS;
            conflictDetails[entry2.filename] = "File exists in target branch but deleted in current branch";
        }
    }

    return conflicts.empty();
}

bool GitMerge::compareBlobs(const std::string& blob1, const std::string& blob2) {
    GitObjectStorage storage(gitDir);
    
    // Get blob contents
    std::string content1 = storage.readObject(GitObjectType::Blob, blob1);
    std::string content2 = storage.readObject(GitObjectType::Blob, blob2);

    // Compare contents
    return content1 == content2;
}

void GitMerge::findConflictsInTree(const std::string& tree1, const std::string& tree2) {
    GitObjectStorage storage(gitDir);
    
    // Get tree entries
    std::vector<TreeEntry> entries1 = TreeObject().readObject(tree1);
    std::vector<TreeEntry> entries2 = TreeObject().readObject(tree2);

    // Compare entries
    for (const auto& entry1 : entries1) {
        bool found = false;
        for (const auto& entry2 : entries2) {
            if (entry1.filename == entry2.filename) {
                found = true;
                // If both are directories, compare recursively
                if (entry1.mode == "40000" && entry2.mode == "40000") {
                    findConflictsInTree(entry1.hash, entry2.hash);
                }
                // If both are files, compare contents
                else if (entry1.mode == "100644" && entry2.mode == "100644") {
                    if (!compareBlobs(entry1.hash, entry2.hash)) {
                        conflicts[entry1.filename] = ConflictStatus::CONTENT_CONFLICT;
                    }
                }
                break;
            }
        }
        if (!found) {
            conflicts[entry1.filename] = ConflictStatus::DELETED_IN_THEIRS;
        }
    }

    // Check for files deleted in current branch
    for (const auto& entry2 : entries2) {
        bool found = false;
        for (const auto& entry1 : entries1) {
            if (entry1.filename == entry2.filename) {
                found = true;
                break;
            }
        }
        if (!found) {
            conflicts[entry2.filename] = ConflictStatus::DELETED_IN_OURS;
        }
    }
}

void GitMerge::detectFileRenames(const std::string& tree1, const std::string& tree2) {
    GitObjectStorage storage(gitDir);
    std::vector<TreeEntry> entries1 = TreeObject().readObject(tree1);
    std::vector<TreeEntry> entries2 = TreeObject().readObject(tree2);

    // Create maps of filename -> hash for both trees
    std::unordered_map<std::string, std::string> fileHashes1;
    std::unordered_map<std::string, std::string> fileHashes2;

    for (const auto& entry : entries1) {
        if (entry.mode == "100644") {
            fileHashes1[entry.filename] = entry.hash;
        }
    }

    for (const auto& entry : entries2) {
        if (entry.mode == "100644") {
            fileHashes2[entry.filename] = entry.hash;
        }
    }

    // Check for renamed files
    for (const auto& pair1 : fileHashes1) {
        for (const auto& pair2 : fileHashes2) {
            if (pair1.second == pair2.second && pair1.first != pair2.first) {
                // Found a renamed file
                std::string details = "File renamed from " + pair1.first + " to " + pair2.first;
                conflicts[pair1.first] = ConflictStatus::RENAMED_IN_ONE;
                conflicts[pair2.first] = ConflictStatus::RENAMED_IN_ONE;
                conflictDetails[pair1.first] = details;
                conflictDetails[pair2.first] = details;
            }
        }
    }
}

void GitMerge::detectDirectoryConflicts(const std::string& tree1, const std::string& tree2) {
    GitObjectStorage storage(gitDir);
    std::vector<TreeEntry> entries1 = TreeObject().readObject(tree1);
    std::vector<TreeEntry> entries2 = TreeObject().readObject(tree2);

    // Create maps of directory name -> hash for both trees
    std::unordered_map<std::string, std::string> dirHashes1;
    std::unordered_map<std::string, std::string> dirHashes2;

    for (const auto& entry : entries1) {
        if (entry.mode == "40000") {
            dirHashes1[entry.filename] = entry.hash;
        }
    }

    for (const auto& entry : entries2) {
        if (entry.mode == "40000") {
            dirHashes2[entry.filename] = entry.hash;
        }
    }

    // Check for directory conflicts
    for (const auto& pair1 : dirHashes1) {
        for (const auto& pair2 : dirHashes2) {
            if (pair1.first == pair2.first && pair1.second != pair2.second) {
                // Found a directory conflict
                conflicts[pair1.first] = ConflictStatus::TREE_CONFLICT;
                std::string details = "Directory structure differs between branches";
                conflictDetails[pair1.first] = details;
            }
        }
    }
}
