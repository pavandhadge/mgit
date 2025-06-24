#pragma once
#include <cstdio>
#include <string>
#include <vector>

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

class GitObjectStorage{
  public :
  std::string writeObject( const std::string& content );

  std::string readObject(const std::string& hash);
  GitObjectType identifyType(const std::string &hash);
  GitObjectStorage(const std::string &gitDir = ".git");
  private :
  std::string gitDir;

      std::string objectTypeToString(GitObjectType type);
      GitObjectType parseGitObjectTypeFromString(const std::string& header);

};

class BlobObject : public GitObjectStorage {
private:
    GitObjectType type;
    BlobData content;
public:
    BlobObject();

    std::string writeObject(const std::string& path, const bool& write); // Reads file and stores blob
    BlobData readObject(const std::string& hash);                        // Reads blob from disk
    const BlobData& getContent() const;
    GitObjectType getType() const;
};

class TreeObject : public GitObjectStorage {
private:
    GitObjectType type;
    std::vector<TreeEntry> content;
public:
    TreeObject();
    bool restoreWorkingDirectoryFromTreeHash(const std::string &hash,const  std::string &path);
    std::string writeObject(const std::string& path);
    std::vector<TreeEntry> readObject(const std::string& hash);
    const std::vector<TreeEntry>& getContent() const;
    GitObjectType getType() const;
};



class CommitObject:public GitObjectStorage{
    private :
    GitObjectType type ;
    CommitData content;
    public :
    std::string writeObject(const CommitData& data) ;
    CommitData readObject(const std::string& hash);
  const CommitData& getContent() const;
     GitObjectType getType() const;
  CommitObject();
};

class TagObject:public GitObjectStorage{
    private :
    GitObjectType type ;
    TagData content;
    public :
    std::string writeObject(const TagData& data);
    const TagData getContent() const ;
       GitObjectType getType() const;

       TagData readObject(const std::string& hash);
  TagObject();
};
