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

GitMerge::GitMerge(const std::string& gitDir) {
    if (gitDir.empty()) {
        throw MergeException("Git directory path cannot be empty");
    }
    
    if (!std::filesystem::exists(gitDir)) {
        throw MergeException("Git directory does not exist: " + gitDir);
    }
    
    storage = std::make_unique<GitObjectStorage>(gitDir);
    index = std::make_unique<GitIndex>(gitDir);
}

GitMerge::~GitMerge() {
    // Ensure resources are properly cleaned up
    storage.reset();
    index.reset();
}

void GitMerge::validateCommit(const std::string& commitHash) {
    if (commitHash.empty()) {
        throw MergeException("Commit hash cannot be empty");
    }
    if (commitHash.size() != 40) {
        throw MergeException("Invalid commit hash length: " + std::to_string(commitHash.size()));
    }
    if (!std::all_of(commitHash.begin(), commitHash.end(), ::isxdigit)) {
        throw MergeException("Invalid commit hash format");
    }
}

void GitMerge::validateBranch(const std::string& branchName) {
    if (branchName.empty()) {
        throw MergeException("Branch name cannot be empty");
    }
    if (branchName.find("/") != std::string::npos) {
        throw MergeException("Branch name cannot contain slash characters");
    }
}

void GitMerge::validatePath(const std::string& path) {
    if (path.empty()) {
        throw MergeException("Path cannot be empty");
    }
    if (path.find("../") != std::string::npos) {
        throw MergeException("Path cannot contain parent directory references");
    }
}

std::string GitMerge::findCommonAncestor(const std::string& currentCommit, const std::string& targetCommit) {
    validateCommit(currentCommit);
    validateCommit(targetCommit);
    
    std::lock_guard<std::mutex> lock(mergeMutex);
    
    GitRepository repo(gitDir);
    std::unordered_set<std::string> currentBranchHistory = repo.logBranchCommitHistory(currentCommit);
    std::unordered_set<std::string> targetBranchHistory = repo.logBranchCommitHistory(targetCommit);
    
    std::string commonAncestor = "";
    for (const auto& commit : currentBranchHistory) {
        if (targetBranchHistory.find(commit) != targetBranchHistory.end()) {
            commonAncestor = commit;
            break;
        }
    }
    
    if (commonAncestor.empty()) {
        throw MergeException("No common ancestor found between commits");
    }
    return commonAncestor;
}

std::string GitMerge::findCommonAncestor(const std::string& currentCommit, const std::string& targetCommit) {
    GitRepository repo(gitDir);
    std::unordered_set<std::string> currentBranchHistory = repo.logBranchCommitHistory(currentCommit);
    std::unordered_set<std::string> targetBranchHistory = repo.logBranchCommitHistory(targetCommit);
    
    std::string commonAncestor = "";
    for (const auto& commit : currentBranchHistory) {
        if (targetBranchHistory.find(commit) != targetBranchHistory.end()) {
            commonAncestor = commit;
            break;
        }
    }
    
    if (commonAncestor.empty()) {
        throw std::runtime_error("No common ancestor found");
    }
    return commonAncestor;
}

std::string GitMerge::getBlobContent(const std::string& hash) {
    if (hash.empty()) return "";
    
    std::string content = storage.readObject(GitObjectType::Blob, hash);
    if (content.empty()) {
        throw std::runtime_error("Failed to read blob content");
    }
    return content;
}

std::string GitMerge::mergeFileContents(const std::string& baseContent, 
                                       const std::string& ourContent, 
                                       const std::string& theirContent) {
    std::stringstream result;
    
    if (baseContent == ourContent && ourContent == theirContent) {
        return ourContent;  // No conflict
    }
    
    if (baseContent == ourContent) {
        return theirContent;  // Their change
    }
    
    if (baseContent == theirContent) {
        return ourContent;  // Our change
    }
    
    // Conflict - create conflict markers
    result << "<<<<<<< HEAD\n"
            << ourContent << "\n=======\n"
            << theirContent << "\n>>>>>>> " << theirContent << "\n";
    
    return result.str();
}

bool GitMerge::threeWayMerge(const std::string& currentCommit, 
                            const std::string& targetCommit, 
                            const std::string& commonAncestor) {
    try {
        // Get tree hashes
        std::string currentTree = storage.readObject(GitObjectType::Commit, currentCommit).substr(5, 40);
        std::string targetTree = storage.readObject(GitObjectType::Commit, targetCommit).substr(5, 40);
        std::string ancestorTree = storage.readObject(GitObjectType::Commit, commonAncestor).substr(5, 40);
        
        // Merge trees
        std::string mergedTree = mergeTrees(currentTree, targetTree, ancestorTree);
        
        // Update index with merged tree
        GitIndex index(gitDir);
        index.updateIndexFromTree(mergedTree);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Three-way merge failed: " << e.what() << "\n";
        return false;
    }
}

std::string GitMerge::mergeTrees(const std::string& currentTree, 
                                const std::string& targetTree, 
                                const std::string& ancestorTree) {
    try {
        // Get tree objects
        std::string currentContent = storage.readObject(GitObjectType::Tree, currentTree);
        std::string targetContent = storage.readObject(GitObjectType::Tree, targetTree);
        std::string ancestorContent = storage.readObject(GitObjectType::Tree, ancestorTree);
        
        // Parse tree entries
        std::map<std::string, std::string> currentEntries, targetEntries, ancestorEntries;
        parseTreeEntries(currentContent, currentEntries);
        parseTreeEntries(targetContent, targetEntries);
        parseTreeEntries(ancestorContent, ancestorEntries);
        
        // Create new tree
        std::stringstream newTree;
        
        // Process each entry
        for (const auto& [path, currentHash] : currentEntries) {
            if (targetEntries.find(path) != targetEntries.end()) {
                // Both branches modified
                std::string baseHash = ancestorEntries[path];
                std::string ourContent = getBlobContent(currentHash);
                std::string theirContent = getBlobContent(targetEntries[path]);
                std::string baseContent = getBlobContent(baseHash);
                
                std::string mergedContent = mergeFileContents(baseContent, ourContent, theirContent);
                std::string mergedHash = storage.writeObject(GitObjectType::Blob, mergedContent);
                newTree << "100644 " << mergedHash << "\0" << path;
            } else if (ancestorEntries.find(path) == ancestorEntries.end()) {
                // Only current branch modified
                newTree << "100644 " << currentHash << "\0" << path;
            }
        }
        
        // Add new entries from target branch
        for (const auto& [path, targetHash] : targetEntries) {
            if (currentEntries.find(path) == currentEntries.end()) {
                newTree << "100644 " << targetHash << "\0" << path;
            }
        }
        
        // Create new tree object
        std::string treeHash = storage.writeObject(GitObjectType::Tree, newTree.str());
        return treeHash;
    } catch (const std::exception& e) {
        std::cerr << "Tree merge failed: " << e.what() << "\n";
        throw;
    }
}

void GitMerge::parseTreeEntries(const std::string& content, std::map<std::string, std::string>& entries) {
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        std::string mode, hash, path;
        std::istringstream iss(line);
        iss >> mode >> hash;
        path = line.substr(mode.size() + hash.size() + 2); // Skip mode, hash, and spaces
        entries[path] = hash;
    }
}

bool GitMerge::checkForConflicts(const std::string& currentBranch, const std::string& targetBranch) {
    validateBranch(currentBranch);
    validateBranch(targetBranch);
    
    std::lock_guard<std::mutex> lock(mergeMutex);
    
    GitRepository repo(gitDir);
    
    // Get tree hashes for both branches
    std::string currentHead = repo.getHashOfBranchHead(currentBranch);
    std::string targetHead = repo.getHashOfBranchHead(targetBranch);
    
    if (currentHead.empty() || targetHead.empty()) {
        throw MergeException("One or both branches have no commits");
    }

    // Get tree objects from commits
    std::string currentTreeHash = storage->readObject(GitObjectType::Commit, currentHead).substr(5, 40);
    validateTreeHash(currentTreeHash);
    
    std::string targetTreeHash = storage->readObject(GitObjectType::Commit, targetHead).substr(5, 40);
    validateTreeHash(targetTreeHash);

    // Find common ancestor (for three-way merge)
    std::string commonAncestor = repo.findCommonAncestor(currentHead, targetHead);
    std::string ancestorTreeHash = commonAncestor.empty() ? "" : storage->readObject(GitObjectType::Commit, commonAncestor).substr(5, 40);
    
    if (!ancestorTreeHash.empty()) {
        validateTreeHash(ancestorTreeHash);
    }

    // Find conflicts
    conflicts.clear();
    conflictDetails.clear();
    
    try {
        compareTreeEntries(currentTreeHash, targetTreeHash);
        detectFileRenames(currentTreeHash, targetTreeHash);
        detectDirectoryConflicts(currentTreeHash, targetTreeHash);
    } catch (const std::exception& e) {
        throw MergeException("Failed to detect conflicts: " + std::string(e.what()));
    }

    return !conflicts.empty();
}

void GitMerge::validateTreeHash(const std::string& treeHash) {
    if (treeHash.empty()) {
        throw MergeException("Tree hash cannot be empty");
    }
    if (treeHash.size() != 40) {
        throw MergeException("Invalid tree hash length: " + std::to_string(treeHash.size()));
    }
    if (!std::all_of(treeHash.begin(), treeHash.end(), ::isxdigit)) {
        throw MergeException("Invalid tree hash format");
    }
    
    // Check if tree exists
    if (storage->readObject(GitObjectType::Tree, treeHash).empty()) {
        throw MergeException("Tree object not found: " + treeHash);
    }
}

void GitMerge::validateBlobHash(const std::string& blobHash) {
    if (blobHash.empty()) {
        throw MergeException("Blob hash cannot be empty");
    }
    if (blobHash.size() != 40) {
        throw MergeException("Invalid blob hash length: " + std::to_string(blobHash.size()));
    }
    if (!std::all_of(blobHash.begin(), blobHash.end(), ::isxdigit)) {
        throw MergeException("Invalid blob hash format");
    }
    
    // Check if blob exists
    if (storage->readObject(GitObjectType::Blob, blobHash).empty()) {
        throw MergeException("Blob object not found: " + blobHash);
    }
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
