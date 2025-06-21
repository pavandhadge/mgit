#include "headers/GitConfig.hpp"
#include "headers/GitObjectStorage.hpp"
#include "headers/ZlibUtils.hpp"
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

std::string CommitObject::writeObject(const std::string& currentHash ,const std::string& parentHash ,const std::string &commitMassage,const std::string &author) {
   std::ostringstream commitText ;
   GitConfig commiter ;

   commitText << "tree "<< currentHash;
   commitText <<"parent "<<parentHash;
   if(author==""){
       commitText <<"author "<< commiter.getName() << " " << commiter.getEmail() << " " << getCurrentTimestampWithTimezone();
   }else{

   commitText <<"author "<<author<<" "<<getCurrentTimestampWithTimezone();
   }
   commitText <<"commiter "<< commiter.getName() << " " << commiter.getEmail() << " " << getCurrentTimestampWithTimezone();

   commitText << "\n"<<commitMassage;

   content = commitText.str();
    return GitObjectStorage::writeObject(content);
}

std::string CommitObject::readObject(const std::string& hash) {
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid commit object (missing null byte).\n";
        return "";
    }

    content = decompressed.substr(nullPos+1);
    /*
    // std::string body = decompressed.substr(nullPos + 1);
     * this is code for making hte commit readable

    std::istringstream iss(body);
    std::ostringstream readable;
    std::string line;
    bool inMessage = false;
    std::string commitMsg;

    while (std::getline(iss, line)) {
        if (line.empty()) {
            inMessage = true;
            continue;
        }

        if (inMessage) {
            commitMsg += line + "\n";
            continue;
        }

        if (line.rfind("tree ", 0) == 0) {
            readable << "Tree: " << line.substr(5) << "\n";
        } else if (line.rfind("parent ", 0) == 0) {
            readable << "Parent: " << line.substr(7) << "\n";
        } else if (line.rfind("author ", 0) == 0) {
            readable << "Author: " << line.substr(7) << "\n";
        } else if (line.rfind("committer ", 0) == 0) {
            readable << "Committer: " << line.substr(10) << "\n";
        }
    }

    if (!commitMsg.empty()) {
        readable << "Message:\n" << commitMsg;
    }

    content = readable.str();  // store parsed version
     */
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
    std::string treeContent ;
    if(!std::filesystem::exists(path)){
        std::cerr<<"no such directry to make tree "<<std::endl;
        return "";
    }
    for(const auto  &entry : std::filesystem::directory_iterator(path)){
        std::string mode ;
        std::string hash ;
        std::string filename = entry.path().filename();

        if(std::filesystem::is_regular_file(entry)){
            mode = "100644";
            BlobObject blob;
            hash = blob.writeObject(entry.path().string());
        }else if(std::filesystem::is_directory(entry)){
            mode = "040000";
            TreeObject subTree ;
            hash = subTree.writeObject(entry.path().string());
        }else{
            continue;
        }
        treeContent += mode+" "+filename+"\0";
        for(int i=0 ; i<40;i+=2){
            std::string bytestr ;
            char byte = static_cast<char>(std::stoi(bytestr, nullptr, 16));
            treeContent += byte;
        }
    }
    std::string header = "tree " + std::to_string(treeContent.size()) + '\0';
       std::string full = header + treeContent;

       content = full; // store full binary content if needed

       return GitObjectStorage::writeObject(full); // write, get hash
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

std::string TagObject::writeObject(const std::string& targetHash,
                                   const std::string& targetType,
                                   const std::string& tagName,
                                   const std::string& tagMessage,
                                   const std::string& taggerLine) {
    GitConfig config;
    std::ostringstream tagData;

    tagData << "object " << targetHash << "\n";
    tagData << "type " << targetType << "\n";
    tagData << "tag " << tagName << "\n";

    if (taggerLine.empty()) {
        tagData << "tagger " << config.getName() << " <" << config.getEmail() << "> "
                << getCurrentTimestampWithTimezone() << "\n";
    } else {
        tagData << "tagger " << taggerLine << " " << getCurrentTimestampWithTimezone() << "\n";
    }

    tagData << "\n" << tagMessage << "\n";

    content = tagData.str();
    std::string header = "tag " + std::to_string(content.size()) + '\0';
    std::string fullTag = header + content;

    return GitObjectStorage::writeObject(fullTag);
}


std::string TagObject::readObject(const std::string& hash) {
    std::string decompressed = GitObjectStorage::readObject(hash);

    size_t nullPos = decompressed.find('\0');
    if (nullPos == std::string::npos) {
        std::cerr << "Error: Invalid tag object (missing null byte).\n";
        return "";
    }

    content = decompressed.substr(nullPos + 1);

    /*
     * code to make the tag readable

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

     */
    return content;
}


const std::string& TagObject::getContent() const {
    return content;
}

GitObjectType TagObject::getType() const {
    return type;
}
