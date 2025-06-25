#include "headers/GitConfig.hpp"
#include "headers/GitHead.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitRepository.hpp"
#include "headers/GitInit.hpp"
#include <iostream>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include "headers/GitIndex.hpp"
#include "headers/GitBranch.hpp"
#include "headers/ZlibUtils.hpp"

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

bool GitRepository::isFullyMerged(const std::string& branchName) {
    // Get current branch and branch to check
    std::string currentBranch = getCurrentBranch();
    if (currentBranch == branchName) {
        std::cerr << "Cannot check merge status of current branch\n";
        return false;
    }

    // Get commit histories for both branches
    std::unordered_set<std::string> currentBranchHistory = logBranchCommitHistory(currentBranch);
    std::unordered_set<std::string> targetBranchHistory = logBranchCommitHistory(branchName);

    // Check if all commits from target branch are in current branch
    for (const auto& commit : targetBranchHistory) {
        if (currentBranchHistory.find(commit) == currentBranchHistory.end()) {
            // Found a commit in target branch that's not in current branch
            return false;
        }
    }

    return true;
}

void GitRepository::reportMergeConflicts(const std::string& targetBranch) {
    std::string currentBranch = getCurrentBranch();
    std::cerr << "Conflicts found during merge:\n";
    for (const auto& file : merge.getConflictingFiles()) {
        std::cerr << "- " << file << " (" << merge.getFileConflictStatus(file) << ")\n";
    }
    std::cerr << "\nResolve conflicts and run 'git merge --continue' when done\n";
}

bool GitRepository::mergeBranch(const std::string& targetBranch) {
    std::string currentBranch = getCurrentBranch();
    if (currentBranch == targetBranch) {
        std::cerr << "Cannot merge branch into itself\n";
        return false;
    }

    // Get current and target branch heads
    std::string currentHead = getHashOfBranchHead(currentBranch);
    std::string targetHead = getHashOfBranchHead(targetBranch);

    if (currentHead.empty() || targetHead.empty()) {
        std::cerr << "Error: One or both branches have no commits\n";
        return false;
    }

    // If fully merged, do fast-forward merge
    if (isFullyMerged(targetBranch)) {
        if (updateBranchHead(currentBranch, targetHead)) {
            std::cout << "Fast-forward merge successful\n";
            return true;
        }
        return false;
    }

    // Create merge commit
    std::string message = "Merge branch '" + targetBranch + "'";
    std::string author = GitConfig().getName() + " <" + GitConfig().getEmail() + "> " + getCurrentTimestampWithTimezone();
    std::string hash = createMergeCommit(message, author, currentHead, targetHead);

    if (hash.empty()) {
        std::cerr << "Error creating merge commit\n";
        return false;
    }

    // Update HEAD
    gitHead head;
    head.updateHead(hash);

    std::cout << "Merge successful. Created merge commit: " << hash << "\n";
    return true;
}

bool GitRepository::abortMerge() {
    IndexManager idx;
    idx.abortMerge();
    return true;
}

std::vector<std::string> GitRepository::getConflictingFiles() {
    IndexManager idx;
    return idx.getConflictingFiles();
}

bool GitRepository::isConflicted(const std::string& path) {
    IndexManager idx;
    return idx.isConflicted(path);
}

void GitRepository::resolveConflict(const std::string& path, const std::string& hash) {
    IndexManager idx;
    idx.resolveConflict(path, hash);
}

std::optional<ConflictMarker> GitRepository::getConflictMarker(const std::string& path) {
    IndexManager idx;
    return idx.getConflictMarker(path);
}

std::string GitRepository::createMergeCommit(const std::string& message, const std::string& author,
                                           const std::string& currentCommit, const std::string& targetCommit) {
    CommitData data;
    data.tree = writeObject(GitObjectType::Tree , ".", true);
    data.parents.push_back(currentCommit);
    data.parents.push_back(targetCommit);
    data.author = author;
    data.committer = author;
    data.message = message;

    return writeObject(GitObjectType::Commit, data);
}

bool GitRepository::resolveConflicts(const std::string& targetBranch) {
    IndexManager idx;
    std::vector<std::string> conflicts = idx.getConflictingFiles();
    
    if (conflicts.empty()) {
        return true;
    }

    std::cout << "Conflicts found:\n";
    for (const auto& file : conflicts) {
        std::cout << "- " << file << "\n";
    }

    std::cout << "\nResolve conflicts and run 'git merge --continue'\n";
    return false;
}


bool GitRepository::createCommit( const std::string& message,const std::string& author) {
    CommitData data;
    data.tree = writeObject(GitObjectType::Tree , ".",true);

    std::string parent= getHashOfBranchHead(getCurrentBranch());
    if (!parent.empty()) {
        data.parents.push_back(parent);
    }

    data.author = author.empty()
                ? GitConfig().getName() + " <" + GitConfig().getEmail() + "> " + getCurrentTimestampWithTimezone()
                : author;

    data.committer = GitConfig().getName() + " <" + GitConfig().getEmail() + "> " + getCurrentTimestampWithTimezone();
    data.message = message;

    std::string hash = writeObject(GitObjectType::Commit, data);
    std::cout << "Commit object written: " << hash << "\n";

    gitHead head;
    head.updateHead(hash);
    return true;
}

std::unordered_set<std::string> GitRepository::logBranchCommitHistory(const std::string &branchName){
    std::string currHash = getHashOfBranchHead(branchName);
    std::unordered_set<std::string> commitList;

    CommitObject commitObj;

    while(!currHash.empty()){
        commitList.insert(currHash);
        CommitData commit = commitObj.readObject(currHash);

        if(commit.parents.empty()) {
            break;
        }

        currHash = commit.parents[0]; // walk to first parent
    }

    return commitList;
}

bool GitRepository::gotoStateAtPerticularCommit(const std::string& hash) {
    std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    if (!std::filesystem::exists(path)) {
        std::cerr << "No such commit exists. Ensure it is part of the current branch.\n";
        return false;
    }

    std::unordered_set<std::string> commitListInCurrentBranch(
        logBranchCommitHistory(getCurrentBranch()).begin(),
        logBranchCommitHistory(getCurrentBranch()).end()
    );
    if (commitListInCurrentBranch.find(hash) == commitListInCurrentBranch.end()) {
        std::cerr << "Commit is not part of current branch history.\n";
        return false;
    }

    IndexManager idx;
    if (!idx.computeStatus().empty()) {
        std::cerr << "Untracked or modified changes exist. Please commit/stash them before reset.\n";
        return false;
    }

    CommitObject commitObj;
    CommitData commit = commitObj.readObject(hash);

    // Recursively clear current directory (except .git)
    for (auto& entry : std::filesystem::directory_iterator(".")) {
        if (entry.path().filename() != ".git") {
            std::filesystem::remove_all(entry);
        }
    }

    // Recursively checkout commit's tree into working directory
    // //yet to implement
    TreeObject treeObj;
    treeObj.restoreWorkingDirectoryFromTreeHash(commit.tree, ".");

    // Update HEAD
    gitHead head;
    head.updateHead(hash);

    std::cout << "Repository successfully reset to commit: " << hash << "\n";
    return true;
}

bool GitRepository::exportHeadAsZip(const std::string& branchName, const std::string& outputZipPath) {
    // Step 1: Get HEAD commit hash of the branch
    std::string commitHash = getHashOfBranchHead(branchName);
    if (commitHash.empty()) {
        std::cerr << "Branch '" << branchName << "' does not exist.\n";
        return false;
    }

    // Step 2: Get tree hash from the commit
    CommitObject commitObj;
    CommitData commitData = commitObj.readObject(commitHash);
    std::string treeHash = commitData.tree;

    // Step 3: Create a temporary export directory
    std::string tempDir = ".mgit_export_tmp";
    std::filesystem::remove_all(tempDir);  // Clean previous export if exists
    std::filesystem::create_directory(tempDir);

    // Step 4: Restore working tree using existing recursive function
    TreeObject treeObj;
    std::unordered_set<std::string> filePaths;
    treeObj.restoreTreeContents(treeHash, tempDir, filePaths);

    // Step 5: Zip the restored folder
    std::string zipCommand = "zip -r " + outputZipPath + " " + tempDir;
    int result = std::system(zipCommand.c_str());

    if (result != 0) {
        std::cerr << "Failed to create zip archive.\n";
        return false;
    }

    // Step 6: Cleanup
    std::filesystem::remove_all(tempDir);

    std::cout << "✅ Successfully exported HEAD of '" << branchName << "' to " << outputZipPath << "\n";
    return true;
}
