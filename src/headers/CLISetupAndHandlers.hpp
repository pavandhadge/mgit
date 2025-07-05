#pragma once
#include "GitRepository.hpp"
#include "../utils/CLI11.hpp"




bool handleInit(GitRepository& repo, const std::string& path);

bool handleHashObject(GitRepository& repo, const std::string& filepath, bool write);
bool handleWriteTree(GitRepository& repo, const std::string& folder);
bool handleCommitTree(GitRepository& repo, const std::string& tree,
                    const std::string& parent, const std::string& message,
                    const std::string& author);
bool handleTagObject(GitRepository& repo, const std::string& targetHash,
                   const std::string& targetType, const std::string& tagName,
                   const std::string& tagMessage, const std::string& tagger);
bool handleReadObject(GitRepository& repo, const std::string& hash);

bool handleCatFile(GitRepository& repo, const std::string& hash,
                 bool showContent, bool showType, bool showSize);
bool handleLsRead(GitRepository& repo, const std::string& hash);
bool handleLsTree(GitRepository& repo, const std::string& hash);

bool handleAddCommand(GitRepository& repo, const std::vector<std::string>& paths);
bool handleStatusCommand(GitRepository& repo, bool shortFormat,
                       bool showUntracked, bool showIgnore, bool showBranch);

bool handleBranchCommand(GitRepository& repo, const std::string& branchName,
                       bool deleteFlag, bool forceDelete, bool listFlag,
                       bool showCurrent, const std::string& newBranchName);
bool handleSwitchBranch(GitRepository& repo, const std::string& targetBranch, bool createFlag);
bool handleCheckoutBranch(GitRepository& repo, const std::string& branchName, bool createFlag);

bool handleMergeCommand(GitRepository& repo, const std::string& targetBranch);
bool handleMergeContinue(GitRepository& repo);
bool handleMergeAbort(GitRepository& repo);
bool handleMergeStatus(GitRepository& repo);
bool handleResolveConflict(GitRepository& repo, const std::string& path, const std::string& hash);
bool handleResolveConflicts(GitRepository& repo);




bool setupAllCommands(CLI::App& app, GitRepository& repo);

// Command setup functions
bool setupCLIAppHelp(CLI::App& app);
bool setupInitCommand(CLI::App& app, GitRepository& repo);
bool setupHashObjectCommand(CLI::App& app, GitRepository& repo);
bool setupWriteTreeCommand(CLI::App& app, GitRepository& repo);
bool setupCommitTreeCommand(CLI::App& app, GitRepository& repo);
bool setupTagObjectCommand(CLI::App& app, GitRepository& repo);
bool setupReadObjectCommand(CLI::App& app, GitRepository& repo);
bool setupCatFileCommand(CLI::App& app, GitRepository& repo);
bool setupLsReadCommand(CLI::App& app, GitRepository& repo);
bool setupLsTreeCommand(CLI::App& app, GitRepository& repo);
bool setupAddCommand(CLI::App& app, GitRepository& repo);
bool setupStatusCommand(CLI::App& app, GitRepository& repo);
bool setupBranchCommand(CLI::App& app, GitRepository& repo);
bool setupSwitchCommand(CLI::App& app, GitRepository& repo);
bool setupCheckoutCommand(CLI::App& app, GitRepository& repo);
bool setupMergeCommand(CLI::App& app, GitRepository& repo);
bool setupMergeContinueCommand(CLI::App& app, GitRepository& repo);
bool setupMergeAbortCommand(CLI::App& app, GitRepository& repo);
bool setupMergeStatusCommand(CLI::App& app, GitRepository& repo);
bool setupResolveConflictCommand(CLI::App& app, GitRepository& repo);
