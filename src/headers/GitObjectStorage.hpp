#pragma once
#include <string>

enum class GitObjectType {
    Blob,
        Tree,
        Commit,
        Tag,
        Unknown
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
    std::string content;
public:
    BlobObject();

    std::string writeObject(const std::string& path,const bool &write);   // Reads file and stores blob
    std::string readObject(const std::string& hash);    // Reads blob from disk
    const std::string& getContent() const;
    GitObjectType getType() const;
};


class CommitObject:public GitObjectStorage{
    private :
    GitObjectType type ;
    std::string content;
    public :
    std::string writeObject(const std::string& currentHash ,const std::string& parentHash ,const std::string &commitMassage,const std::string &author="");

  std::string readObject(const std::string& hash);
  const std::string& getContent() const;
     GitObjectType getType() const;
  CommitObject();
};
class TreeObject:public GitObjectStorage{
    private :
    GitObjectType type ;
    std::string content;
    public :
    std::string writeObject( const std::string& path);
    const std::string& getContent() const;
       GitObjectType getType() const;
    TreeObject();

  std::string readObject(const std::string& hash);
};
class TagObject:public GitObjectStorage{
    private :
    GitObjectType type ;
    std::string content;
    public :
    std::string writeObject(const std::string& targetHash,
                                       const std::string& targetType,
                                       const std::string& tagName,
                                       const std::string& tagMessage,
                                       const std::string& taggerLine);
    const std::string& getContent() const;
       GitObjectType getType() const;

  std::string readObject(const std::string& hash);
  TagObject();
};

/*
 * use following the blob or other codes
 *         std::ifstream file(path, std::ios::binary);
 if (!file.is_open()) {
     std::cerr << "Error: Unable to open file: " << path << "\n";
     return EXIT_FAILURE;
 }

 std::ostringstream buffer;
 buffer << file.rdbuf();
 std::string fileData = buffer.str();
 file.close();

 std::string blobHeader = "blob " + std::to_string(fileData.size()) + '\0';
 std::string fullBlob = blobHeader + fileData;
 *
 *
 */
