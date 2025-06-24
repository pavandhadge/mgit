
#include "headers/GitObjectStorage.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include "headers/HashUtils.hpp"


BlobObject::BlobObject() : type(GitObjectType::Blob), content({}) {}

std::string BlobObject::writeObject(const std::string& path, const bool &write) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file: " << path << "\n";
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();

    BlobData data;
    data.content = buffer.str();
    content = data;

    std::string blobHeader = "blob " + std::to_string(data.content.size()) + '\0';
    std::string fullBlob = blobHeader + data.content;

    if (write) {
        return GitObjectStorage::writeObject(fullBlob);
    }
    return hash_sha1(fullBlob);
}

BlobData BlobObject::readObject(const std::string& hash) {
    BlobData data;
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid blob object (missing null byte).\n";
        return data;
    }

    data.content = decompressed.substr(nullPos + 1);
    content = data;
    return data;
}

const BlobData& BlobObject::getContent() const {
    return content;
}

GitObjectType BlobObject::getType() const {
    return type;
}

TreeObject::TreeObject() : type(GitObjectType::Tree), content({}) {}

std::string TreeObject::writeObject(const std::string& path) {
    std::vector<TreeEntry> entries;

    if (!std::filesystem::exists(path)) {
        std::cerr << "no such directory to make tree \n";
        return "";
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        TreeEntry treeEntry;
        treeEntry.filename = entry.path().filename();

        if (std::filesystem::is_regular_file(entry)) {
            treeEntry.mode = "100644";
            BlobObject blob;
            treeEntry.hash = blob.writeObject(entry.path().string(), true);
        } else if (std::filesystem::is_directory(entry)) {
            treeEntry.mode = "040000";
            TreeObject subTree;
            treeEntry.hash = subTree.writeObject(entry.path().string());
        } else {
            continue;
        }

        entries.push_back(treeEntry);
    }

    std::string binaryTreeContent;
    for (const auto& entry : entries) {
        binaryTreeContent += entry.mode + " " + entry.filename + "\0";
        for (int i = 0; i < 40; i += 2) {
            std::string bytestr = entry.hash.substr(i, 2);
            char byte = static_cast<char>(std::stoi(bytestr, nullptr, 16));
            binaryTreeContent += byte;
        }
    }

    std::string header = "tree " + std::to_string(binaryTreeContent.size()) + '\0';
    std::string full = header + binaryTreeContent;
    content = entries;
    return GitObjectStorage::writeObject(full);
}

std::vector<TreeEntry> TreeObject::readObject(const std::string& hash) {
    std::vector<TreeEntry> entries;
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid tree object (missing null byte).\n";
        return entries;
    }

    std::string treeBody = decompressed.substr(nullPos + 1);
    size_t i = 0;

    while (i < treeBody.size()) {
        TreeEntry entry;
        while (treeBody[i] != ' ') entry.mode += treeBody[i++];
        i++;
        while (treeBody[i] != '\0') entry.filename += treeBody[i++];
        i++;

        std::ostringstream hexHash;
        for (int j = 0; j < 20; ++j) {
            hexHash << std::hex << std::setw(2) << std::setfill('0')
                    << (static_cast<unsigned int>(static_cast<unsigned char>(treeBody[i + j])));
        }
        entry.hash = hexHash.str();
        i += 20;

        entries.push_back(entry);
    }

    content = entries;
    return entries;
}

const std::vector<TreeEntry>& TreeObject::getContent() const {
    return content;
}

GitObjectType TreeObject::getType() const {
    return type;
}

bool TreeObject::restoreWorkingDirectoryFromTreeHash(const std::string &hash, const std::string &path) {
    std::vector<TreeEntry> entities = readObject(hash);

    for (const TreeEntry &entity : entities) {
        std::string fullPath = path + "/" + entity.filename;

        if (entity.mode == "100644" || entity.mode == "100755") {
            // It's a file (blob)
            BlobObject blobObj;
            BlobData blob = blobObj.readObject(entity.hash);

            std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());

            std::ofstream outFile(fullPath, std::ios::binary);
            outFile << blob.content;
            outFile.close();
        } else if (entity.mode == "040000") {
            // It's a subdirectory (tree)
            std::filesystem::create_directories(fullPath);

            TreeObject subtree;
            subtree.restoreWorkingDirectoryFromTreeHash(entity.hash, fullPath);  // 🔁 recursive
        }
    }

    return true;
}




//Commit)bject Class
CommitObject::CommitObject() : type(GitObjectType::Commit), content({}) {}

std::string CommitObject::writeObject(const CommitData& data) {
    std::ostringstream commitText;

    // Tree
    commitText << "tree " << data.tree << "\n";

    // Parents (can be zero or many)
    for (const auto& parent : data.parents) {
        commitText << "parent " << parent << "\n";
    }

    // Author (must be present)
    commitText << "author " << data.author << "\n";

    // Committer (must be present)
    commitText << "committer " << data.committer << "\n";

    // Message (separated by a blank line)
    commitText << "\n" << data.message << "\n";
    std::string body = commitText.str();
    std::string header = "commit " + std::to_string(body.size()) + '\0';
    content = data;
    std::string fullContent = header+body;
    return GitObjectStorage::writeObject(fullContent);
}


CommitData CommitObject::readObject(const std::string& hash) {
    CommitData data;
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid commit object (missing null byte).\n";
        return data;
    }

    std::string body = decompressed.substr(nullPos + 1);
    std::istringstream iss(body);
    std::string line;
    bool inMessage = false;

    while (std::getline(iss, line)) {
        if (line.empty()) {
            inMessage = true;
            continue;
        }

        if (inMessage) {
            data.message += line + "\n";
            continue;
        }

        if (line.rfind("tree ", 0) == 0) {
            data.tree = line.substr(5);
        } else if (line.rfind("parent ", 0) == 0) {
            data.parents.push_back(line.substr(7));
        } else if (line.rfind("author ", 0) == 0) {
            data.author = line.substr(7);
        } else if (line.rfind("committer ", 0) == 0) {
            data.committer = line.substr(10);
        }
    }

    // Trim final newline from message if present
    if (!data.message.empty() && data.message.back() == '\n') {
        data.message.pop_back();
    }

    content = data;  // optional: keep original content
    return data;
}


const CommitData& CommitObject::getContent() const {
    return content;
}

GitObjectType CommitObject::getType() const {
    return type;
}



//tag object
//

TagObject::TagObject() : type(GitObjectType::Tag), content({}) {}

std::string TagObject::writeObject(const TagData& data) {
    std::ostringstream tagStream;

    tagStream << "object " << data.objectHash << "\n";
    tagStream << "type " << data.objectType << "\n";
    tagStream << "tag " << data.tagName << "\n";
    tagStream << "tagger " << data.tagger << "\n";
    tagStream << "\n" << data.message << "\n";

    std::string body = tagStream.str();  // cache it
    content = data;

    std::string header = "tag " + std::to_string(body.size()) + '\0';
    std::string fullTag = header + body;

    return GitObjectStorage::writeObject(fullTag);
}




TagData TagObject::readObject(const std::string& hash) {
    TagData tag;
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid tag object (missing null byte).\n";
        return tag;
    }

    std::string body = decompressed.substr(nullPos + 1);
    std::istringstream iss(body);
    std::string line;
    bool inMessage = false;

    while (std::getline(iss, line)) {
        if (line.empty()) {
            inMessage = true;
            continue;
        }

        if (inMessage) {
            tag.message += line + "\n";
            continue;
        }

        if (line.rfind("object ", 0) == 0) {
            tag.objectHash = line.substr(7);
        } else if (line.rfind("type ", 0) == 0) {
            tag.objectType = line.substr(5);
        } else if (line.rfind("tag ", 0) == 0) {
            tag.tagName = line.substr(4);
        } else if (line.rfind("tagger ", 0) == 0) {
            tag.tagger = line.substr(7);
        }
    }

    // Remove trailing newline from message
    if (!tag.message.empty() && tag.message.back() == '\n') {
        tag.message.pop_back();
    }

    content = tag;
    return tag;
}



const TagData TagObject::getContent() const {
    return content;
}

GitObjectType TagObject::getType() const {
    return type;
}
