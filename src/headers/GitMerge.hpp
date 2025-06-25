#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "GitObjectStorage.hpp"
#include "GitObjectType.hpp"

enum class ConflictStatus {
    NO_CONFLICT,
    CONTENT_CONFLICT,
    DELETED_IN_OURS,
    DELETED_IN_THEIRS,
    TREE_CONFLICT,
    MODIFIED_IN_BOTH,
    ADDED_IN_BOTH,
    RENAMED_IN_BOTH,
    RENAMED_IN_ONE
};

std::ostream& operator<<(std::ostream& os, const ConflictStatus& status);

class GitMerge {
public:
    // Constructor
    GitMerge(const std::string& gitDir);

    // Check for conflicts between two branches
    bool checkForConflicts(const std::string& currentBranch, const std::string& targetBranch);

    // Get list of conflicting files
    std::vector<std::string> getConflictingFiles();

    // Get conflict status for a specific file
    ConflictStatus getFileConflictStatus(const std::string& filename);

    // Get detailed conflict information
    std::string getConflictDetails(const std::string& filename);

private:
    std::string gitDir;
    std::unordered_map<std::string, ConflictStatus> conflicts;
    std::unordered_map<std::string, std::string> conflictDetails;

    // Helper functions
    bool compareTrees(const std::string& tree1, const std::string& tree2);
    bool compareBlobs(const std::string& blob1, const std::string& blob2);
    void findConflictsInTree(const std::string& tree1, const std::string& tree2);
    void findConflictsInBlob(const std::string& blob1, const std::string& blob2, const std::string& filename);
    void detectFileRenames(const std::string& tree1, const std::string& tree2);
    void detectDirectoryConflicts(const std::string& tree1, const std::string& tree2);
};
