#include <string>
#include <unordered_map>
#include <vector>
struct IndexEntry {
    std::string mode;
    std::string path;
    std::string hash;
};

class IndexManager {
private:
    std::vector<IndexEntry> entries;                     // maintains order
    std::unordered_map<std::string, size_t> pathToIndex; // fast lookup: path → index in vector

public:
    // Core index methods
    void readIndex();             // Populates entries and map
    void writeIndex();            // Writes vector entries to disk
    void addOrUpdateEntry(const IndexEntry& entry);
    const std::vector<IndexEntry>& getEntries() const;   // ← This is your return
};
