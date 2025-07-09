# mgit API Reference

## Table of Contents
- [Command Handlers](#command-handlers)
- [Repository Layer](#repository-layer)
- [Object Model](#object-model)
- [Branching & Merging](#branching--merging)
- [Index & HEAD](#index--head)
- [Configuration](#configuration)
- [Logging & Analytics](#logging--analytics)
- [Utilities](#utilities)

---

## Command Handlers

All CLI commands are registered and handled in `CLISetupAndHandlers`. Each handler function takes a `GitRepository` reference and command-specific arguments.

**Key Handlers:**
- `handleInit` — Initialize a new repository
- `handleHashObject` — Hash and optionally store a file
- `handleWriteTree` — Create a tree object from a directory
- `handleCommitTree` — Create a commit object
- `handleTagObject` — Create an annotated tag
- `handleReadObject` — Read a raw object
- `handleCatFile` — Inspect object type/content/size
- `handleLsRead` — Auto-detect and pretty-print object
- `handleLsTree` — Pretty-print a tree object
- `handleAddCommand` — Add files to the index
- `handleStatusCommand` — Show repository status
- `handleBranchCommand` — Branch management (create, delete, list, rename)
- `handleSwitchBranch` / `handleCheckoutBranch` — Switch branches
- `handleMergeCommand` — Start a merge
- `handleMergeContinue` / `handleMergeAbort` — Continue/abort merge
- `handlePushCommand` / `handlePullCommand` — Push/pull to/from remote
- `handleRemoteAdd` / `handleRemoteRemove` / `handleRemoteList` — Manage remotes
- `handleConfigSet` / `handleConfigGet` — Set/get config values
- `handleCommitCommand` — Create a commit (high-level)

---

## Repository Layer

### `GitRepository`
Central class for repository state and operations.
- **init(path)**: Initialize a new repository at `path`.
- **writeObject(type, path/data, write)**: Write blob/tree/commit/tag objects.
- **readObject(type, hash)**: Read and deserialize objects.
- **reportStatus(...)**: Show status (short/long, untracked, ignored, branch info).
- **Branch management**: Create, delete, rename, list, and switch branches.
- **Commit management**: Create commits, log history, checkout specific commits.
- **Merge operations**: Start, abort, and resolve merges.
- **Push/Pull**: Sync with remote repositories.

---

## Object Model

### `GitObjectStorage`
- **readObject(hash)**: Read object content by hash.
- **writeObject(hash, content)**: Write object content by hash.
- **objectExists(hash)**: Check if object exists.
- **validateObjectIntegrity(hash)**: Check object integrity.
- **listAllObjects()**: List all stored objects.

### `GitObjectTypesClasses` and Subclasses
- **BlobObject**: Handles file blobs.
- **TreeObject**: Handles directory trees.
- **CommitObject**: Handles commit objects.
- **TagObject**: Handles annotated tags.

---

## Branching & Merging

### `GitBranch` / `Branch`
- **createBranch(name)**: Create a new branch.
- **checkout(name)**: Switch to a branch.
- **listBranches()**: List all branches.
- **deleteBranch(name)**: Delete a branch.
- **renameBranch(old, new)**: Rename a branch.

### `GitMerge`
- **checkForConflicts(current, target)**: Detect merge conflicts.
- **threeWayMerge(current, target, ancestor)**: Perform a three-way merge.
- **getConflictingFiles()**: List files with conflicts.
- **getFileConflictStatus(filename)**: Get conflict status for a file.
- **mergeTrees(...)**: Merge tree objects.

---

## Index & HEAD

### `GitIndex` / `IndexManager`
- **readIndex() / writeIndex()**: Load/save index from disk.
- **addOrUpdateEntry(entry)**: Add or update an index entry.
- **getEntries()**: Get all index entries.
- **computeStatus()**: Compute status for working directory.
- **Conflict handling**: Record, resolve, and query conflicts.

### `GitHead`
- **readHead()**: Load current HEAD state.
- **updateHead(hash)**: Update HEAD to a new commit.
- **writeHeadToHeadOfNewBranch(branch)**: Switch HEAD to a new branch.

---

## Configuration

### `GitConfig`
- **setConfig(key, value)**: Set a config value.
- **getConfig(key, value)**: Get a config value.
- **listConfig()**: List all config values.
- **removeConfig(key)**: Remove a config value.
- **addRemote(name, path)**: Add a remote.
- **removeRemote(name)**: Remove a remote.
- **listRemotes()**: List all remotes.
- **setUserName(name) / setUserEmail(email)**: Set user info.

---

## Logging & Analytics

### `GitActivityLogger`
- **startCommand(command, args)**: Begin logging a command.
- **endCommand(result, exit_code, error_msg)**: End logging for a command.
- **logRepositoryState()**: Log current repository state.
- **logObjectCreation(type, hash, path)**: Log object creation.
- **logFileModification(path, operation)**: Log file changes.
- **logBranchOperation(branch, operation)**: Log branch changes.
- **logMergeOperation(target, status, conflicts)**: Log merge events.
- **logPerformanceMetrics(metrics)**: Log performance data.
- **logError(type, message, stack_trace)**: Log errors.
- **getRecentActivity(limit)**: Query recent activity.
- **getErrors(limit)**: Query recent errors.
- **generateLogSummary()**: Get a summary of all activity.
- **exportToCSV(path)**: Export logs for analysis.

---

## Utilities

### `ZlibUtils`
- **compressZlib(input)**: Compress data.
- **decompressZlib(compressed)**: Decompress data.
- **hash_sha1(data)**: SHA-1 hash utility.
- **getCurrentTimestampWithTimezone()**: Get timestamp.
- **hexToBinary(hex) / binaryToHex(binary)**: Convert between hex and binary.

### `HashUtils`
- **hash_sha1(data)**: SHA-1 hash utility (duplicate for convenience).

---

## Data Structures

- **CommitData**: Tree, parents, author, committer, message.
- **TagData**: Object hash, type, tag name, tagger, message.
- **BlobData**: File content.
- **TreeEntry**: Mode, filename, hash.
- **IndexEntry**: Mode, path, hash, base_hash, their_hash, conflict state, marker.
- **ConflictMarker**: Base, ours, theirs content for merge conflicts.
- **ActivityRecord**: Full log of a command execution.
- **PerformanceMetrics**: Memory, CPU, files processed, etc.
- **RepositoryState**: HEAD, branch, commit counts, etc.

---

## See Also
- [ARCHITECTURE.md](ARCHITECTURE.md) for high-level design and data flow. 