#include "headers/GitBranch.hpp"
#include "headers/GitHead.hpp"
#include "headers/GitIndex.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
// #include <filesystem>

Branch::Branch(){

}

bool Branch::createBranch(const std::string& branchName){
    gitHead head ;

    std::string path = headsDir+branchName;
    if(std::filesystem::exists(path)){
        std::cerr<<"branch already exist "<<std::endl;
        return false ;
    }
    std::ofstream branchFile(path);
    branchFile << head.getBranchHeadHash() ;
    branchFile.close();
    return true ;
}

bool Branch::checkout(const std::string& branchName){
    gitHead head ;
    try{

    head.writeHeadToHeadOfNewBranch(branchName);
    return true;
    }catch(const std::exception &e){
        std::cerr<<"error while updating head file "<<e.what()<<std::endl;
        return false;
    }
}
std::string Branch::getCurrentBranch()const {
    gitHead head ;
    return head.getBranch() ;
}

 bool Branch::deleteBranch(const std::string &branchName){
     std::string path = headsDir+branchName;
     if(!std::filesystem::exists(path)){
         std::cerr<<"no such branch exist"<<std::endl;
         return false;
     }
     if(getCurrentBranch() == branchName){
         IndexManager idx;
         std::vector<std::pair<std::string, std::string>> changelist = idx.computeStatus();
         if(changelist.size() !=0){
             std::cerr<<"uncommited changes . commit changes to delete this branch. merge required."<<std::endl;
             return false;
         }
         //here add logic to check if merged or not and then delte
     }
     std::filesystem::remove(path);
     return true;
}
std::vector<std::string> Branch::listBranches() const{
    std::vector<std::string> branchList;
    for(const auto& entity : std::filesystem::directory_iterator(headsDir) ){
        if(!entity.is_regular_file()){
            continue;
        }
        branchList.push_back(entity.path().filename());
    }
    return branchList;
}

bool Branch::renameBranch(const std::string& oldName, const std::string& newName){
    std::string path = headsDir + oldName;
     std::string newPath = headsDir + newName;
   if(!std::filesystem::exists(path)){
       std::cerr<<"no such branch exist"<<std::endl;
       return false;
   }
   std::string currentBranch = getCurrentBranch();
   if(currentBranch == oldName){
       gitHead head ;
       head.writeHeadToHeadOfNewBranch(newName);
   }
   std::filesystem::rename(path , newPath);
   return true;
}
std::string Branch::getCurrentBranchHash() const{
    gitHead head ;
    return head.getBranchHeadHash() ;
    // return hash ;
}

std::string Branch::getBranchHash(const std::string& branchName) const {
    std::string path = headsDir + branchName;
    if (!std::filesystem::exists(path)) return "";

    std::ifstream in(path);
    std::string hash;
    getline(in, hash);
    return hash;
}


bool Branch::updateBranchHead(const std::string& branchName, const std::string& newHash){
    std::string path = headsDir+branchName;
    if(!std::filesystem::exists(path)){
        std::cerr<<"branch does not exist"<<std::endl;
        std::cout<<"to make new branch use git branch or git checkout command"<<std::endl;
        return false ;
    }

    std::ofstream branchFile(path);
    branchFile << newHash;
    branchFile.close();
    return true;
}
