#include "headers/CLISetupAndHandlers.hpp"
#include "headers/GitConfig.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/GitRepository.hpp"
#include "headers/ZlibUtils.hpp"
#include "headers/GitActivityLogger.hpp"
#include "utils/CLI11.hpp"
#include <iostream>
#include <vector>
#include <iomanip>

// ===================== INITIALIZATION =====================
bool handleInit(GitRepository &repo, const std::string &path) {
  repo.init(path);
  std::cout << "Initialized empty Git repository in " << path << "\n";
  return true;
}

// ==================== OBJECT OPERATIONS ====================
bool handleHashObject(GitRepository &repo, const std::string &filepath,
                      bool write) {
  std::string hash = repo.writeObject(GitObjectType::Blob, filepath, write);
  std::cout << hash << "\n";
  return true;
}

bool handleWriteTree(GitRepository &repo, const std::string &folder) {
  std::string hash = repo.writeObject(GitObjectType::Tree, folder, true);
  std::cout << "Tree object written: " << hash << "\n";
  return true;
}

bool handleCommitTree(GitRepository &repo, const std::string &tree,
                      const std::string &parent, const std::string &message,
                      const std::string &author) {
  CommitData data;
  data.tree = tree;

  if (!parent.empty()) {
    data.parents.push_back(parent);
  }

  data.author = author.empty()
                    ? GitConfig().getUserName() + " <" + GitConfig().getUserEmail() +
                          "> " + getCurrentTimestampWithTimezone()
                    : author;

  data.committer = GitConfig().getUserName() + " <" + GitConfig().getUserEmail() +
                   "> " + getCurrentTimestampWithTimezone();
  data.message = message;

  std::string hash = repo.writeObject(GitObjectType::Commit, data);
  std::cout << "Commit object written: " << hash << "\n";
  return true;
}

bool handleTagObject(GitRepository &repo, const std::string &targetHash,
                     const std::string &targetType, const std::string &tagName,
                     const std::string &tagMessage, const std::string &tagger) {
  TagData data;
  data.objectHash = targetHash;
  data.objectType = targetType;
  data.tagName = tagName;

  data.tagger = tagger.empty()
                    ? GitConfig().getUserName() + " <" + GitConfig().getUserEmail() +
                          "> " + getCurrentTimestampWithTimezone()
                    : tagger + " " + getCurrentTimestampWithTimezone();

  data.message = tagMessage;

  std::string hash = repo.writeObject(GitObjectType::Tag, data);
  std::cout << "Tag object written: " << hash << "\n";
  return true;
}

// ==================== INSPECTION COMMANDS ====================
bool handleReadObject(GitRepository &repo, const std::string &hash) {
  std::string content = repo.readObjectRaw(hash);
  std::cout << "----- Raw Object -----\n" << content << "\n";
  return true;
}

bool handleCatFile(GitRepository &repo, const std::string &hash,
                   bool showContent, bool showType, bool showSize) {
  std::string full = repo.readObjectRaw(hash);
  if (full.empty()) {
    std::cerr << "Object not found\n";
    return false;
  }

  size_t nullIdx = full.find('\0');
  if (nullIdx == std::string::npos) {
    std::cerr << "Invalid object format\n";
    return false;
  }

  std::string header = full.substr(0, nullIdx);
  std::string content = full.substr(nullIdx + 1);
  std::string type = header.substr(0, header.find(' '));
  size_t size = std::stoul(header.substr(header.find(' ') + 1));

  if (showType)
    std::cout << type << "\n";
  if (showSize)
    std::cout << size << "\n";
  if (showContent)
    std::cout << content << "\n";
  return true;
}

bool handleLsRead(GitRepository &repo, const std::string &hash) {
  std::string content = repo.readObjectRaw(hash);
  if (content.empty()) {
    std::cerr << "Could not read object.\n";
    return false;
  } else {
    std::cout << "----- Object Content -----\n" << content << "\n";
    return true;
  }
}

bool handleLsTree(GitRepository &repo, const std::string &hash) {
  std::string content = repo.readObject(GitObjectType::Tree, hash);
  if (content.empty()) {
    std::cerr << "Invalid tree object\n";
    return false;
  } else {
    std::cout << content;
    return true;
  }
}

// =============== INDEX/WORKING DIRECTORY OPERATIONS ===============
bool handleAddCommand(GitRepository &repo,
                      const std::vector<std::string> &paths) {
  repo.indexHandler(paths);
  return true;
}

bool handleStatusCommand(GitRepository &repo, bool shortFormat,
                         bool showUntracked, bool showIgnore, bool showBranch) {
  repo.reportStatus(shortFormat, showUntracked);
  return true;
}

// ==================== BRANCH OPERATIONS ====================
bool handleBranchCommand(GitRepository &repo, const std::string &branchName,
                         bool deleteFlag, bool forceDelete, bool listFlag,
                         bool showCurrent, const std::string &newBranchName) {
  if (showCurrent) {
    std::cout << repo.getCurrentBranch() << "\n";
    return true;
  }

  if (listFlag || (branchName.empty() && newBranchName.empty() && !deleteFlag &&
                   !forceDelete)) {
    repo.listbranches("");
    return true;
  }

  if (!newBranchName.empty()) {
    if (branchName.empty()) {
      std::cerr << "Error: Provide current branch name to rename.\n";
      return false;
    }
    if (repo.renameBranch(branchName, newBranchName)) {
      std::cout << "Renamed branch from '" << branchName << "' to '"
                << newBranchName << "'.\n";
      return true;
    } else {
      std::cerr << "Rename failed.\n";
      return false;
    }
  }

  if (deleteFlag || forceDelete) {
    if (branchName.empty()) {
      std::cerr << "Error: Branch name required for deletion.\n";
      return false;
    }

    if (!forceDelete && !repo.isFullyMerged(branchName)) {
      std::cerr << "error: The branch '" << branchName
                << "' is not fully merged.\n";
      std::cerr << "Use -D to force delete.\n";
      return false;
    }

    if (repo.deleteBranch(branchName)) {
      std::cout << "Deleted branch: " << branchName << "\n";
      return true;
    } else {
      std::cerr << "Failed to delete branch: " << branchName << "\n";
      return false;
    }
  }

  // Create branch (default)
  if (repo.CreateBranch(branchName)) {
    std::cout << "Created branch: " << branchName << "\n";
    return true;
  } else {
    std::cerr << "Failed to create branch: " << branchName << "\n";
    return false;
  }
}

bool handleSwitchBranch(GitRepository &repo, const std::string &targetBranch,
                        bool createFlag) {
  if (createFlag) {
    if (repo.CreateBranch(targetBranch)) {
      std::cout << "Created and switched to branch: " << targetBranch << "\n";
    } else {
      std::cerr << "Failed to create branch: " << targetBranch << "\n";
      return false;
    }
  }

  if (repo.changeCurrentBranch(targetBranch, false)) {
    std::cout << "Switched to branch: " << targetBranch << "\n";
    return true;
  } else {
    std::cerr << "Failed to switch to branch: " << targetBranch << "\n";
    return false;
  }
}

bool handleCheckoutBranch(GitRepository &repo, const std::string &branchName,
                          bool createFlag) {
  if (createFlag) {
    if (repo.CreateBranch(branchName)) {
      std::cout << "Created and checked out branch: " << branchName << "\n";
    } else {
      std::cerr << "Failed to create branch: " << branchName << "\n";
      return false;
    }
  }

  if (repo.changeCurrentBranch(branchName, false)) {
    std::cout << "Checked out branch: " << branchName << "\n";
    return true;
  } else {
    std::cerr << "Failed to checkout branch: " << branchName << "\n";
    return false;
  }
}

// ==================== MERGE OPERATIONS ====================
bool handleMergeCommand(GitRepository &repo, const std::string &targetBranch) {
  if (repo.mergeBranch(targetBranch)) {
    std::cout << "Successfully merged branch: " << targetBranch << "\n";
    return true;
  } else {
    std::cerr << "Merge failed or conflicts detected.\n";
    return false;
  }
}

bool handleMergeContinue(GitRepository &repo) {
  if (repo.resolveConflicts(repo.getCurrentBranch())) {
    std::cout << "Merge completed successfully.\n";
    return true;
  } else {
    std::cerr << "Merge continue failed.\n";
    return false;
  }
}

bool handleMergeAbort(GitRepository &repo) {
  if (repo.abortMerge()) {
    std::cout << "Merge aborted.\n";
    return true;
  } else {
    std::cerr << "Merge abort failed.\n";
    return false;
  }
}

bool handleMergeStatus(GitRepository &repo) {
  repo.reportMergeConflicts(repo.getCurrentBranch());
  return true;
}

bool handleResolveConflict(GitRepository& repo, const std::string& path, const std::string& hash) {
  if (repo.resolveConflict(path, hash)) {
    std::cout << "Conflict resolved for: " << path << "\n";
    return true;
  } else {
    std::cerr << "Failed to resolve conflict for: " << path << "\n";
    return false;
  }
}

bool handleActivityLog(GitRepository& repo, const std::string& command, int limit) {
    GitActivityLogger logger;
    
    if (command == "summary") {
        std::cout << logger.generateDetailedSummary() << std::endl;
        return true;
    } else if (command == "stats") {
        std::cout << logger.getDatabaseStats() << std::endl;
        return true;
    } else if (command == "recent") {
        auto activities = logger.getRecentActivity(limit);
        std::cout << "Recent Activity (last " << limit << " commands):\n";
        std::cout << "==========================================\n";
        for (const auto& activity : activities) {
            std::cout << "[" << activity.timestamp << "] " << activity.command;
            if (!activity.arguments.empty()) {
                std::cout << " " << activity.arguments;
            }
            std::cout << " (exit: " << activity.exit_code << ")";
            if (activity.execution_time_ms > 0) {
                std::cout << " [" << std::fixed << std::setprecision(2) << activity.execution_time_ms << "ms]";
            }
            std::cout << "\n";
            if (!activity.error_message.empty()) {
                std::cout << "  Error: " << activity.error_message << "\n";
            }
        }
        return true;
    } else if (command == "usage") {
        auto stats = logger.getCommandUsageStats();
        std::cout << "Command Usage Statistics:\n";
        std::cout << "========================\n";
        for (const auto& stat : stats) {
            std::cout << std::setw(15) << stat.first << ": " << stat.second << " times\n";
        }
        return true;
    } else if (command == "performance") {
        std::cout << logger.generatePerformanceReport() << std::endl;
        return true;
    } else if (command == "errors") {
        std::cout << logger.generateErrorReport() << std::endl;
        return true;
    } else if (command == "analysis") {
        std::cout << logger.generateUsagePatterns() << std::endl;
        return true;
    } else if (command == "timeline") {
        std::cout << logger.generateTimelineReport(7) << std::endl;
        return true;
    } else if (command == "health") {
        std::cout << logger.generateRepositoryHealthReport() << std::endl;
        return true;
    } else if (command == "workflow") {
        std::cout << logger.generateWorkflowAnalysis() << std::endl;
        return true;
    } else if (command == "slow") {
        std::cout << logger.generateSlowCommandsReport(1000.0) << std::endl;
        return true;
    } else if (command == "export") {
        std::string csv_path = ".mgit/activity_export.csv";
        if (logger.exportToCSV(csv_path)) {
            std::cout << "Activity log exported to: " << csv_path << std::endl;
        } else {
            std::cerr << "Failed to export activity log" << std::endl;
        }
        return true;
    } else if (command == "raw") {
        std::cout << "=== RAW ACTIVITY LOG ===\n";
        std::cout << logger.getLogFileContents("activity") << std::endl;
        return true;
    } else if (command == "errors-raw") {
        std::cout << "=== RAW ERROR LOG ===\n";
        std::cout << logger.getLogFileContents("errors") << std::endl;
        return true;
    } else if (command == "performance-raw") {
        std::cout << "=== RAW PERFORMANCE LOG ===\n";
        std::cout << logger.getLogFileContents("performance") << std::endl;
        return true;
    }
    
    std::cerr << "Unknown activity log command: " << command << std::endl;
    std::cerr << "Available commands: summary, stats, recent, usage, performance, errors, analysis, timeline, health, workflow, slow, export, raw, errors-raw, performance-raw" << std::endl;
    return false;
}

// ==================== CLI SETUP FUNCTIONS ====================
bool setupCLIAppHelp(CLI::App &app) {
  app.set_help_flag("-h,--help", "Print this help message and exit");
  return true;
}

bool setupInitCommand(CLI::App &app, GitRepository &repo) {
  auto cmd = app.add_subcommand("init", "Initialize Git repository");
  auto initPath = std::make_shared<std::string>(".git");
  cmd->add_option("path", *initPath, "Path to initialize repository")->default_val(".git");
  cmd->callback([&repo, initPath]() { 
    return handleInit(repo, *initPath); 
  });
  return true;
}

bool setupHashObjectCommand(CLI::App &app, GitRepository &repo) {
  auto filePath = std::make_shared<std::string>();
  auto writeFlag = std::make_shared<bool>(false);
  auto cmd = app.add_subcommand(
      "hash-object", "Compute object ID and optionally creates blob");
  cmd->add_option("file", *filePath, "Path to file")->required()->check(CLI::ExistingFile);
  cmd->add_flag("-w", *writeFlag, "Write object to database");
  cmd->callback([&repo, filePath, writeFlag]() {
    return handleHashObject(repo, *filePath, *writeFlag);
  });
  return true;
}

bool setupWriteTreeCommand(CLI::App &app, GitRepository &repo) {
  auto directoryPath = std::make_shared<std::string>();
  auto cmd = app.add_subcommand("write-tree", "Create tree object from current index");
  cmd->add_option("directory", *directoryPath, "Directory to write tree from")->default_val(".");
  cmd->callback([&repo, directoryPath]() { 
    return handleWriteTree(repo, *directoryPath); 
  });
  return true;
}

bool setupCommitTreeCommand(CLI::App &app, GitRepository &repo) {
  auto treeHash = std::make_shared<std::string>();
  auto parentHash = std::make_shared<std::string>();
  auto message = std::make_shared<std::string>();
  auto author = std::make_shared<std::string>();
  auto cmd = app.add_subcommand("commit-tree", "Create commit object");
  cmd->add_option("tree", *treeHash, "Tree object hash")->required();
  cmd->add_option("-p,--parent", *parentHash, "Parent commit hash");
  cmd->add_option("-m,--message", *message, "Commit message")->required();
  cmd->add_option("--author", *author, "Commit author");
  cmd->callback([&repo, treeHash, parentHash, message, author]() {
    return handleCommitTree(repo, *treeHash, *parentHash, *message, *author);
  });
  return true;
}

bool setupTagObjectCommand(CLI::App &app, GitRepository &repo) {
  auto objectHash = std::make_shared<std::string>();
  auto objectType = std::make_shared<std::string>();
  auto tagName = std::make_shared<std::string>();
  auto message = std::make_shared<std::string>();
  auto tagger = std::make_shared<std::string>();
  auto cmd = app.add_subcommand("tag", "Create tag object");
  cmd->add_option("object", *objectHash, "Object to tag")->required();
  cmd->add_option("type", *objectType, "Object type")->required();
  cmd->add_option("name", *tagName, "Tag name")->required();
  cmd->add_option("-m,--message", *message, "Tag message")->required();
  cmd->add_option("--tagger", *tagger, "Tagger information");
  cmd->callback([&repo, objectHash, objectType, tagName, message, tagger]() {
    return handleTagObject(repo, *objectHash, *objectType, *tagName, *message, *tagger);
  });
  return true;
}

bool setupReadObjectCommand(CLI::App &app, GitRepository &repo) {
  auto objectHash = std::make_shared<std::string>();
  auto cmd = app.add_subcommand("read-object", "Read raw object content");
  cmd->add_option("hash", *objectHash, "Object hash")->required();
  cmd->callback([&repo, objectHash]() { 
    return handleReadObject(repo, *objectHash); 
  });
  return true;
}

bool setupCatFileCommand(CLI::App &app, GitRepository &repo) {
  auto objectHash = std::make_shared<std::string>();
  auto showType = std::make_shared<bool>(false);
  auto showSize = std::make_shared<bool>(false);
  auto showContent = std::make_shared<bool>(false);
  auto cmd = app.add_subcommand("cat-file", "Inspect object content");
  cmd->add_option("hash", *objectHash, "Object hash")->required();
  cmd->add_flag("-t", *showType, "Show object type");
  cmd->add_flag("-s", *showSize, "Show object size");
  cmd->add_flag("-p", *showContent, "Pretty-print object content");
  cmd->callback([&repo, objectHash, showType, showSize, showContent]() {
    return handleCatFile(repo, *objectHash, *showContent, *showType, *showSize);
  });
  return true;
}

bool setupLsReadCommand(CLI::App &app, GitRepository &repo) {
  auto objectHash = std::make_shared<std::string>();
  auto cmd = app.add_subcommand("ls-read", "Read and parse object content");
  cmd->add_option("hash", *objectHash, "Object hash")->required();
  cmd->callback([&repo, objectHash]() { 
    return handleLsRead(repo, *objectHash); 
  });
  return true;
}

bool setupLsTreeCommand(CLI::App &app, GitRepository &repo) {
  auto treeHash = std::make_shared<std::string>();
  auto cmd = app.add_subcommand("ls-tree", "List tree contents");
  cmd->add_option("hash", *treeHash, "Tree object hash")->required();
  cmd->callback([&repo, treeHash]() { 
    return handleLsTree(repo, *treeHash); 
  });
  return true;
}

bool setupAddCommand(CLI::App &app, GitRepository &repo) {
  auto paths = std::make_shared<std::vector<std::string>>();
  auto cmd = app.add_subcommand("add", "Add files to index");
  cmd->add_option("paths", *paths, "Files to add")->required()->expected(-1);
  cmd->callback([&repo, paths]() { 
    return handleAddCommand(repo, *paths); 
  });
  return true;
}

bool setupStatusCommand(CLI::App &app, GitRepository &repo) {
  auto shortFormat = std::make_shared<bool>(false);
  auto showUntracked = std::make_shared<bool>(true);
  auto showIgnored = std::make_shared<bool>(false);
  auto showBranch = std::make_shared<bool>(false);
  auto cmd = app.add_subcommand("status", "Show working tree status");
  cmd->add_flag("-s,--short", *shortFormat, "Short format");
  cmd->add_flag("-u,--untracked", *showUntracked, "Show untracked files");
  cmd->add_flag("-i,--ignored", *showIgnored, "Show ignored files");
  cmd->add_flag("-b,--branch", *showBranch, "Show branch information");
  cmd->callback([&repo, shortFormat, showUntracked, showIgnored, showBranch]() {
    return handleStatusCommand(repo, *shortFormat, *showUntracked, *showIgnored, *showBranch);
  });
  return true;
}

bool setupBranchCommand(CLI::App &app, GitRepository &repo) {
  auto branchName = std::make_shared<std::string>();
  auto newBranchName = std::make_shared<std::string>();
  auto deleteFlag = std::make_shared<bool>(false);
  auto forceDelete = std::make_shared<bool>(false);
  auto listFlag = std::make_shared<bool>(false);
  auto showCurrent = std::make_shared<bool>(false);

  auto cmd = app.add_subcommand("branch", "List, create, or delete branches");
  cmd->add_option("name", *branchName, "Branch name");
  cmd->add_option("-m,--move", *newBranchName, "New branch name for rename");
  cmd->add_flag("-d,--delete", *deleteFlag, "Delete branch");
  cmd->add_flag("-D", *forceDelete, "Force delete branch");
  cmd->add_flag("-l,--list", *listFlag, "List branches");
  cmd->add_flag("--show-current", *showCurrent, "Show current branch");

  cmd->callback([&repo, branchName, newBranchName, deleteFlag, forceDelete,
                 listFlag, showCurrent]() {
    return handleBranchCommand(repo, *branchName, *deleteFlag, *forceDelete, *listFlag,
                        *showCurrent, *newBranchName);
  });
  return true;
}

bool setupSwitchCommand(CLI::App &app, GitRepository &repo) {
  auto branchName = std::make_shared<std::string>();
  auto createFlag = std::make_shared<bool>(false);
  auto cmd = app.add_subcommand("switch", "Switch branches");
  cmd->add_option("branch", *branchName, "Branch name")->required();
  cmd->add_flag("-c,--create", *createFlag, "Create new branch");
  cmd->callback([&repo, branchName, createFlag]() {
    return handleSwitchBranch(repo, *branchName, *createFlag);
  });
  return true;
}

bool setupCheckoutCommand(CLI::App &app, GitRepository &repo) {
  auto branchName = std::make_shared<std::string>();
  auto createFlag = std::make_shared<bool>(false);
  auto cmd = app.add_subcommand(
      "checkout", "Switch branches or restore working tree files");
  cmd->add_option("branch", *branchName, "Branch name to switch to")->required();
  cmd->add_flag("-b", *createFlag, "Create and checkout new branch");
  cmd->callback([&repo, branchName, createFlag]() {
    return handleCheckoutBranch(repo, *branchName, *createFlag);
  });
  return true;
}

bool setupMergeCommand(CLI::App &app, GitRepository &repo) {
  auto targetBranch = std::make_shared<std::string>();
  auto cmd = app.add_subcommand(
      "merge", "Join two or more development histories together");
  cmd->add_option("branch", *targetBranch, "Branch name to merge")->required();
  cmd->callback([&repo, targetBranch]() { 
    return handleMergeCommand(repo, *targetBranch); 
  });
  return true;
}

bool setupMergeContinueCommand(CLI::App &app, GitRepository &repo) {
  auto cmd = app.add_subcommand(
      "merge-continue", "Complete a merge after conflicts are resolved");
  cmd->callback([&repo]() { 
    return handleMergeContinue(repo); 
  });
  return true;
}

bool setupMergeAbortCommand(CLI::App &app, GitRepository &repo) {
  auto cmd = app.add_subcommand("merge-abort", "Abort a merge in progress");
  cmd->callback([&repo]() { 
    return handleMergeAbort(repo); 
  });
  return true;
}

bool setupMergeStatusCommand(CLI::App &app, GitRepository &repo) {
  auto cmd = app.add_subcommand("merge-status", "Show merge conflicts status");
  cmd->callback([&repo]() { 
    return handleMergeStatus(repo); 
  });
  return true;
}

bool setupResolveConflictCommand(CLI::App &app, GitRepository &repo) {
  auto path = std::make_shared<std::string>();
  auto hash = std::make_shared<std::string>();
  auto cmd = app.add_subcommand("resolve-conflict", "Resolve a merge conflict");
  cmd->add_option("path", *path, "Path of the conflicting file")
      ->required()
      ->type_name("PATH");
  cmd->add_option("hash", *hash, "Hash of the version to keep")
      ->required()
      ->type_name("HASH");
  cmd->callback([&repo, path, hash]() { 
    return handleResolveConflict(repo, *path, *hash); 
  });
  return true;
}

bool setupActivityLogCommand(CLI::App &app, GitRepository &repo) {
  auto command = std::make_shared<std::string>();
  auto limit = std::make_shared<int>(10);
  auto cmd = app.add_subcommand("activity", "View activity logs and statistics");
  cmd->add_option("command", *command, "Activity command (summary, stats, recent, usage)")->required();
  cmd->add_option("-l,--limit", *limit, "Limit for recent commands")->default_val(10);
  cmd->callback([&repo, command, limit]() {
    return handleActivityLog(repo, *command, *limit);
  });
  return true;
}

// ==================== MAIN APP SETUP ====================
bool setupAllCommands(CLI::App &app, GitRepository &repo) {
  setupCLIAppHelp(app);
  setupInitCommand(app, repo);
  setupHashObjectCommand(app, repo);
  setupWriteTreeCommand(app, repo);
  setupCommitTreeCommand(app, repo);
  setupTagObjectCommand(app, repo);
  setupReadObjectCommand(app, repo);
  setupCatFileCommand(app, repo);
  setupLsReadCommand(app, repo);
  setupLsTreeCommand(app, repo);
  setupAddCommand(app, repo);
  setupStatusCommand(app, repo);
  setupBranchCommand(app, repo);
  setupSwitchCommand(app, repo);
  setupCheckoutCommand(app, repo);
  setupMergeCommand(app, repo);
  setupMergeContinueCommand(app, repo);
  setupMergeAbortCommand(app, repo);
  setupMergeStatusCommand(app, repo);
  setupResolveConflictCommand(app, repo);
  setupActivityLogCommand(app, repo);
  return true;
}
