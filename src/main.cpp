#include "headers/CLISetupAndHandlers.hpp"
#include "headers/GitRepository.hpp"
#include "headers/GitActivityLogger.hpp"
#include "utils/CLI11.hpp"
#include <iostream>
#include <exception>
#include <vector>
#include <filesystem>

int main(int argc, char** argv) {
    try {
        CLI::App app{"MGit - A Git clone implementation"};
        GitRepository* repo = nullptr;
        GitActivityLogger logger;
        
        // Log the command start
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        
        if (argc > 1) {
            logger.startCommand(argv[1], args);
        }
        
        // Use a pointer for CLI setup and update it later if needed
        GitRepository dummyRepo;
        GitRepository* repoPtr = &dummyRepo;
        setupAllCommands(app, repoPtr);
        
        // Parse command to see if .git is needed
        bool needsRepo = true;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h" || arg == "help") {
                needsRepo = false;
                break;
            }
            if (arg == "init" || arg == "remote" || arg == "config") {
                needsRepo = false;
                break;
            }
        }
        if (needsRepo && !std::filesystem::exists(".git")) {
            std::cerr << "Fatal error: Git directory does not exist: .git\n";
            return 1;
        }
        if (needsRepo) {
            repo = new GitRepository();
            repoPtr = repo;
        }
        
        int exit_code = 0;
        std::string result = "Success";
        std::string error_msg = "";
        
        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            exit_code = app.exit(e);
            result = "CLI parse error";
            error_msg = e.what();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            exit_code = 1;
            result = "Exception occurred";
            error_msg = e.what();
        }
        
        // Log the command end
        if (argc > 1) {
            logger.endCommand(result, exit_code, error_msg);
        }
        
        if (repo) delete repo;
        return exit_code;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
