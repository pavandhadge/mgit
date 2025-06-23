#pragma once
#include <string>
#include <map>

class GitCommit {
public:
    GitCommit(const std::string& hash, const std::string& treeHash, const std::string& parentHash = "")
        : hash(hash), treeHash(treeHash), parentHash(parentHash) {}

    std::string getHash() const { return hash; }
    std::string getTreeHash() const { return treeHash; }
    std::string getParentHash() const { return parentHash; }

private:
    std::string hash;
    std::string treeHash;
    std::string parentHash;
};
