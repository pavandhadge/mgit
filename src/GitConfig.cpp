#include "headers/GitConfig.hpp"

GitConfig::GitConfig():name("pavan dhadge"),email("pavan@pd.org.com"){

}
std::string GitConfig::getName(){
    return name;
}
std::string GitConfig::getEmail(){
    return email;
}
