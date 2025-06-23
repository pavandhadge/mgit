#include "headers/GitObjectStorage.hpp"
#include "headers/GitRepository.hpp"
#include "headers/GitInit.hpp"
#include <iostream>
#include <vector>
#include <filesystem>
#include "headers/GitIndex.hpp"

GitRepository::GitRepository(const std::string& root) : gitDir(root) {}

void GitRepository::init(const std::string& path) {
    gitDir = path; // set gitDir first
    GitInit objInit(gitDir);
    objInit.run();
}

// ---------- Blob / Tree ----------
std::string GitRepository::writeObject(const GitObjectType type, const std::string& path , const bool &write) {
    if (type == GitObjectType::Blob) {
        BlobObject blob;
        return blob.writeObject(path , write);
    } else if (type == GitObjectType::Tree) {
        TreeObject tree;
        return tree.writeObject(path );
    } else {
        std::cerr << "Invalid object type for path-based creation\n";
        return "";
    }
}


// ---------- Commit ----------
std::string GitRepository::writeObject(GitObjectType type,const std::string& currentHash,
                         const std::string& parentHash,
                         const std::string& commitMessage,
                         const std::string& author ) {
                             if(type != GitObjectType::Commit){
                                 std::cerr<<"invalid object type used"<<std::endl;
                                 return "";
                             }
    CommitObject commit;
    return commit.writeObject(currentHash, parentHash, commitMessage, author);
}

// ---------- Tag ----------
std::string GitRepository::writeObject(GitObjectType type,const std::string& targetHash,
                         const std::string& targetType,
                         const std::string& tagName,
                         const std::string& tagMessage,
                         const std::string& taggerLine) {
                             if(type != GitObjectType::Tag){
                                 std::cerr<<"invalid object type used"<<std::endl;
                                 return "";
                             }
    TagObject tag;
    return tag.writeObject(targetHash, targetType, tagName, tagMessage, taggerLine);
}


std::string GitRepository::readObjectRaw(const std::string& path){
    GitObjectStorage obj;
    return obj.readObject(path);
}

 std::string GitRepository::readObject(const GitObjectType type , const std::string& hash){
     if(type == GitObjectType::Blob){
         BlobObject blob;
         return blob.readObject(hash);
     }else if(type == GitObjectType::Tree){
         TreeObject tree;
         return tree.readObject(hash);

     }else if(type == GitObjectType::Tag){
         TagObject tag ;
         return tag.readObject(hash);
     }else if(type==GitObjectType::Commit){
         CommitObject commit ;
         return commit.readObject(hash);
     }else {
         std::cerr<<"invalid object type "<<std::endl;
         return "";
     }
 }

 void GitRepository::indexHandler(const std::vector<std::string>& paths) {
     IndexManager idx;
     idx.readIndex();

     if (paths.size() == 1 && paths[0] == ".") {
         for (const auto& entry : std::filesystem::recursive_directory_iterator(".")) {
             if (entry.is_regular_file()) {
                 std::string pathStr = entry.path().string();
                 IndexEntry newEntry = idx.gitIndexEntryFromPath(pathStr);
                 if (!newEntry.hash.empty()) {
                     idx.addOrUpdateEntry(newEntry);
                 }
             }
         }
     } else {
         for (const std::string& path : paths) {
             if (!std::filesystem::exists(path)) {
                 std::cerr << "Error: Path does not exist: " << path << std::endl;
                 continue; // don't return, just skip this one
             }
             IndexEntry newEntry = idx.gitIndexEntryFromPath(path);
             if (!newEntry.hash.empty()) {
                 idx.addOrUpdateEntry(newEntry);
             }
         }
     }

     idx.writeIndex();
 }
