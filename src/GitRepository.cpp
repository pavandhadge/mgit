#include "headers/GitRepository.hpp"
#include "headers/GitBranch.hpp"
#include "headers/GitConfig.hpp"
#include "headers/GitHead.hpp"
#include "headers/GitIndex.hpp"
#include "headers/GitInit.hpp"
#include "headers/GitMerge.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitObjectTypesClasses.hpp"
#include "headers/ZlibUtils.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

GitRepository::GitRepository(const std::string &root)
    : gitDir(root), merge(std::make_unique<GitMerge>(gitDir)) {}

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

    std::string headCommitHash = getHashOfBranchHead(getCurrentBranch());
    std::string headTreeHash;
    if (!headCommitHash.empty()) {
      CommitObject commitObj(gitDir);
      CommitData commitData = commitObj.readObject(headCommitHash);
      headTreeHash = commitData.tree;
    }

    StatusResult status = idx.computeStatus(headTreeHash);

    std::ostringstream out;
    out << "On branch " << getCurrentBranch() << "\n";

    if (headCommitHash.empty()) {
      out << "\nNo commits yet\n";
    }

    if (status.staged_changes.empty() && status.unstaged_changes.empty() &&
        status.untracked_files.empty()) {
      out << "\nnothing to commit, working tree clean\n";
      std::cout << out.str();
      return true;
    }

    // Staged changes
    if (!status.staged_changes.empty()) {
      out << "\nChanges to be committed:\n";
      out << "  (use \"mgit restore --staged <file>...\" to unstage)\n";
      for (const auto &change : status.staged_changes) {
        out << "\t" << change.first << ":   " << change.second << "\n";
      }
      out << "\n";
    }

    // Unstaged changes
    if (!status.unstaged_changes.empty()) {
      out << "\nChanges not staged for commit:\n";
      out << "  (use \"mgit add <file>...\" to update what will be "
             "committed)\n";
      out << "  (use \"mgit checkout -- <file>...\" to discard changes in "
             "working directory)\n";
      for (const auto &change : status.unstaged_changes) {
        out << "\t" << change.first << ":   " << change.second << "\n";
      }
      out << "\n";
    }

    // Untracked files
    if (showUntracked && !status.untracked_files.empty()) {
      out << "Untracked files:\n";
      out << "  (use \"mgit add <file>...\" to include in what will be "
             "committed)\n";
      for (const auto &path : status.untracked_files) {
        out << "\t" << path << "\n";
      }
      out << "\n";
    }

    std::cout << out.str();
    return true;
  } catch (const std::exception &e) {
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

bool GitRepository::listbranches(const std::string &branchName) {
  try {
    Branch branchObj;
    if (!branchObj.listBranches()) {
      throw std::runtime_error("Failed to list branches");
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "listbranches failed: " << e.what() << std::endl;
    return false;
  }
}

std::string GitRepository::getCurrentBranch() const {
  Branch branchObj;
  return branchObj.getCurrentBranch();
}

bool GitRepository::changeCurrentBranch(const std::string &targetBranch,
                                        bool createflag) {
  try {
    if (createflag) {
      if (!CreateBranch(targetBranch))
        return false;
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
  std::lock_guard<std::mutex> lock(mergeMutex);

  if (targetBranch.empty()) {
    throw std::invalid_argument("Branch name cannot be empty");
  }

  std::string currentBranch = getCurrentBranch();
  if (currentBranch == targetBranch) {
    throw std::runtime_error("Cannot merge branch into itself");
  }

  std::string currentHead = getHashOfBranchHead(currentBranch);
  std::string targetHead = getHashOfBranchHead(targetBranch);

  if (currentHead.empty() || targetHead.empty()) {
    throw std::runtime_error("One or both branches have no commits");
  }

  std::string baseCommitHash = findCommonAncestor(currentHead, targetHead);

  if (baseCommitHash == targetHead) {
    std::cout << "Already up-to-date." << std::endl;
    return true;
  }

  if (baseCommitHash == currentHead) {
    // Fast-forward merge
    gotoStateAtPerticularCommit(targetHead);
    gitHead head;
    head.updateHead(targetHead);
    std::cout << "Fast-forward merge." << std::endl;
    return true;
  }

  CommitObject commitObj(gitDir);
  std::string baseTree = commitObj.readObject(baseCommitHash).tree;
  std::string ourTree = commitObj.readObject(currentHead).tree;
  std::string theirTree = commitObj.readObject(targetHead).tree;

  TreeObject treeObj(gitDir);
  std::map<std::string, std::string> baseFiles, ourFiles, theirFiles;
  treeObj.getAllFiles(baseTree, baseFiles);
  treeObj.getAllFiles(ourTree, ourFiles);
  treeObj.getAllFiles(theirTree, theirFiles);

  std::vector<std::string> conflictingFiles;
  IndexManager idx(gitDir);
  idx.readIndex();

  std::set<std::string> allPaths;
  for (const auto &[path, hash] : baseFiles)
    allPaths.insert(path);
  for (const auto &[path, hash] : ourFiles)
    allPaths.insert(path);
  for (const auto &[path, hash] : theirFiles)
    allPaths.insert(path);

  for (const std::string &path : allPaths) {
    std::string baseHash = baseFiles.count(path) ? baseFiles[path] : "";
    std::string ourHash = ourFiles.count(path) ? ourFiles[path] : "";
    std::string theirHash = theirFiles.count(path) ? theirFiles[path] : "";

    if (ourHash != theirHash) {
      if (baseHash == ourHash) { // Changed in theirs
        IndexEntry entry = idx.gitIndexEntryFromPath(path);
        entry.hash = theirHash;
        idx.addOrUpdateEntry(entry);
        BlobObject blob(gitDir);
        std::ofstream f(path);
        f << blob.readObject(theirHash).content;
        f.close();
      } else if (baseHash == theirHash) { // Changed in ours
        // Do nothing, it's already in our branch
      } else { // Conflict
        conflictingFiles.push_back(path);
        BlobObject blob(gitDir);
        std::string ourContent = blob.readObject(ourHash).content;
        std::string theirContent = blob.readObject(theirHash).content;

        std::ofstream f(path);
        f << "<<<<<<< HEAD\n";
        f << ourContent;
        f << "=======\n";
        f << theirContent;
        f << ">>>>>>> " << targetBranch << "\n";
        f.close();

        IndexEntry entry;
        entry.path = path;
        entry.mode = "100644";
        entry.hash = ourHash;
        entry.base_hash = baseHash;
        entry.their_hash = theirHash;
        entry.conflict_state = ConflictState::UNRESOLVED;
        idx.addOrUpdateEntry(entry);
      }
    }
  }

  idx.writeIndex();

  if (!conflictingFiles.empty()) {
    std::ofstream mergeHeadFile(gitDir + "/MERGE_HEAD");
    mergeHeadFile << targetHead;
    mergeHeadFile.close();

    std::ofstream mergeBranchFile(gitDir + "/MERGE_BRANCH");
    mergeBranchFile << targetBranch;
    mergeBranchFile.close();

    std::cout << "Auto-merging " << targetBranch << std::endl;
    for (const auto &file : conflictingFiles) {
      std::cout << "CONFLICT (content): Merge conflict in " << file
                << std::endl;
    }
    std::cout << "Automatic merge failed; fix conflicts and then commit the "
                 "result."
              << std::endl;
    return false;
  }

  // Create merge commit
  std::cout << "Merge successful. Please commit the changes." << std::endl;
  return true;
}

bool GitRepository::resolveConflicts() {
  IndexManager idx(gitDir);
  idx.readIndex();

  if (idx.hasConflicts()) {
    std::cerr << "error: You still have unresolved conflicts." << std::endl;
    return false;
  }

  // All conflicts resolved, proceed with commit
  std::string currentHead = getHashOfBranchHead(getCurrentBranch());
  std::string mergeHead;
  std::ifstream mergeHeadFile(gitDir + "/MERGE_HEAD");
  if (mergeHeadFile) {
    mergeHeadFile >> mergeHead;
    mergeHeadFile.close();
  }

  std::string targetBranch;
  std::ifstream mergeBranchFile(gitDir + "/MERGE_BRANCH");
  if (mergeBranchFile) {
    mergeBranchFile >> targetBranch;
    mergeBranchFile.close();
  }

  if (currentHead.empty() || mergeHead.empty() || targetBranch.empty()) {
    std::cerr << "error: Could not find HEAD, MERGE_HEAD, or MERGE_BRANCH. "
                 "Cannot complete merge."
              << std::endl;
    return false;
  }

  std::string message = "Merge branch '" + targetBranch + "'";
  if (createCommit(message, "")) {
    std::filesystem::remove(gitDir + "/MERGE_HEAD");
    std::filesystem::remove(gitDir + "/MERGE_BRANCH");
    std::cout << "Merge completed successfully." << std::endl;
    return true;
  } else {
    std::cerr << "error: Failed to create merge commit." << std::endl;
    return false;
  }
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
  try {
    std::string mergeHeadPath = gitDir + "/MERGE_HEAD";
    if (std::filesystem::exists(mergeHeadPath)) {
      std::filesystem::remove(mergeHeadPath);
    }

    std::string mergeBranchPath = gitDir + "/MERGE_BRANCH";
    if (std::filesystem::exists(mergeBranchPath)) {
      std::filesystem::remove(mergeBranchPath);
    }

    std::string headCommit = getHashOfBranchHead(getCurrentBranch());
    if (headCommit.empty()) {
      IndexManager idx(gitDir);
      idx.resetFromTree(""); // Reset to an empty index
      // Also need to clean the working directory
      for (auto &p : std::filesystem::directory_iterator(".")) {
        if (p.path().filename() != ".git" && p.path().filename() != ".mgit") {
          std::filesystem::remove_all(p.path());
        }
      }
      std::cout << "Merge aborted." << std::endl;
      return true;
    }

    CommitObject commitObj(gitDir);
    CommitData commitData = commitObj.readObject(headCommit);
    std::string headTreeHash = commitData.tree;

    TreeObject treeObj(gitDir);
    treeObj.restoreWorkingDirectoryFromTreeHash(headTreeHash, ".");

    IndexManager idx(gitDir);
    idx.resetFromTree(headTreeHash);

    std::cout
        << "Merge aborted. Your branch is now back to its pre-merge state."
        << std::endl;
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Failed to abort merge: " << e.what() << std::endl;
    return false;
  }
}

std::vector<std::string> GitRepository::getConflictingFiles() {
  IndexManager idx(gitDir);
  return idx.getConflictingFiles();
}

bool GitRepository::isConflicted(const std::string &path) {
  IndexManager idx(gitDir);
  return idx.isConflicted(path);
}

bool GitRepository::resolveConflict(const std::string &path,
                                    const std::string &hash) {
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
    std::cerr
        << "fatal: unable to auto-detect email address (user.email not set)\n";
    return false;
  }

  IndexManager idx(gitDir);
  idx.readIndex();

  if (idx.hasConflicts()) {
    std::cerr << "Cannot commit with unresolved conflicts." << std::endl;
    return false;
  }

  TreeObject tree(gitDir);
  std::string treeHash = tree.writeTreeFromIndex(idx.getEntries());

  if (treeHash.empty()) {
    std::cerr << "Failed to create tree from index.\n";
    return false;
  }

  CommitData data;
  data.tree = treeHash;
  std::string parent = getHashOfBranchHead(getCurrentBranch());
  if (!parent.empty()) {
    data.parents.push_back(parent);
  }

  // Handle merge commit
  std::string mergeHeadPath = gitDir + "/MERGE_HEAD";
  if (std::filesystem::exists(mergeHeadPath)) {
    std::ifstream mergeHeadFile(mergeHeadPath);
    std::string mergeParent;
    mergeHeadFile >> mergeParent;
    data.parents.push_back(mergeParent);
    std::filesystem::remove(mergeHeadPath);
  }

  data.author = author.empty() ? userName + " <" + userEmail + "> " +
                                     getCurrentTimestampWithTimezone()
                               : author;
  data.committer =
      userName + " <" + userEmail + "> " + getCurrentTimestampWithTimezone();
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

std::string GitRepository::findCommonAncestor(const std::string &commitA,
                                              const std::string &commitB) {
  std::unordered_set<std::string> historyA = logBranchCommitHistory(commitA);
  std::string currentB = commitB;
  CommitObject commitObj(gitDir);

  while (!currentB.empty()) {
    if (historyA.count(currentB)) {
      return currentB;
    }
    CommitData commit = commitObj.readObject(currentB);
    if (commit.parents.empty()) {
      break;
    }
    currentB = commit.parents[0];
  }
  return ""; // No common ancestor found
}

bool GitRepository::gotoStateAtPerticularCommit(const std::string &hash) {
  std::string path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
  if (!std::filesystem::exists(path)) {
    std::cerr
        << "No such commit exists. Ensure it is part of the current branch.\n";
    return false;
  }
  std::unordered_set<std::string> commitListInCurrentBranch(
      logBranchCommitHistory(getCurrentBranch()).begin(),
      logBranchCommitHistory(getCurrentBranch()).end());
  if (commitListInCurrentBranch.find(hash) == commitListInCurrentBranch.end()) {
    std::cerr << "Commit is not part of current branch history.\n";
    return false;
  }

  CommitObject commitObj(gitDir);
  CommitData commit = commitObj.readObject(hash);
  std::vector<std::filesystem::path> to_delete;
  for (auto &entry : std::filesystem::directory_iterator(".")) {
    if (entry.path().filename() != ".git") {
      to_delete.push_back(entry.path());
    }
  }
  for (const auto &path : to_delete) {
    std::filesystem::remove_all(path);
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
  if (remote.find('/') == std::string::npos &&
      remote.find('.') == std::string::npos) {
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
      std::string relPath =
          fs::relative(dirEntry.path(), localObjects).string();
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
      fs::copy_file(dirEntry.path(), remoteRefPath,
                    fs::copy_options::overwrite_existing);
    }
  }
  return true;
}

bool GitRepository::pull(const std::string &remote) {
  std::string remoteGitDir = remote;
  // If not a path, try to resolve as remote name
  if (remote.find('/') == std::string::npos &&
      remote.find('.') == std::string::npos) {
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
      std::string relPath =
          fs::relative(dirEntry.path(), remoteObjects).string();
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
      fs::copy_file(dirEntry.path(), localRefPath,
                    fs::copy_options::overwrite_existing);
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
