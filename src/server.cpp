#include <exception>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "headers/util.h"  // compressZlib, decompressZlib, hash_sha1

int main(int argc, char *argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Error: No command provided.\n";
        return EXIT_FAILURE;
    }

    std::string command = argv[1];

    if (command == "init") {
        // Validate args — no args needed for init
        try {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");

            std::ofstream headFile(".git/HEAD");
            if (!headFile.is_open()) {
                std::cerr << "Error: Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
            headFile << "ref: refs/heads/main\n";
            headFile.close();

            std::cout << "Initialized git directory\n";
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Filesystem error during init: " << e.what() << "\n";
            return EXIT_FAILURE;
        }
    }else if (command == "cat-file") {
        if (argc != 4) {
            std::cerr << "Usage: ./yourgit cat-file -p <hash>\n";
            return EXIT_FAILURE;
        }

        std::string flag = argv[2];
        std::string hash = argv[3];

        if (flag != "-p") {
            std::cerr << "Error: Unsupported flag '" << flag << "'. Use -p.\n";
            return EXIT_FAILURE;
        }

        // Risky parts in try-catch:
        try {
            std::string blobPath = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
            if (!std::filesystem::exists(blobPath)) {
                std::cerr << "Error: Blob file not found: " << blobPath << "\n";
                return EXIT_FAILURE;
            }

            std::ifstream blobFile(blobPath, std::ios::binary);
            if (!blobFile.is_open()) {
                std::cerr << "Error: Cannot open blob file: " << blobPath << "\n";
                return EXIT_FAILURE;
            }

            std::vector<char> compressedData((std::istreambuf_iterator<char>(blobFile)),
                                              std::istreambuf_iterator<char>());
            blobFile.close();

            std::string decompressed = decompressZlib(compressedData);

            size_t nullPos = decompressed.find('\0');
            if (nullPos == std::string::npos) {
                std::cerr << "Error: Invalid blob object (missing null byte).\n";
                return EXIT_FAILURE;
            }

            std::string content = decompressed.substr(nullPos + 1);
            std::cout << content;
        } catch (const std::exception& e) {
            std::cerr << "Error during cat-file: " << e.what() << "\n";
            return EXIT_FAILURE;
        }
    }else if (command == "hash-object") {
        if (argc != 4) {
            std::cerr << "Usage: ./yourgit hash-object -w <path>\n";
            return EXIT_FAILURE;
        }

        std::string flag = argv[2];
        std::string path = argv[3];

        if (flag != "-w") {
            std::cerr << "Error: Unsupported flag '" << flag << "'. Use -w.\n";
            return EXIT_FAILURE;
        }

        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: File does not exist: " << path << "\n";
            return EXIT_FAILURE;
        }

        try {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Error: Unable to open file: " << path << "\n";
                return EXIT_FAILURE;
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();
            std::string fileData = buffer.str();
            file.close();

            std::string blobHeader = "blob " + std::to_string(fileData.size()) + '\0';
            std::string fullBlob = blobHeader + fileData;

            std::string hash = hash_sha1(fullBlob);
            std::string compressed = compressZlib(fullBlob);

            std::string objDir = ".git/objects/" + hash.substr(0, 2);

            if (!std::filesystem::exists(".git/objects")) {
                std::cerr << "Error: .git/objects directory does not exist. Did you run `init`?\n";
                return EXIT_FAILURE;
            }

            std::error_code ec;
            if (!std::filesystem::exists(objDir) && !std::filesystem::create_directories(objDir, ec)) {
                std::cerr << "Error: Failed to create directory " << objDir << ": " << ec.message() << "\n";
                return EXIT_FAILURE;
            }

            std::string objPath = objDir + "/" + hash.substr(2);

            std::ofstream outFile(objPath, std::ios::binary);
            if (!outFile.is_open()) {
                std::cerr << "Error: Failed to write blob file: " << objPath << "\n";
                return EXIT_FAILURE;
            }

            outFile.write(compressed.data(), compressed.size());
            outFile.close();

            std::cout << hash << "\n";

        } catch (const std::exception& e) {
            std::cerr << "Error during hash-object: " << e.what() << "\n";
            return EXIT_FAILURE;
        }
    }else if(command == "ls-tree"){
        if (argc != 4) {
            std::cerr << "Usage: ./yourgit ls-tree --name-only <hash>\n";
            return EXIT_FAILURE;
        }

        std::string flag = argv[2];
        std::string hash = argv[3];
        std::string path = ".git/objects/"+hash.substr(0,2)+"/"+hash.substr(2) ;
        if (flag != "--name-only") {
            std::cerr << "Error: Unsupported flag '" << flag << "'. Use --name-only.\n";
            return EXIT_FAILURE;
        }
        try {

        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: File does not exist: " << path << "\n";
            return EXIT_FAILURE;
        }
        std::ifstream treeblob(path,std::ios::binary);
        if(!treeblob.is_open()){
            throw std::runtime_error("unable to open file at path : "+ path);
        }
        std::vector<char> compressed_treeblob((std::istreambuf_iterator<char>(treeblob)),std::istreambuf_iterator<char>());
        std::string decompressed_treeblob = decompressZlib(compressed_treeblob);
        // std::cout << "decompressed treeblob : "<<decompressed_treeblob<<std::endl;
        size_t nullPos = decompressed_treeblob.find('\0');
        if (nullPos == std::string::npos) {
            std::cerr << "Error: Invalid blob object (missing null byte).\n";
            return EXIT_FAILURE;
        }
        decompressed_treeblob = decompressed_treeblob.substr(nullPos+1);
        size_t i = nullPos + 1;

               // Parse entries
               while (i < decompressed_treeblob.size()) {
                   std::string mode;
                   while (decompressed_treeblob[i] != ' ') {
                       mode += decompressed_treeblob[i++];
                   }
                   i++; // Skip space

                   std::string filename;
                   while (decompressed_treeblob[i] != '\0') {
                       filename += decompressed_treeblob[i++];
                   }
                   i++; // Skip null terminator

                   std::cout << filename << "\n";

                   std::ostringstream hex_hash;
                   for (int j = i; j < i + 20; j++) {
                       hex_hash << std::hex << std::setw(2) << std::setfill('0')
                                << (static_cast<unsigned int>(static_cast<unsigned char>(decompressed_treeblob[j])));
                   }
                   std::cout << hex_hash.str() << "\n";

                   i+=20;

               }

        // std::cout << "decompressed treeblob formated :\n "<<decompressed_treeblob<<std::endl;
        }catch(const std::exception &e){
            std::cerr << "Error during hash-object: " << e.what() << "\n";
            return EXIT_FAILURE;
        }
    }else {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
