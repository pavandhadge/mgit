#include "headers/GitBranch.hpp"
#include "headers/GitConfig.hpp"
#include "headers/GitHead.hpp"
#include "headers/GitIndex.hpp"
#include "headers/GitInit.hpp"
#include "headers/GitMerge.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitRepository.hpp"
#include "headers/ZlibUtils.hpp"
#include "headers/GitObjectTypesClasses.hpp"
#include <exception>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

GitRepository::GitRepository(const std::string &root) : gitDir(root), merge(std::make_unique<GitMerge>(gitDir)) {}

bool GitRepository::init(const std::string &path) {
  try {
    gitDir = path;
    GitInit objInit(gitDir);
    objInit.run();
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Init failed: " << e.what() << std::endl;
    return false;
  }
}

// ---------- Blob / Tree ----------
std::string GitRepository::writeObject(const GitObjectType type,
                                       const std::string &path,
                                       const bool &write) {
  if (type == GitObjectType::Blob) {
    BlobObject blob(gitDir);
    return blob.writeObject(path, write);
  } else if (type == GitObjectType::Tree) {
    TreeObject tree(gitDir);
    return tree.writeObject(path);
  } else {
    std::cerr << "Invalid object type for path-based creation\n";
    return "";
  }
}

// ---------- Commit ----------
std::string GitRepository::writeObject(GitObjectType type,
                                       const CommitData &data) {
  if (type != GitObjectType::Commit) {
    std::cerr << "Invalid object type used for commit\n";
    return "";
  }
  CommitObject commit(gitDir);
  return commit.writeObject(data);
}

// ---------- Tag ----------
std::string GitRepository::writeObject(GitObjectType type,
                                       const TagData &data) {
  if (type != GitObjectType::Tag) {
    std::cerr << "Invalid object type used for tag\n";
    return "";
  }
  TagObject tag(gitDir);
  return tag.writeObject(data);
}

std::string GitRepository::readObjectRaw(const std::string &path) {
  GitObjectStorage obj(gitDir);
  return obj.readObject(path);
}

std::string GitRepository::readObject(const GitObjectType type,
                                      const std::string &hash) {
  if (type == GitObjectType::Blob) {
    BlobObject blob(gitDir);
    BlobData data = blob.readObject(hash);
    return data.content;
  } else if (type == GitObjectType::Tree) {
    TreeObject tree(gitDir);
    std::vector<TreeEntry> entries = tree.readObject(hash);
    std::ostringstream out;
    for (const auto &entry : entries) {
      out << entry.mode << " " << entry.filename << " " << entry.hash << "\n";
    }
    return out.str();
  } else if (type == GitObjectType::Tag) {
    TagObject tag(gitDir);
    TagData data = tag.readObject(hash);
    std::ostringstream out;
    out << "Object: " << data.objectHash << "\n"
        << "Type: " << data.objectType << "\n"
        << "Tag: " << data.tagName << "\n"
        << "Tagger: " << data.tagger << "\n"
        << "Message: " << data.message << "\n";
    return out.str();
  } else if (type == GitObjectType::Commit) {
    CommitObject commit(gitDir);
    CommitData data = commit.readObject(hash);
    std::ostringstream out;
    out << "Tree: " << data.tree << "\n";
    for (const auto &parent : data.parents) {
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

void GitRepository::indexHandler(const std::vector<std::string> &paths) {
  IndexManager idx(gitDir);
  if (!idx.readIndex()) {
    std::cerr << "Failed to read index." << std::endl;
    return;
  }

  if (paths.size() == 1 && paths[0] == ".") {
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(".")) {
      if (entry.is_regular_file()) {
        std::string pathStr = entry.path().string();
        IndexEntry newEntry = idx.gitIndexEntryFromPath(pathStr);
        if (!newEntry.hash.empty()) {
          idx.addOrUpdateEntry(newEntry);
        }
      }
    }
  } else {
    for (const std::string &path : paths) {
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

bool GitRepository::reportStatus(bool shortFormat, bool showUntracked) {
  try {
    IndexManager idx(gitDir);
    std::vector<std::pair<std::string, std::string>> changes = idx.computeStatus();
    std::ostringstream out;
    // Print branch info like git
    out << "On branch " << getCurrentBranch() << "\n";
    if (changes.empty()) {
      out << "nothing to commit, working tree clean\n";
    } else {
      bool hasStaged = false;
      bool hasUnstaged = false;
      for (const auto& change : changes) {
        if (change.first == "staged") {
          if (!hasStaged) {
            out << "\nChanges to be committed:\n";
            hasStaged = true;
          }
          out << "  modified: " << change.second << "\n";
        } else if (change.first == "unstaged") {
          if (!hasUnstaged) {
            out << "\nChanges not staged for commit:\n";
            hasUnstaged = true;
          }
          out << "  modified: " << change.second << "\n";
        }
      }
    }
    std::cout << out.str();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Status failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitRepository::CreateBranch(const std::string &branchName) {
  try {
    Branch branchObj;
    return branchObj.createBranch(branchName);
  } catch (const std::exception &e) {
    std::cerr << "CreateBranch failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitRepository::listbranches(const std::string& branchName) {
    try {
        Branch branchObj;
        if (!branchObj.listBranches()) {
            throw std::runtime_error("Failed to list branches");
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "listbranches failed: " << e.what() << std::endl;
        return false;
    }
}

std::string GitRepository::getCurrentBranch() const {
  Branch branchObj;
  return branchObj.getCurrentBranch();
}

bool GitRepository::changeCurrentBranch(const std::string &targetBranch, bool createflag) {
  try {
    if (createflag) {
      if (!CreateBranch(targetBranch)) return false;
    }
    Branch branchObj;
    gitHead head;
    head.writeHeadToHeadOfNewBranch(targetBranch);
    // After switching, restore working directory to latest commit on branch
    std::string latest = getHashOfBranchHead(targetBranch);
    if (!latest.empty()) {
      gotoStateAtPerticularCommit(latest);
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "changeCurrentBranch failed: " << e.what() << std::endl;
    return false;
  }
}

std::string GitRepository::getHashOfBranchHead(const std::string &branchName) {
  Branch branchObj;
  return branchObj.getBranchHash(branchName);
}

bool GitRepository::deleteBranch(const std::string &branchName) {
  Branch branchObj;
  return branchObj.deleteBranch(branchName);
}

bool GitRepository::renameBranch(const std::string &oldName,
                                 const std::string &newName) {
  Branch branchObj;
  return branchObj.renameBranch(oldName, newName);
}

bool GitRepository::exportHeadAsZip(const std::string &branchName,
                                    const std::string &outputZipPath) {
  // Step 1: Get HEAD commit hash of the branch
  std::string commitHash = getHashOfBranchHead(branchName);
  if (commitHash.empty()) {
    std::cerr << "Branch '" << branchName << "' does not exist.\n";
    return false;
  }

  // Step 2: Get tree hash from the commit
  CommitObject commitObj(gitDir);
  CommitData commitData = commitObj.readObject(commitHash);
  std::string treeHash = commitData.tree;

  // Step 3: Create a temporary export directory
  std::string tempDir = ".mgit_export_tmp";
  std::filesystem::remove_all(tempDir); // Clean previous export if exists
  std::filesystem::create_directory(tempDir);

  // Step 4: Restore working tree using existing recursive function
  TreeObject treeObj(gitDir);
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

  std::cout << "✅ Successfully exported HEAD of '" << branchName << "' to "
            << outputZipPath << "\n";
  return true;
}

// this is the new part uk
bool GitRepository::isFullyMerged(const std::string &branchName) {
  // Get current branch and branch to check
  std::string currentBranch = getCurrentBranch();
  if (currentBranch == branchName) {
    std::cerr << "Cannot check merge status of current branch\n";
    return false;
  }

  // Check for conflicts first
  GitMerge merge(gitDir);
  if (merge.checkForConflicts(currentBranch, branchName)) {
    std::cerr << "The branch '" << branchName
              << "' has conflicts with the current branch\n";
    std::cerr << "Conflicting files:\n";
    for (const auto &file : merge.getConflictingFiles()) {
      std::cerr << "- " << file << " (" << merge.getFileConflictStatus(file)
                << ")\n";
    }
    return false;
  }

  // Get commit histories for both branches
  std::unordered_set<std::string> currentBranchHistory =
      logBranchCommitHistory(currentBranch);
  std::unordered_set<std::string> targetBranchHistory =
      logBranchCommitHistory(branchName);

  // Check if all commits from target branch are in current branch
  for (const auto &commit : targetBranchHistory) {
    if (currentBranchHistory.find(commit) == currentBranchHistory.end()) {
      // Found a commit in target branch that's not in current branch
      return false;
    }
  }

  return true;
}

bool GitRepository::reportMergeConflicts(const std::string &targetBranch) {
  try {
    if (!merge) {
      std::cout << "No merge in progress or no conflicts to report.\n";
      return false;
    }
    std::string currentBranch = getCurrentBranch();
    std::vector<std::string> conflicts = merge->getConflictingFiles();
    if (conflicts.empty()) {
      std::cout << "No conflicts found during merge.\n";
      return true;
    }
    std::cerr << "Conflicts found during merge:\n";
    for (const auto &file : conflicts) {
      std::cerr << "- " << file << " (" << merge->getFileConflictStatus(file)
                << ")\n";
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "reportMergeConflicts failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitRepository::mergeBranch(const std::string &targetBranch) {
  std::lock_guard<std::mutex> lock(mergeMutex); // Lock for thread safety

  // Validate inputs
  if (targetBranch.empty()) {
    throw std::invalid_argument("Branch name cannot be empty");
  }

  try {
    std::string currentBranch = getCurrentBranch();
    if (currentBranch == targetBranch) {
      throw std::runtime_error("Cannot merge branch into itself");
    }

    // Get current and target branch heads
    std::string currentHead = getHashOfBranchHead(currentBranch);
    std::string targetHead = getHashOfBranchHead(targetBranch);

    if (currentHead.empty() || targetHead.empty()) {
      throw std::runtime_error("One or both branches have no commits");
    }

    // Check for conflicts
    if (merge->checkForConflicts(currentBranch, targetBranch)) {
      reportMergeConflicts(targetBranch);
      return false; // Return false but don't throw to allow conflict resolution
    }

    // If fully merged, do fast-forward merge
    if (isFullyMerged(targetBranch)) {
      if (gitHead().updateHead(targetHead)) { // make the update bool func
        std::cout << "Fast-forward merge successful\n";
        return true;
      }
      std::cerr << "Failed to update branch head\n" << std::endl;
    }

    // Require user.name and user.email for merge commit
    GitConfig config(GitConfig::findGitDir());
    std::string userName = config.getUserName();
    std::string userEmail = config.getUserEmail();
    if (userName == "Your Name") {
      std::cerr << "fatal: unable to auto-detect name (user.name not set)\n";
      return false;
    }
    if (userEmail == "your@email.com") {
      std::cerr << "fatal: unable to auto-detect email address (user.email not set)\n";
      return false;
    }

    // Create merge commit
    std::string message = "Merge branch '" + targetBranch + "'";
    std::string author = userName + " <" + userEmail + "> " + getCurrentTimestampWithTimezone();

    // Get tree from index
    std::string treeHash = writeObject(GitObjectType::Tree, ".", true);
    IndexManager idx(gitDir);
    idx.writeIndex();
    if (treeHash.empty()) {
      throw std::runtime_error("Failed to write tree object");
    }

    // Create commit data
    CommitData data;
    data.tree = treeHash;
    data.parents.push_back(currentHead);
    data.parents.push_back(targetHead);
    data.author = author;
    data.committer = author;
    data.message = message;

    // Create commit object
    std::string hash = writeObject(GitObjectType::Commit, data);
    if (hash.empty()) {
      throw std::runtime_error("Failed to create merge commit");
    }

    // Update HEAD
    gitHead head;
    if (!head.updateHead(hash)) {
      throw std::runtime_error("Failed to update HEAD");
    }

    // Update branch head
    if (!Branch().updateBranchHead(currentBranch, hash)) {
      throw std::runtime_error("Failed to update branch head");
    }

    std::cout << "Merge successful. Created merge commit: " << hash << "\n";
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Merge failed: " << e.what() << "\n";
    return false;
  }
}

bool GitRepository::resolveConflicts(const std::string &targetBranch) {
  IndexManager idx(gitDir);
  std::vector<std::string> conflicts = idx.getConflictingFiles();

  if (conflicts.empty()) {
    return true;
  }

  std::cout << "Conflicts found:\n";
  for (const auto &file : conflicts) {
    std::cout << "- " << file << "\n";
  }

  std::cout << "\nResolve conflicts and run 'git merge --continue'\n";
  return false;
}

std::string GitRepository::createMergeCommit(const std::string &message,
                                             const std::string &author,
                                             const std::string &currentCommit,
                                             const std::string &targetCommit) {
  // Validate inputs
  if (message.empty() || currentCommit.empty() || targetCommit.empty()) {
    std::cerr << "Error: Invalid merge commit parameters\n";
    return "";
  }

  // Create commit data
  CommitData data;
  data.tree = writeObject(GitObjectType::Tree, ".", true);
  if (data.tree.empty()) {
    std::cerr << "Error: Failed to create tree object\n";
    return "";
  }

  // Add both parents
  data.parents.push_back(currentCommit);
  data.parents.push_back(targetCommit);
  data.author = author;
  data.committer = author;
  data.message = message;

  // Create commit object
  std::string hash = writeObject(GitObjectType::Commit, data);
  if (hash.empty()) {
    std::cerr << "Error: Failed to create merge commit\n";
    return "";
  }

  return hash;
}

bool GitRepository::abortMerge() {
  IndexManager idx(gitDir);
  idx.abortMerge();
  return true;
}

std::vector<std::string> GitRepository::getConflictingFiles() {
  IndexManager idx(gitDir);
  return idx.getConflictingFiles();
}

bool GitRepository::isConflicted(const std::string &path) {
  IndexManager idx(gitDir);
  return idx.isConflicted(path);
}

bool GitRepository::resolveConflict(const std::string &path, const std::string &hash) {
  try {
    IndexManager idx(gitDir);
    return idx.resolveConflict(path, hash);
  } catch (const std::exception &e) {
    std::cerr << "resolveConflict failed: " << e.what() << std::endl;
    return false;
  }
}

std::optional<ConflictMarker>
GitRepository::getConflictMarker(const std::string &path) {
  IndexManager idx(gitDir);
  return idx.getConflictMarker(path);
}

// new part ends here alright
bool GitRepository::createCommit(const std::string &message,
                                 const std::string &author) {
  GitConfig config(GitConfig::findGitDir());
  std::string userName = config.getUserName();
  std::string userEmail = config.getUserEmail();
  if (userName == "Your Name") {
    std::cerr << "fatal: unable to auto-detect name (user.name not set)\n";
    return false;
  }
  if (userEmail == "your@email.com") {
    std::cerr << "fatal: unable to auto-detect email address (user.email not set)\n";
    return false;
  }
  CommitData data;
  data.tree = writeObject(GitObjectType::Tree, ".", true);
  std::string parent = getHashOfBranchHead(getCurrentBranch());
  if (!parent.empty()) {
    data.parents.push_back(parent);
  }
  data.author = author.empty()
                    ? userName + " <" + userEmail + "> " + getCurrentTimestampWithTimezone()
                    : author;
  data.committer = userName + " <" + userEmail + "> " + getCurrentTimestampWithTimezone();
  data.message = message;
  std::string hash = writeObject(GitObjectType::Commit, data);
  std::cout << "Commit object written: " << hash << "\n";
  gitHead head;
  head.updateHead(hash);
  return true;
}

std::unordered_set<std::string>
GitRepository::logBranchCommitHistory(const std::string &branchName) {
  std::string currHash = getHashOfBranchHead(branchName);
  std::unordered_set<std::string> commitList;

  CommitObject commitObj(gitDir);

  while (!currHash.empty()) {
    commitList.insert(currHash);
    CommitData commit = commitObj.readObject(currHash);

    if (commit.parents.empty()) {
      break;
    }

    currHash = commit.parents[0]; // walk to first parent
  }

  return commitList;
}

bool GitRepository::gotoStateAtPerticularCommit(const std::string &hash) {
  std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
  if (!std::filesystem::exists(path)) {
    std::cerr << "No such commit exists. Ensure it is part of the current branch.\n";
    return false;
  }
  std::unordered_set<std::string> commitListInCurrentBranch(
      logBranchCommitHistory(getCurrentBranch()).begin(),
      logBranchCommitHistory(getCurrentBranch()).end());
  if (commitListInCurrentBranch.find(hash) == commitListInCurrentBranch.end()) {
    std::cerr << "Commit is not part of current branch history.\n";
    return false;
  }
  IndexManager idx(gitDir);
  auto status = idx.computeStatus();
  if (!status.empty()) {
    std::cerr << "Untracked or modified changes exist. Please commit/stash them before reset.\n";
    return false;
  }
  CommitObject commitObj(gitDir);
  CommitData commit = commitObj.readObject(hash);
  for (auto &entry : std::filesystem::directory_iterator(".")) {
    if (entry.path().filename() != ".git") {
      std::filesystem::remove_all(entry);
    }
  }
  TreeObject treeObj(gitDir);
  treeObj.restoreWorkingDirectoryFromTreeHash(commit.tree, ".");
  gitHead head;
  head.updateHead(hash);
  std::cout << "Repository successfully reset to commit: " << hash << "\n";
  return true;
}

bool GitRepository::push(const std::string &remote) {
    std::string remoteGitDir = remote;
    // If not a path, try to resolve as remote name
    if (remote.find('/') == std::string::npos && remote.find('.') == std::string::npos) {
        GitConfig config(GitConfig::findGitDir());
        std::string resolved;
        if (config.getRemote(remote, resolved)) {
            remoteGitDir = resolved;
        } else {
            std::cerr << "Remote '" << remote << "' not found in config.\n";
            return false;
        }
    }
    namespace fs = std::filesystem;
    std::string localObjects = gitDir + "/objects";
    std::string remoteObjects = remoteGitDir + "/objects";
    for (auto &dirEntry : fs::recursive_directory_iterator(localObjects)) {
        if (dirEntry.is_regular_file()) {
            std::string relPath = fs::relative(dirEntry.path(), localObjects).string();
            fs::path remoteObjPath = fs::path(remoteObjects) / relPath;
            if (!fs::exists(remoteObjPath.parent_path())) {
                fs::create_directories(remoteObjPath.parent_path());
            }
            if (!fs::exists(remoteObjPath)) {
                fs::copy_file(dirEntry.path(), remoteObjPath);
            }
        }
    }
    std::string localRefs = gitDir + "/refs/heads";
    std::string remoteRefs = remoteGitDir + "/refs/heads";
    for (auto &dirEntry : fs::recursive_directory_iterator(localRefs)) {
        if (dirEntry.is_regular_file()) {
            std::string relPath = fs::relative(dirEntry.path(), localRefs).string();
            fs::path remoteRefPath = fs::path(remoteRefs) / relPath;
            if (!fs::exists(remoteRefPath.parent_path())) {
                fs::create_directories(remoteRefPath.parent_path());
            }
            fs::copy_file(dirEntry.path(), remoteRefPath, fs::copy_options::overwrite_existing);
        }
    }
    return true;
}

bool GitRepository::pull(const std::string &remote) {
    std::string remoteGitDir = remote;
    // If not a path, try to resolve as remote name
    if (remote.find('/') == std::string::npos && remote.find('.') == std::string::npos) {
        GitConfig config(GitConfig::findGitDir());
        std::string resolved;
        if (config.getRemote(remote, resolved)) {
            remoteGitDir = resolved;
        } else {
            std::cerr << "Remote '" << remote << "' not found in config.\n";
            return false;
        }
    }
    namespace fs = std::filesystem;
    std::string localObjects = gitDir + "/objects";
    std::string remoteObjects = remoteGitDir + "/objects";
    for (auto &dirEntry : fs::recursive_directory_iterator(remoteObjects)) {
        if (dirEntry.is_regular_file()) {
            std::string relPath = fs::relative(dirEntry.path(), remoteObjects).string();
            fs::path localObjPath = fs::path(localObjects) / relPath;
            if (!fs::exists(localObjPath.parent_path())) {
                fs::create_directories(localObjPath.parent_path());
            }
            if (!fs::exists(localObjPath)) {
                fs::copy_file(dirEntry.path(), localObjPath);
            }
        }
    }
    std::string localRefs = gitDir + "/refs/heads";
    std::string remoteRefs = remoteGitDir + "/refs/heads";
    for (auto &dirEntry : fs::recursive_directory_iterator(remoteRefs)) {
        if (dirEntry.is_regular_file()) {
            std::string relPath = fs::relative(dirEntry.path(), remoteRefs).string();
            fs::path localRefPath = fs::path(localRefs) / relPath;
            if (!fs::exists(localRefPath.parent_path())) {
                fs::create_directories(localRefPath.parent_path());
            }
            fs::copy_file(dirEntry.path(), localRefPath, fs::copy_options::overwrite_existing);
        }
    }
    // Automatically checkout latest commit on current branch
    std::string branch = getCurrentBranch();
    std::string latest = getHashOfBranchHead(branch);
    if (!latest.empty()) {
        gotoStateAtPerticularCommit(latest);
    }
    return true;
}
