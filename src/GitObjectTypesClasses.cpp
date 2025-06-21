#include "headers/GitObjectStorage.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

BlobObject::BlobObject() : type(GitObjectType::Blob), content("") {}

std::string BlobObject::writeObject(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file: " << path << "\n";
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();  // Store in content member
    file.close();

    std::string blobHeader = "blob " + std::to_string(content.size()) + '\0';
    std::string fullBlob = blobHeader + content;

    return GitObjectStorage::writeObject(fullBlob);
}

std::string BlobObject::readObject(const std::string& hash) {
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid blob object (missing null byte).\n";
        return "";
    }

    // std::string header = decompressed.substr(0, nullPos); //not required now
    content = decompressed.substr(nullPos + 1);  // Store in content
    // type = GitObjectType::Blob;  // Since we're BlobObject anyway //already set

    return content;
}

const std::string& BlobObject::getContent() const {
    return content;
}

GitObjectType BlobObject::getType() const {
    return type;
}




//Commit)bject Class
CommitObject::CommitObject() : type(GitObjectType::Commit), content("") {}

std::string CommitObject::writeObject(const std::string& path) {
    // yet to return
    return "";
}

std::string CommitObject::readObject(const std::string& hash) {
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid commit object (missing null byte).\n";
        return "";
    }

    content = decompressed.substr(nullPos + 1);  // Store actual commit content
    return content;
}

const std::string& CommitObject::getContent() const {
    return content;
}

GitObjectType CommitObject::getType() const {
    return type;
}

// TreeObject Class
TreeObject::TreeObject() : type(GitObjectType::Tree), content("") {}

std::string TreeObject::writeObject(const std::string& path) {
    // yet to implement
    return "";
}

std::string TreeObject::readObject(const std::string& hash) {
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid tree object (missing null byte).\n";
        return "";
    }

    std::string treeBody = decompressed.substr(nullPos + 1); // Skip header
    std::ostringstream readableContent;

    size_t i = 0;
    while (i < treeBody.size()) {
        std::string mode;
        while (treeBody[i] != ' ') {
            mode += treeBody[i++];
        }
        i++; // skip space

        std::string filename;
        while (treeBody[i] != '\0') {
            filename += treeBody[i++];
        }
        i++; // skip null terminator

        // Extract and convert 20-byte binary SHA-1 to hex
        std::ostringstream hexHash;
        for (int j = 0; j < 20; ++j) {
            hexHash << std::hex << std::setw(2) << std::setfill('0')
                    << (static_cast<unsigned int>(static_cast<unsigned char>(treeBody[i + j])));
        }

        i += 20; // move past hash

        // Store entry in readable form: <mode> <filename> <hash>
        readableContent << mode << " " << filename << " " << hexHash.str() << "\n";
    }

    content = readableContent.str();  // Store for getContent()
    return content;
}


const std::string& TreeObject::getContent() const {
    return content;
}

GitObjectType TreeObject::getType() const {
    return type;
}


//tag object
//

TagObject::TagObject() : type(GitObjectType::Tag), content("") {}

std::string TagObject::writeObject(const std::string& path) {
    //yet to be implemented
   return "";
}

std::string TagObject::readObject(const std::string& hash) {
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid tag object (missing null byte).\n";
        return "";
    }

    std::string tagBody = decompressed.substr(nullPos + 1);
    std::istringstream iss(tagBody);
    std::ostringstream formatted;

    std::string line;
    std::string tagMessage;
    bool inMessage = false;

    while (std::getline(iss, line)) {
        if (line.empty()) {
            inMessage = true;
            continue;
        }

        if (inMessage) {
            tagMessage += line + "\n";
            continue;
        }

        if (line.rfind("object ", 0) == 0) {
            formatted << "Object: " << line.substr(7) << "\n";
        } else if (line.rfind("type ", 0) == 0) {
            formatted << "Type: " << line.substr(5) << "\n";
        } else if (line.rfind("tag ", 0) == 0) {
            formatted << "Tag: " << line.substr(4) << "\n";
        } else if (line.rfind("tagger ", 0) == 0) {
            formatted << "Tagger: " << line.substr(7) << "\n";
        }
    }

    if (!tagMessage.empty()) {
        formatted << "Message:\n" << tagMessage;
    }

    content = formatted.str();  // store in object for getContent()
    return content;
}


const std::string& TagObject::getContent() const {
    return content;
}

GitObjectType TagObject::getType() const {
    return type;
}
