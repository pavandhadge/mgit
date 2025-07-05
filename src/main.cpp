#include "headers/CLISetupAndHandlers.hpp"
#include "headers/GitRepository.hpp"
#include "headers/GitActivityLogger.hpp"
#include "utils/CLI11.hpp"
#include <iostream>
#include <exception>
#include <vector>

int main(int argc, char** argv) {
    try {
        CLI::App app{"MGit - A Git clone implementation"};
        
        GitRepository repo;
        GitActivityLogger logger;
        
        // Log the command start
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        
        if (argc > 1) {
            logger.startCommand(argv[1], args);
        }
        
        if (!setupAllCommands(app, repo)) {
            std::cerr << "Failed to setup CLI commands" << std::endl;
            logger.endCommand("Failed to setup CLI commands", 1, "CLI setup failed");
            return 1;
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
        
        return exit_code;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
