#include "headers/GitRepository.h"
#include "headers/GitInit.h"

GitRepository::GitRepository(const std::string& root) : gitDir(root) {}

void GitRepository::init(const std::string& path) {
    gitDir = path; // set gitDir first
    GitInit objInit(gitDir);
    objInit.run();
}
