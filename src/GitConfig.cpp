#include "headers/GitConfig.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

GitConfig::GitConfig(const std::string&) {
    gitDir = ".git";
}

bool GitConfig::setConfig(const std::string& key, const std::string& value) {
  try {
    if (key.empty()) {
      throw ConfigException("Config key cannot be empty");
    }
    if (value.empty()) {
      throw ConfigException("Config value cannot be empty");
    }
    std::string configPath = gitDir + "/config";
    std::ofstream configFile(configPath, std::ios::app);
    if (!configFile.is_open()) {
      throw ConfigException("Failed to open config file: " + configPath);
    }
    configFile << key << " = " << value << std::endl;
    configFile.close();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "setConfig failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitConfig::getConfig(const std::string& key, std::string& value) {
  try {
    if (key.empty()) {
      throw ConfigException("Config key cannot be empty");
    }
    std::string configPath = gitDir + "/config";
    if (!std::filesystem::exists(configPath)) {
      throw ConfigException("Config file does not exist: " + configPath);
    }
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
      throw ConfigException("Failed to open config file: " + configPath);
    }
    std::string line;
    while (std::getline(configFile, line)) {
      size_t pos = line.find('=');
      if (pos != std::string::npos) {
        std::string configKey = line.substr(0, pos);
        std::string configValue = line.substr(pos + 1);
        configKey.erase(0, configKey.find_first_not_of(" \t"));
        configKey.erase(configKey.find_last_not_of(" \t") + 1);
        configValue.erase(0, configValue.find_first_not_of(" \t"));
        configValue.erase(configValue.find_last_not_of(" \t") + 1);
        if (configKey == key) {
          value = configValue;
          return true;
        }
      }
    }
    throw ConfigException("Config key not found: " + key);
  } catch (const std::exception& e) {
    std::cerr << "getConfig failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitConfig::listConfig() {
  try {
    std::string configPath = gitDir + "/config";
    if (!std::filesystem::exists(configPath)) {
      std::cout << "No config file found." << std::endl;
      return true;
    }
    
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
      throw ConfigException("Failed to open config file: " + configPath);
    }
    
    std::cout << "Git configuration:" << std::endl;
    std::string line;
    while (std::getline(configFile, line)) {
      if (!line.empty() && line.find('=') != std::string::npos) {
        std::cout << "  " << line << std::endl;
      }
    }
    
    return true;
  } catch (const std::exception& e) {
    std::cerr << "listConfig failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitConfig::removeConfig(const std::string& key) {
  try {
    if (key.empty()) {
      throw ConfigException("Config key cannot be empty");
    }
    
    std::string configPath = gitDir + "/config";
    if (!std::filesystem::exists(configPath)) {
      throw ConfigException("Config file does not exist: " + configPath);
    }
    
    std::ifstream inputFile(configPath);
    if (!inputFile.is_open()) {
      throw ConfigException("Failed to open config file: " + configPath);
    }
    
    std::vector<std::string> lines;
    std::string line;
    bool found = false;
    
    while (std::getline(inputFile, line)) {
      size_t pos = line.find('=');
      if (pos != std::string::npos) {
        std::string configKey = line.substr(0, pos);
        configKey.erase(0, configKey.find_first_not_of(" \t"));
        configKey.erase(configKey.find_last_not_of(" \t") + 1);
        
        if (configKey == key) {
          found = true;
          continue; // Skip this line
        }
      }
      lines.push_back(line);
    }
    
    if (!found) {
      throw ConfigException("Config key not found: " + key);
    }
    
    std::ofstream outputFile(configPath);
    if (!outputFile.is_open()) {
      throw ConfigException("Failed to write config file: " + configPath);
    }
    
    for (const auto& l : lines) {
      outputFile << l << std::endl;
    }
    
    return true;
  } catch (const std::exception& e) {
    std::cerr << "removeConfig failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitConfig::validateConfigKey(const std::string& key) {
  try {
    if (key.empty()) {
      throw ConfigException("Config key cannot be empty");
    }
    
    // Check for invalid characters
    if (key.find(' ') != std::string::npos) {
      throw ConfigException("Config key cannot contain spaces");
    }
    
    if (key.find('\t') != std::string::npos) {
      throw ConfigException("Config key cannot contain tabs");
    }
    
    if (key.find('=') != std::string::npos) {
      throw ConfigException("Config key cannot contain '='");
    }
    
    if (key.find('\n') != std::string::npos) {
      throw ConfigException("Config key cannot contain newlines");
    }
    
    return true;
  } catch (const std::exception& e) {
    std::cerr << "validateConfigKey failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitConfig::validateConfigValue(const std::string& value) {
  try {
    if (value.empty()) {
      throw ConfigException("Config value cannot be empty");
    }
    
    // Check for invalid characters
    if (value.find('\n') != std::string::npos) {
      throw ConfigException("Config value cannot contain newlines");
    }
    
    return true;
  } catch (const std::exception& e) {
    std::cerr << "validateConfigValue failed: " << e.what() << std::endl;
    return false;
  }
}

bool GitConfig::setUserName(const std::string& name) {
    return setConfig("user.name", name);
}

bool GitConfig::setUserEmail(const std::string& email) {
    return setConfig("user.email", email);
}

std::string GitConfig::getUserName() const {
    std::string value;
    GitConfig mutableThis = *this;
    if (mutableThis.getConfig("user.name", value)) {
        return value;
    }
    return "Your Name";
}

std::string GitConfig::getUserEmail() const {
    std::string value;
    GitConfig mutableThis = *this;
    if (mutableThis.getConfig("user.email", value)) {
        return value;
    }
    return "your@email.com";
}

bool GitConfig::addRemote(const std::string& name, const std::string& path) {
    return setConfig("remote." + name, path);
}

bool GitConfig::removeRemote(const std::string& name) {
    return removeConfig("remote." + name);
}

bool GitConfig::getRemote(const std::string& name, std::string& path) const {
    GitConfig mutableThis = *this; // getConfig is not const, workaround
    return mutableThis.getConfig("remote." + name, path);
}

std::vector<std::pair<std::string, std::string>> GitConfig::listRemotes() const {
    std::vector<std::pair<std::string, std::string>> remotes;
    std::string configPath = gitDir + "/config";
    if (!std::filesystem::exists(configPath)) return remotes;
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) return remotes;
    std::string line;
    while (std::getline(configFile, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string configKey = line.substr(0, pos);
            std::string configValue = line.substr(pos + 1);
            configKey.erase(0, configKey.find_first_not_of(" \t"));
            configKey.erase(configKey.find_last_not_of(" \t") + 1);
            configValue.erase(0, configValue.find_first_not_of(" \t"));
            configValue.erase(configValue.find_last_not_of(" \t") + 1);
            if (configKey.rfind("remote.", 0) == 0) {
                std::string name = configKey.substr(7); // after remote.
                remotes.emplace_back(name, configValue);
            }
        }
    }
    return remotes;
}

std::string GitConfig::findGitDir() {
    namespace fs = std::filesystem;
    fs::path dir = fs::current_path();
    while (true) {
        fs::path gitPath = dir / ".git";
        if (fs::exists(gitPath) && fs::is_directory(gitPath)) {
            return gitPath.string();
        }
        if (dir == dir.root_path()) break;
        dir = dir.parent_path();
    }
    return ".git"; // fallback, may not exist
}
