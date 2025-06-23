#pragma once
#include "GitRepository.hpp"
#include "../utils/CLI11.hpp"




void handleInit(GitRepository& repo, const std::string& path);

void handleHashObject(GitRepository& repo, const std::string& filepath, bool write);
void handleWriteTree(GitRepository& repo, const std::string& folder);
void handleCommitTree(GitRepository& repo, const std::string& tree,
                    const std::string& parent, const std::string& message,
                    const std::string& author);
void handleTagObject(GitRepository& repo, const std::string& targetHash,
                   const std::string& targetType, const std::string& tagName,
                   const std::string& tagMessage, const std::string& tagger);
void handleReadObject(GitRepository& repo, const std::string& hash);

void handleCatFile(GitRepository& repo, const std::string& hash,
                 bool showContent, bool showType, bool showSize);
void handleLsRead(GitRepository& repo, const std::string& hash);
void handleLsTree(GitRepository& repo, const std::string& hash);

void handleAddCommand(GitRepository& repo, const std::vector<std::string>& paths);
void handleStatusCommand(GitRepository& repo, bool shortFormat,
                       bool showUntracked, bool showIgnore, bool showBranch);

void handleBranchCommand(GitRepository& repo, const std::string& branchName,
                       bool deleteFlag, bool forceDelete, bool listFlag,
                       bool showCurrent, const std::string& newBranchName);
void handleSwitchBranch(GitRepository& repo, const std::string& targetBranch, bool createFlag);
void handleCheckoutBranch(GitRepository& repo, const std::string& branchName, bool createFlag);




void setupAllCommands(CLI::App& app, GitRepository& repo);

// Command setup functions
void setupInitCommand(CLI::App& app, GitRepository& repo);
void setupHashObjectCommand(CLI::App& app, GitRepository& repo);
void setupWriteTreeCommand(CLI::App& app, GitRepository& repo);
void setupCommitTreeCommand(CLI::App& app, GitRepository& repo);
void setupTagObjectCommand(CLI::App& app, GitRepository& repo);
void setupReadObjectCommand(CLI::App& app, GitRepository& repo);
void setupCatFileCommand(CLI::App& app, GitRepository& repo);
void setupLsReadCommand(CLI::App& app, GitRepository& repo);
void setupLsTreeCommand(CLI::App& app, GitRepository& repo);
void setupAddCommand(CLI::App& app, GitRepository& repo);
void setupStatusCommand(CLI::App& app, GitRepository& repo);
void setupBranchCommand(CLI::App& app, GitRepository& repo);
void setupSwitchCommand(CLI::App& app, GitRepository& repo);
void setupCheckoutCommand(CLI::App& app, GitRepository& repo);
