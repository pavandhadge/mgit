#include "headers/CLISetupAndHandlers.hpp"
#include <iostream>
#include <vector>
#include "headers/GitObjectStorage.hpp"
#include "headers/GitRepository.hpp"
#include "headers/ZlibUtils.hpp"
#include "headers/GitConfig.hpp"
#include "utils/CLI11.hpp"


// ===================== INITIALIZATION =====================
void handleInit(GitRepository& repo, const std::string& path) {
    repo.init(path);
    std::cout << "Initialized empty Git repository in " << path << "\n";
}

// ==================== OBJECT OPERATIONS ====================
void handleHashObject(GitRepository& repo, const std::string& filepath, bool write) {
    std::string hash = repo.writeObject(GitObjectType::Blob, filepath, write);
    std::cout << hash << "\n";
}

void handleWriteTree(GitRepository& repo, const std::string& folder) {
    std::string hash = repo.writeObject(GitObjectType::Tree, folder, true);
    std::cout << "Tree object written: " << hash << "\n";
}

void handleCommitTree(GitRepository& repo, const std::string& tree,
                    const std::string& parent, const std::string& message,
                    const std::string& author) {
    CommitData data;
    data.tree = tree;

    if (!parent.empty()) {
        data.parents.push_back(parent);
    }

    data.author = author.empty()
                ? GitConfig().getName() + " <" + GitConfig().getEmail() + "> " + getCurrentTimestampWithTimezone()
                : author;

    data.committer = GitConfig().getName() + " <" + GitConfig().getEmail() + "> " + getCurrentTimestampWithTimezone();
    data.message = message;

    std::string hash = repo.writeObject(GitObjectType::Commit, data);
    std::cout << "Commit object written: " << hash << "\n";
}

void handleTagObject(GitRepository& repo, const std::string& targetHash,
                   const std::string& targetType, const std::string& tagName,
                   const std::string& tagMessage, const std::string& tagger) {
    TagData data;
    data.objectHash = targetHash;
    data.objectType = targetType;
    data.tagName = tagName;

    data.tagger = tagger.empty()
                ? GitConfig().getName() + " <" + GitConfig().getEmail() + "> " + getCurrentTimestampWithTimezone()
                : tagger + " " + getCurrentTimestampWithTimezone();

    data.message = tagMessage;

    std::string hash = repo.writeObject(GitObjectType::Tag, data);
    std::cout << "Tag object written: " << hash << "\n";
}

// ==================== INSPECTION COMMANDS ====================
void handleReadObject(GitRepository& repo, const std::string& hash) {
    std::string content = repo.readObjectRaw(hash);
    std::cout << "----- Raw Object -----\n" << content << "\n";
}

void handleCatFile(GitRepository& repo, const std::string& hash,
                 bool showContent, bool showType, bool showSize) {
    std::string full = repo.readObjectRaw(hash);
    if (full.empty()) {
        std::cerr << "Object not found\n";
        return;
    }

    size_t nullIdx = full.find('\0');
    if (nullIdx == std::string::npos) {
        std::cerr << "Invalid object format\n";
        return;
    }

    std::string header = full.substr(0, nullIdx);
    std::string content = full.substr(nullIdx + 1);
    std::string type = header.substr(0, header.find(' '));
    size_t size = std::stoul(header.substr(header.find(' ') + 1));

    if (showType) std::cout << type << "\n";
    if (showSize) std::cout << size << "\n";
    if (showContent) std::cout << content << "\n";
}

void handleLsRead(GitRepository& repo, const std::string& hash) {
    GitObjectStorage obj;
    GitObjectType type = obj.identifyType(hash);
    std::string content = repo.readObject(type, hash);
    if (content.empty()) {
        std::cerr << "Could not read object.\n";
    } else {
        std::cout << "----- Object Content -----\n" << content << "\n";
    }
}

void handleLsTree(GitRepository& repo, const std::string& hash) {
    std::string content = repo.readObject(GitObjectType::Tree, hash);
    if (content.empty()) {
        std::cerr << "Invalid tree object\n";
    } else {
        std::cout << content;
    }
}

// =============== INDEX/WORKING DIRECTORY OPERATIONS ===============
void handleAddCommand(GitRepository& repo, const std::vector<std::string>& paths) {
    repo.indexHandler(paths);
}

void handleStatusCommand(GitRepository& repo, bool shortFormat,
                       bool showUntracked, bool showIgnore, bool showBranch) {
    repo.reportStatus(shortFormat, showUntracked);
}

// ==================== BRANCH OPERATIONS ====================
void handleBranchCommand(GitRepository& repo, const std::string& branchName,
                       bool deleteFlag, bool forceDelete, bool listFlag,
                       bool showCurrent, const std::string& newBranchName) {
    if (showCurrent) {
        std::cout << repo.getCurrentBranch() << "\n";
        return;
    }

    if (listFlag || (branchName.empty() && newBranchName.empty() && !deleteFlag && !forceDelete)) {
        repo.listbranches("");
        return;
    }

    if (!newBranchName.empty()) {
        if (branchName.empty()) {
            std::cerr << "Error: Provide current branch name to rename.\n";
            return;
        }
        if (repo.renameBranch(branchName, newBranchName)) {
            std::cout << "Renamed branch from '" << branchName << "' to '" << newBranchName << "'.\n";
        } else {
            std::cerr << "Rename failed.\n";
        }
        return;
    }

    if (deleteFlag || forceDelete) {
        if (branchName.empty()) {
            std::cerr << "Error: Branch name required for deletion.\n";
            return;
        }

        if (!forceDelete && !repo.isFullyMerged(branchName)) {
            std::cerr << "error: The branch '" << branchName << "' is not fully merged.\n";
            std::cerr << "Use -D to force delete.\n";
            return;
        }

        if (repo.deleteBranch(branchName)) {
            std::cout << "Deleted branch: " << branchName << "\n";
        } else {
            std::cerr << "Failed to delete branch: " << branchName << "\n";
        }
        return;
    }

    // Create branch (default)
    repo.CreateBranch(branchName);
    std::cout << "Created branch: " << branchName << "\n";
}

void handleSwitchBranch(GitRepository& repo, const std::string& targetBranch, bool createFlag) {
    repo.changeCurrentBranch(targetBranch, createFlag);
    std::cout << "Switched to branch: " << targetBranch << "\n";
}

void handleCheckoutBranch(GitRepository& repo, const std::string& branchName, bool createFlag) {
    repo.changeCurrentBranch(branchName, createFlag);
    std::cout << "Switched to branch: " << branchName << "\n";
}







// ===================== INITIALIZATION =====================
void setupInitCommand(CLI::App& app, GitRepository& repo) {
    std::string initPath = ".git";
    auto cmd = app.add_subcommand("init", "Initialize Git repository");
    cmd->add_option("path", initPath, "Path to initialize repository")->default_val(".git");
    cmd->callback([&repo, initPath]() {
        handleInit(repo, initPath);
    });
}

// ==================== OBJECT OPERATIONS ====================
void setupHashObjectCommand(CLI::App& app, GitRepository& repo) {
    std::string filePath;
    bool writeFlag = false;
    auto cmd = app.add_subcommand("hash-object", "Compute object ID and optionally creates blob");
    cmd->add_option("file", filePath, "Path to file")->required()->check(CLI::ExistingFile);
    cmd->add_flag("-w", writeFlag, "Write object to database");
    cmd->callback([&repo, filePath, writeFlag]() {
        handleHashObject(repo, filePath, writeFlag);
    });
}

void setupWriteTreeCommand(CLI::App& app, GitRepository& repo) {
    std::string directoryPath;
    auto cmd = app.add_subcommand("write-tree", "Create tree object from current index");
    cmd->add_option("directory", directoryPath, "Directory to write tree from")->default_val(".");
    cmd->callback([&repo, directoryPath]() {
        handleWriteTree(repo, directoryPath);
    });
}

void setupCommitTreeCommand(CLI::App& app, GitRepository& repo) {
    std::string treeHash, parentHash, message, author;
    auto cmd = app.add_subcommand("commit-tree", "Create commit object");
    cmd->add_option("tree", treeHash, "Tree object hash")->required();
    cmd->add_option("-p,--parent", parentHash, "Parent commit hash");
    cmd->add_option("-m,--message", message, "Commit message")->required();
    cmd->add_option("--author", author, "Commit author");
    cmd->callback([&repo, treeHash, parentHash, message, author]() {
        handleCommitTree(repo, treeHash, parentHash, message, author);
    });
}

void setupTagObjectCommand(CLI::App& app, GitRepository& repo) {
    std::string objectHash, objectType, tagName, message, tagger;
    auto cmd = app.add_subcommand("tag", "Create tag object");
    cmd->add_option("object", objectHash, "Object to tag")->required();
    cmd->add_option("type", objectType, "Object type")->required();
    cmd->add_option("name", tagName, "Tag name")->required();
    cmd->add_option("-m,--message", message, "Tag message")->required();
    cmd->add_option("--tagger", tagger, "Tagger information");
    cmd->callback([&repo, objectHash, objectType, tagName, message, tagger]() {
        handleTagObject(repo, objectHash, objectType, tagName, message, tagger);
    });
}

// ==================== INSPECTION COMMANDS ====================
void setupReadObjectCommand(CLI::App& app, GitRepository& repo) {
    std::string objectHash;
    auto cmd = app.add_subcommand("read-object", "Read raw object content");
    cmd->add_option("hash", objectHash, "Object hash")->required();
    cmd->callback([&repo, objectHash]() {
        handleReadObject(repo, objectHash);
    });
}

void setupCatFileCommand(CLI::App& app, GitRepository& repo) {
    std::string objectHash;
    bool showType = false, showSize = false, showContent = false;
    auto cmd = app.add_subcommand("cat-file", "Inspect object content");
    cmd->add_option("hash", objectHash, "Object hash")->required();
    cmd->add_flag("-t", showType, "Show object type");
    cmd->add_flag("-s", showSize, "Show object size");
    cmd->add_flag("-p", showContent, "Pretty-print object content");
    cmd->callback([&repo, objectHash, showType, showSize, showContent]() {
        handleCatFile(repo, objectHash, showContent, showType, showSize);
    });
}

void setupLsReadCommand(CLI::App& app, GitRepository& repo) {
    std::string objectHash;
    auto cmd = app.add_subcommand("ls-read", "Read and parse object content");
    cmd->add_option("hash", objectHash, "Object hash")->required();
    cmd->callback([&repo, objectHash]() {
        handleLsRead(repo, objectHash);
    });
}

void setupLsTreeCommand(CLI::App& app, GitRepository& repo) {
    std::string treeHash;
    auto cmd = app.add_subcommand("ls-tree", "List tree contents");
    cmd->add_option("hash", treeHash, "Tree object hash")->required();
    cmd->callback([&repo, treeHash]() {
        handleLsTree(repo, treeHash);
    });
}

// =============== INDEX/WORKING DIRECTORY OPERATIONS ===============
void setupAddCommand(CLI::App& app, GitRepository& repo) {
    std::vector<std::string> paths;
    auto cmd = app.add_subcommand("add", "Add files to index");
    cmd->add_option("paths", paths, "Files to add")->required()->expected(-1);
    cmd->callback([&repo, paths]() {
        handleAddCommand(repo, paths);
    });
}

void setupStatusCommand(CLI::App& app, GitRepository& repo) {
    bool shortFormat = false;
    bool showUntracked = true;
    bool showIgnored = false;
    bool showBranch = false;
    auto cmd = app.add_subcommand("status", "Show working tree status");
    cmd->add_flag("-s,--short", shortFormat, "Short format");
    cmd->add_flag("-u,--untracked", showUntracked, "Show untracked files");
    cmd->add_flag("-i,--ignored", showIgnored, "Show ignored files");
    cmd->add_flag("-b,--branch", showBranch, "Show branch information");
    cmd->callback([&repo, shortFormat, showUntracked, showIgnored, showBranch]() {
        handleStatusCommand(repo, shortFormat, showUntracked, showIgnored, showBranch);
    });
}

// ==================== BRANCH OPERATIONS ====================
void setupBranchCommand(CLI::App& app, GitRepository& repo) {
    std::string branchName;
    std::string newBranchName;
    bool deleteFlag = false;
    bool forceDelete = false;
    bool listFlag = false;
    bool showCurrent = false;

    auto cmd = app.add_subcommand("branch", "List, create, or delete branches");
    cmd->add_option("name", branchName, "Branch name");
    cmd->add_option("-m,--move", newBranchName, "New branch name for rename");
    cmd->add_flag("-d,--delete", deleteFlag, "Delete branch");
    cmd->add_flag("-D", forceDelete, "Force delete branch");
    cmd->add_flag("-l,--list", listFlag, "List branches");
    cmd->add_flag("--show-current", showCurrent, "Show current branch");

    cmd->callback([&repo, branchName, newBranchName, deleteFlag, forceDelete, listFlag, showCurrent]() {
        handleBranchCommand(repo, branchName, deleteFlag, forceDelete, listFlag, showCurrent, newBranchName);
    });
}

void setupSwitchCommand(CLI::App& app, GitRepository& repo) {
    std::string branchName;
    bool createFlag = false;
    auto cmd = app.add_subcommand("switch", "Switch branches");
    cmd->add_option("branch", branchName, "Branch name")->required();
    cmd->add_flag("-c,--create", createFlag, "Create new branch");
    cmd->callback([&repo, branchName, createFlag]() {
        handleSwitchBranch(repo, branchName, createFlag);
    });
}

void setupCheckoutCommand(CLI::App& app, GitRepository& repo) {
    std::string branchName;
    bool createFlag = false;
    auto cmd = app.add_subcommand("checkout", "Switch branches (legacy command)");
    cmd->add_option("branch", branchName, "Branch name")->required();
    cmd->add_flag("-c,--create", createFlag, "Create new branch");
    cmd->callback([&repo, branchName, createFlag]() {
        handleCheckoutBranch(repo, branchName, createFlag);
    });
}

// ==================== MAIN APP SETUP ====================
void setupAllCommands(CLI::App& app, GitRepository& repo) {
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
}
