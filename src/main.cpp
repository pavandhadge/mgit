#include <iostream>
#include "headers/GitObjectStorage.hpp"
#include "headers/GitRepository.hpp"
#include "utils/CLI11.hpp"

void handleInit(GitRepository& repo, const std::string& path) {
    repo.init(path);
    std::cout << "Initialized empty Git repository in " << path << "\n";
}

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
    std::string hash = repo.writeObject(GitObjectType::Commit, tree, parent, message, author);
    std::cout << "Commit object written: " << hash << "\n";
}

void handleTagObject(GitRepository& repo, const std::string& targetHash,
                     const std::string& targetType, const std::string& tagName,
                     const std::string& tagMessage, const std::string& tagger) {
    std::string hash = repo.writeObject(GitObjectType::Tag, targetHash, targetType, tagName, tagMessage, tagger);
    std::cout << "Tag object written: " << hash << "\n";
}

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

void handleAddCommand(GitRepository& repo, const std::vector<std::string>& paths){
    repo.indexHandler(paths);
}

void handleStatusCommand(GitRepository& repo, bool shortFormat,
bool showUntracked,
bool showIgnore,
bool showBranch){
    repo.reportStatus(shortFormat , showUntracked);
}

int main(int argc, char** argv) {
    CLI::App app{"yourgit - a minimal Git implementation"};
    GitRepository repo;

    // init
    std::string initPath = ".git";
    auto initCmd = app.add_subcommand("init", "Initialize Git repository");
    initCmd->add_option("path", initPath, "Path")->default_val(".git");

    // hash-object
    std::string filePath;
    bool writeBlob = false;
    auto hashCmd = app.add_subcommand("hash-object", "Hash/write blob object");
    hashCmd->add_option("file", filePath, "File path")->required();
    hashCmd->add_flag("-w", writeBlob, "Write object to DB");

    // write-tree
    std::string folderPath;
    auto treeCmd = app.add_subcommand("write-tree", "Write tree object from folder");
    treeCmd->add_option("folder", folderPath, "Folder path")->required();

    // commit-tree
    std::string treeHash, parentHash, commitMsg, commitAuthor;
    auto commitCmd = app.add_subcommand("commit-tree", "Create commit object");
    commitCmd->add_option("tree", treeHash)->required();
    commitCmd->add_option("parent", parentHash)->default_val("");
    commitCmd->add_option("message", commitMsg)->required();
    commitCmd->add_option("author", commitAuthor)->default_val("Anon <anon@example.com>");

    // tag-object
    std::string targetHash, targetType, tagName, tagMsg, tagger;
    auto tagCmd = app.add_subcommand("tag-object", "Create tag object");
    tagCmd->add_option("object", targetHash)->required();
    tagCmd->add_option("type", targetType)->required();
    tagCmd->add_option("tag", tagName)->required();
    tagCmd->add_option("message", tagMsg)->required();
    tagCmd->add_option("tagger", tagger)->required();

    // read-object
    std::string readHash;
    auto readCmd = app.add_subcommand("read-object", "Raw read from object DB");
    readCmd->add_option("hash", readHash)->required();

    // cat-file
    std::string catHash;
    bool catP = false, catT = false, catS = false;
    auto catCmd = app.add_subcommand("cat-file", "Git style object inspection");
    catCmd->add_option("hash", catHash)->required();
    catCmd->add_flag("-p", catP, "Print content");
    catCmd->add_flag("-t", catT, "Show type");
    catCmd->add_flag("-s", catS, "Show size");

    // ls-read
    std::string lsHash;
    auto lsCmd = app.add_subcommand("ls-read", "Auto-detect read with parsed content");
    lsCmd->add_option("hash", lsHash)->required();

    // ls-tree
    std::string lsTreeHash;
    auto lsTreeCmd = app.add_subcommand("ls-tree", "List tree object contents");
    lsTreeCmd->add_option("hash", lsTreeHash)->required();

    std::vector<std::string> addCommandPaths;
    auto add_cmd = app.add_subcommand("add", "Add files to the staging area");
    add_cmd->add_option("paths", addCommandPaths, "Paths to add to the index")
           ->required()
           ->check(CLI::ExistingPath) // Optional: check if file/folder exists
           ->expected(-1); // Allow unlimited number of arguments


    bool shortFormat = false;
    bool showUntracked = false;
    bool showIgnore = false;
    bool showBranch = false;
    auto statusCmd = app.add_subcommand("status", "Git style status");
    statusCmd->add_flag("-s", shortFormat, "Show short format output");
    statusCmd->add_flag("--short", shortFormat, "Show short format output");
    statusCmd->add_flag("--untracked-files", showUntracked, "Show untracked files");
    statusCmd->add_flag("--ignored", showIgnore, "Show ignored files");
    statusCmd->add_flag("--branch", showBranch, "Show branch info");


    CLI11_PARSE(app, argc, argv);

    if (*initCmd) {
        handleInit(repo, initPath);
    } else if (*hashCmd) {
        handleHashObject(repo, filePath, writeBlob);
    } else if (*treeCmd) {
        handleWriteTree(repo, folderPath);
    } else if (*commitCmd) {
        handleCommitTree(repo, treeHash, parentHash, commitMsg, commitAuthor);
    } else if (*tagCmd) {
        handleTagObject(repo, targetHash, targetType, tagName, tagMsg, tagger);
    } else if (*readCmd) {
        handleReadObject(repo, readHash);
    } else if (*catCmd) {
        if (!catP && !catT && !catS) {
            std::cerr << "Use at least one of -p, -t, or -s\n";
        } else {
            handleCatFile(repo, catHash, catP, catT, catS);
        }
    } else if (*lsCmd) {
        handleLsRead(repo, lsHash);
    } else if (*lsTreeCmd) {
        handleLsTree(repo, lsTreeHash);
    }else if(*add_cmd){
        handleAddCommand(repo, addCommandPaths);
    }else if(*statusCmd){
        handleStatusCommand(repo ,shortFormat,
    showUntracked,
    showIgnore,
    showBranch);
    } else {
        std::cout << app.help() << "\n";
    }

    return 0;
}
