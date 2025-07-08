#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <optional>
#include <filesystem>

class ObjectException : public std::exception {
public:
    explicit ObjectException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

enum class GitObjectType {
    Blob,
    Tree,
    Commit,
    Tag,
    Unknown
};

struct CommitData {
    std::string tree;
    std::vector<std::string> parents;
    std::string author;
    std::string committer;
    std::string message;
};

struct TagData {
    std::string objectHash;   // SHA of the object being tagged
    std::string objectType;   // "commit", "tree", "blob"
    std::string tagName;      // e.g., "v1.0"
    std::string tagger;       // Full tagger line: "Name <email> timestamp +timezone"
    std::string message;      // The tag message
};

// Struct for Blob content
struct BlobData {
    std::string content;
};

// Struct for Tree entry
struct TreeEntry {
    std::string mode;
    std::string filename;
    std::string hash;
};

class StorageException : public std::exception {
public:
    explicit StorageException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class GitObjectStorage {
public:
    explicit GitObjectStorage(const std::string& gitDir = ".git");
    
    // Core storage operations
    std::string readObject(const std::string& hash);
    bool writeObject(const std::string& hash, const std::string& content);
    std::string writeObject(const std::string& content);
    bool deleteObject(const std::string& hash);
    bool objectExists(const std::string& hash);
    
    // Validation and maintenance
    bool validateHash(const std::string& hash);
    bool validateObjectIntegrity(const std::string& hash);
    bool cleanupOrphanedObjects();
    bool compressObjects();
    
    // Utility methods
    std::string getObjectPath(const std::string& hash) const;
    std::vector<std::string> listAllObjects() const;
    size_t getObjectCount() const;

protected:
    const std::string& getGitDir() const { return gitDir; }

private:
    std::string gitDir;
    std::string objectsDir;
    std::string objectTypeToString(GitObjectType type);
    GitObjectType parseGitObjectTypeFromString(const std::string& header);
};
