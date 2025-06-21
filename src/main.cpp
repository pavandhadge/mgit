#include <iostream>
#include "utils/CLI11.hpp"
#include "headers/GitRepository.h"



// Function to handle 'hash-object' command
void handleHashObject(const std::string& filepath, bool write) {
    std::cout << "Running hash-object\n";
    std::cout << "Filepath: " << filepath << ", Write: " << (write ? "true" : "false") << "\n";
    // Call your hash object logic here
}

int main(int argc, char** argv) {

    GitRepository repo;

    CLI::App app{"yourgit - a minimal Git implementation"};

    // Init command
    std::string initPath = ".git";

    auto initCmd = app.add_subcommand("init", "Initialize a new Git repository");
    initCmd->add_option("path", initPath, "Directory to initialize")->default_val(".git");

    // hash-object command
    std::string hashPath;
    bool hashWrite = false;
    auto hashCmd = app.add_subcommand("hash-object", "Compute object hash");
    hashCmd->add_option("file", hashPath, "File to hash")->required();
    hashCmd->add_flag("-w,--write", hashWrite, "Write object to database");

    CLI11_PARSE(app, argc, argv);

    if (*initCmd) {
        repo.init(initPath);
    } else if (*hashCmd) {
        handleHashObject(hashPath, hashWrite);
    } else {
        std::cout << app.help() << std::endl;
    }

    return 0;
}
