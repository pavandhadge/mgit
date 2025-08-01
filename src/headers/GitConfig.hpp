#pragma once

#include <string>
#include <vector>
#include <filesystem>

class ConfigException : public std::exception {
public:
    explicit ConfigException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class GitConfig {
public:
    explicit GitConfig(const std::string& gitDir);
    static std::string findGitDir();
    
    // Core config operations
    bool setConfig(const std::string& key, const std::string& value);
    bool getConfig(const std::string& key, std::string& value);
    bool listConfig();
    bool removeConfig(const std::string& key);
    
    // Validation methods
    bool validateConfigKey(const std::string& key);
    bool validateConfigValue(const std::string& value);
    
    // Getters
    std::string getUserName() const;
    std::string getUserEmail() const;
    std::string getRepositoryName() const;
    std::string getRemoteUrl() const;

    // Remote management
    bool addRemote(const std::string& name, const std::string& path);
    bool removeRemote(const std::string& name);
    bool getRemote(const std::string& name, std::string& path) const;
    std::vector<std::pair<std::string, std::string>> listRemotes() const;

    // User config
    bool setUserName(const std::string& name);
    bool setUserEmail(const std::string& email);

    std::string gitDir;

private:
    std::string configFile;
};
