#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>

enum class ConflictState {
    NONE,
    UNRESOLVED,
    RESOLVED
};

struct IndexEntry {
    std::string mode;
    std::string path;
    std::string hash;  // Our version
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

class IndexManager {
private:
    std::vector<IndexEntry> entries;                     // maintains order
    std::unordered_map<std::string, size_t> pathToIndex; // fast lookup: path → index in vector
    std::unordered_map<std::string, ConflictMarker> conflictMarkers;

public:
    // Core index methods
    bool readIndex();             // Populates entries and map
    void writeIndex();            // Writes vector entries to disk
    void addOrUpdateEntry(const IndexEntry& entry);
    const std::vector<IndexEntry>& getEntries() const;
    void printEntries() const;
    std::vector<std::pair<std::string, std::string>> computeStatus();
    IndexEntry gitIndexEntryFromPath(const std::string &path);

    // Conflict handling
    void recordConflict(const std::string& path, const IndexEntry& base, 
                        const IndexEntry& ours, const IndexEntry& theirs);
    bool hasConflicts() const;
    std::vector<std::string> getConflictingFiles() const;
    bool isConflicted(const std::string& path) const;
    bool resolveConflict(const std::string& path, const std::string& hash);
    void abortMerge();
    void addConflictMarker(const std::string& path, const ConflictMarker& marker);
    std::optional<ConflictMarker> getConflictMarker(const std::string& path) const;
};
