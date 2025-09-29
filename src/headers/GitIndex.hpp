#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

enum class ConflictState { NONE, UNRESOLVED, RESOLVED };

struct IndexEntry {
  std::string mode;
  std::string path;
  std::string hash;       // Our version
  std::string base_hash;  // Base version
  std::string their_hash; // Their version
  ConflictState conflict_state = ConflictState::NONE;
  std::string conflict_marker; // For file conflicts
};

struct ConflictMarker {
  std::string base_content;
  std::string our_content;
  std::string their_content;
};

struct StatusResult {
  std::vector<std::pair<std::string, std::string>> staged_changes;
  std::vector<std::pair<std::string, std::string>> unstaged_changes;
  std::vector<std::string> untracked_files;
};

class IndexManager {
private:
  std::string gitDir;
  std::vector<IndexEntry> entries; // maintains order
  std::unordered_map<std::string, size_t>
      pathToIndex; // fast lookup: path → index in vector
  std::unordered_map<std::string, ConflictMarker> conflictMarkers;

public:
  IndexManager(const std::string &gitDir = ".git");
  // Core index methods
  bool readIndex();  // Populates entries and map
  void writeIndex(); // Writes vector entries to disk
  void addOrUpdateEntry(const IndexEntry &entry);
  const std::vector<IndexEntry> &getEntries() const;
  void printEntries() const;
  StatusResult computeStatus(const std::string &headTreeHash);
  IndexEntry gitIndexEntryFromPath(const std::string &path);

  // Conflict handling
  void recordConflict(const std::string &path, const IndexEntry &base,
                      const IndexEntry &ours, const IndexEntry &theirs);
  bool hasConflicts() const;
  std::vector<std::string> getConflictingFiles() const;
  bool isConflicted(const std::string &path) const;
  bool resolveConflict(const std::string &path, const std::string &hash);
  void abortMerge();
  void addConflictMarker(const std::string &path, const ConflictMarker &marker);
  std::optional<ConflictMarker>
  getConflictMarker(const std::string &path) const;
};
