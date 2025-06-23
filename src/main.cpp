
#include "headers/GitRepository.hpp"
#include "utils/CLI11.hpp"
#include "headers/CLISetupAndHandlers.hpp"

int main(int argc, char** argv) {
    CLI::App app{"yourgit - a minimal Git implementation"};
    GitRepository repo;

    setupAllCommands(app, repo);

    CLI11_PARSE(app, argc, argv);
    return 0;
}
