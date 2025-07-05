#pragma once

#include <string>
#include <vector>
#include <memory>
#include "GitObjectStorage.hpp"

class GitObject {
public:
    GitObjectType type;
    std::string hash;
    std::string content;
    
    virtual ~GitObject() = default;
    virtual bool writeObject(const std::string& content) = 0;
    virtual std::string readObject(const std::string& hash) = 0;
};

class BlobObject : public GitObjectStorage {
private:
    GitObjectType type;
    BlobData content;
public:
    BlobObject(const std::string& gitDir);
    std::string writeObject(const std::string& path, const bool& write);
    bool validateContent(const std::string& content);
    bool updateContent(const std::string& newContent);
    BlobData readObject(const std::string& hash);
    const BlobData& getContent() const;
    GitObjectType getType() const;
};

class TreeObject : public GitObjectStorage {
private:
    GitObjectType type;
    std::vector<TreeEntry> content;
public:
    TreeObject(const std::string& gitDir);
    bool restoreWorkingDirectoryFromTreeHash(const std::string &hash, const std::string &path);
    std::string writeObject(const std::string& path);
    bool validateEntry(const TreeEntry& entry);
    bool addEntry(const TreeEntry& entry);
    bool removeEntry(const std::string& filename);
    std::vector<TreeEntry> readObject(const std::string& hash);
    const std::vector<TreeEntry>& getContent() const;
    GitObjectType getType() const;
    void restoreTreeContents(const std::string &hash, const std::string &path, std::unordered_set<std::string>& treePaths);
};

class CommitObject : public GitObjectStorage {
private:
    GitObjectType type;
    CommitData content;
public:
    CommitObject(const std::string& gitDir);
    std::string writeObject(const CommitData& data);
    bool validateCommit(const std::string& treeHash, const std::string& message);
    CommitData readObject(const std::string& hash);
    const CommitData& getContent() const;
    GitObjectType getType() const;
};

class TagObject : public GitObjectStorage {
private:
    GitObjectType type;
    TagData content;
public:
    TagObject(const std::string& gitDir);
    std::string writeObject(const TagData& data);
    const TagData& getContent() const;
    GitObjectType getType() const;
    TagData readObject(const std::string& hash);
}; 