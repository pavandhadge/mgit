#include "headers/GitHead.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitRepository.hpp"
#include "headers/GitInit.hpp"
#include <iostream>
#include <vector>
#include <filesystem>
#include "headers/GitIndex.hpp"
#include "headers/GitBranch.hpp"

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
std::string GitRepository::writeObject(GitObjectType type, const CommitData& data) {
    if (type != GitObjectType::Commit) {
        std::cerr << "Invalid object type used for commit\n";
        return "";
    }
    CommitObject commit;
    return commit.writeObject(data);
}


// ---------- Tag ----------
std::string GitRepository::writeObject(GitObjectType type, const TagData& data) {
    if (type != GitObjectType::Tag) {
        std::cerr << "Invalid object type used for tag\n";
        return "";
    }
    TagObject tag;
    return tag.writeObject(data);
}

std::string GitRepository::readObjectRaw(const std::string& path){
    GitObjectStorage obj;
    return obj.readObject(path);
}

std::string GitRepository::readObject(const GitObjectType type, const std::string& hash) {
    if (type == GitObjectType::Blob) {
        BlobObject blob;
        BlobData data = blob.readObject(hash);
        return data.content;
    } else if (type == GitObjectType::Tree) {
        TreeObject tree;
        std::vector<TreeEntry> entries = tree.readObject(hash);
        std::ostringstream out;
        for (const auto& entry : entries) {
            out << entry.mode << " " << entry.filename << " " << entry.hash << "\n";
        }
        return out.str();
    } else if (type == GitObjectType::Tag) {
        TagObject tag;
        TagData data = tag.readObject(hash);
        std::ostringstream out;
        out << "Object: " << data.objectHash << "\n"
            << "Type: " << data.objectType << "\n"
            << "Tag: " << data.tagName << "\n"
            << "Tagger: " << data.tagger << "\n"
            << "Message: " << data.message << "\n";
        return out.str();
    } else if (type == GitObjectType::Commit) {
        CommitObject commit;
        CommitData data = commit.readObject(hash);
        std::ostringstream out;
        out << "Tree: " << data.tree << "\n";
        for (const auto& parent : data.parents) {
            out << "Parent: " << parent << "\n";
        }
        out << "Author: " << data.author << "\n"
            << "Committer: " << data.committer << "\n"
            << "Message: " << data.message << "\n";
        return out.str();
    } else {
        std::cerr << "Invalid object type\n";
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

void GitRepository::reportStatus(bool shortFormat , bool showUntracked ) {
     IndexManager idx;
     auto changes = idx.computeStatus(); // your function returning vector<pair<string, string>>

     std::ostringstream out;

     for (const auto& [status, path] : changes) {
         if (!showUntracked && status == "untracked:") {
             continue;  // skip untracked files if flag is off
         }
         if (shortFormat) {
             // Print short status codes, like 'M ', '?? '
             if (status == "modified:") out << "M ";
             else if (status == "untracked:") out << "?? ";
             else if (status == "deleted:") out << "D ";
             else out << "  "; // unknown, print blank
             out << path << "\n";
         } else {
             // Long format: full words
             out << status << " " << path << "\n";
         }
     }

     std::cout<<out.str()<<std::endl;
}

void GitRepository::CreateBranch(const std::string &branchName){
    Branch branchObj;
    branchObj.createBranch(branchName);
}
void GitRepository::listbranches(const std::string &branchName){
    Branch branchObj;
    std::vector<std::string> branchList = branchObj.listBranches();
    std::string currBranch = branchObj.getCurrentBranch();

    for(const std::string branch : branchList){
        if(branch==currBranch){
            std::cout<<"*";
        }
        std::cout<<branch<<std::endl;
    }
}
std::string GitRepository::getCurrentBranch()const {
    Branch branchObj;
    return branchObj.getCurrentBranch();
}

void GitRepository::changeCurrentBranch(const std::string &targetBranch , bool createflag){
    if(createflag){
        CreateBranch(targetBranch);
    }
    //alot to do here but currently we wait
    Branch branchObj;
    gitHead head;
    head.writeHeadToHeadOfNewBranch(targetBranch);

}

std::string GitRepository::getHashOfBranchHead(const std::string &branchName){
     Branch branchObj;
    return branchObj.getBranchHash(branchName);
}

bool GitRepository::deleteBranch(const std::string &branchName){
    Branch branchObj;
    return branchObj.deleteBranch(branchName);
}

bool GitRepository::renameBranch(const std::string& oldName, const std::string& newName){
    Branch branchObj;
    return branchObj.renameBranch(oldName,newName);
}

bool GitRepository::isFullyMerged(const std::string& branchName){
    return true;
}
