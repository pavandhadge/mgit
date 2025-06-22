#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "headers/GitIndex.hpp"
#include "headers/GitObjectStorage.hpp"

void IndexManager::readIndex(){
    std::string path = ".git/INDEX";
    if(!std::filesystem::exists(path)){
        std::cerr<<"INDEX file does nor exist"<<std::endl;
        return;
    // }
    // std::ifstream index(path);
    // if (!index.is_open()) {
    //         std::cerr << "Error opening file!" << std::endl;
    //         return ; // Indicate an error
    }
    GitObjectStorage obj;
    std::string content = obj.readObject(path);
    std::istringstream s(content);
    std::string line;
    size_t i=0;
    while(std::getline(s,line)){
        IndexEntry entry;
        entry.mode = line.substr(0,6);
        entry.path = line.substr(7,line.find(" ",7));
        entry.hash = line.substr(entry.mode.length() + entry.path.length()+1);

        IndexManager::entries.push_back(entry);
        IndexManager::pathToIndex[entry.path] = i;
        i++;

    }


}
