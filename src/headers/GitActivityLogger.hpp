#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <fstream>
#include <sstream>
#include <sqlite3.h>

struct ActivityRecord {
    int id;
    std::string timestamp;
    std::string command;
    std::string arguments;
    std::string result;
    int exit_code;
    std::string error_message;
    std::string working_directory;
    std::string git_dir;
    std::string current_branch;
    std::string user_agent;
    double execution_time_ms;
    std::string object_hashes_created;
    std::string files_modified;
    std::string branches_affected;
    std::string merge_conflicts;
    std::string performance_metrics;
    std::string context_data;
};

struct PerformanceMetrics {
    double memory_usage_mb;
    double cpu_usage_percent;
    int files_processed;
    int objects_created;
    int database_operations;
    double network_latency_ms;
};

struct RepositoryState {
    std::string head_commit;
    std::string current_branch;
    int total_commits;
    int total_branches;
    int total_files;
    std::string last_modified;
    std::string repository_size;
    std::string index_status;
    std::string merge_state;
};

struct LogSummary {
    int total_commands;
    int successful_commands;
    int failed_commands;
    double average_execution_time;
    std::string most_used_command;
    int most_used_count;
    std::string longest_running_command;
    double longest_execution_time;
    std::string last_command;
    std::string last_command_time;
    std::vector<std::string> recent_errors;
    std::map<std::string, int> command_frequency;
    std::map<std::string, double> command_avg_times;
};

class GitActivityLogger {
private:
    std::string log_dir;
    std::string activity_log_path;
    std::string performance_log_path;
    std::string error_log_path;
    std::chrono::high_resolution_clock::time_point command_start_time;
    sqlite3* db = nullptr;
    
    bool initializeLogDirectory();
    std::string getCurrentTimestamp();
    std::string serializeMap(const std::map<std::string, std::string>& data);
    std::map<std::string, std::string> deserializeMap(const std::string& data);
    void writeToLog(const std::string& log_path, const std::string& entry);
    bool openDatabase();
    void closeDatabase();
    bool initializeDatabase();
    
public:
    GitActivityLogger(const std::string& git_dir = ".git");
    ~GitActivityLogger();
    
    // Core logging functions
    void startCommand(const std::string& command, const std::vector<std::string>& args);
    void endCommand(const std::string& result, int exit_code, const std::string& error_msg = "");
    
    // Repository state tracking
    void logRepositoryState();
    void logObjectCreation(const std::string& object_type, const std::string& hash, const std::string& path = "");
    void logFileModification(const std::string& file_path, const std::string& operation);
    void logBranchOperation(const std::string& branch_name, const std::string& operation);
    void logMergeOperation(const std::string& target_branch, const std::string& status, const std::vector<std::string>& conflicts = {});
    
    // Performance tracking
    void logPerformanceMetrics(const PerformanceMetrics& metrics);
    void logError(const std::string& error_type, const std::string& error_message, const std::string& stack_trace = "");
    void logUserAction(const std::string& action_type, const std::map<std::string, std::string>& context);
    
    // Query functions for AI analysis
    std::vector<ActivityRecord> getRecentActivity(int limit = 100);
    std::vector<ActivityRecord> getActivityByCommand(const std::string& command, int limit = 50);
    std::vector<ActivityRecord> getErrors(int limit = 50);
    std::vector<ActivityRecord> getSlowCommands(double threshold_ms = 1000.0);
    RepositoryState getCurrentRepositoryState();
    std::map<std::string, int> getCommandUsageStats();
    std::vector<std::string> getCommonErrorPatterns();
    std::map<std::string, double> getAverageExecutionTimes();
    
    // Enhanced summary and analysis functions
    LogSummary generateLogSummary();
    std::string generateDetailedSummary();
    std::string generatePerformanceReport();
    std::string generateErrorAnalysis();
    std::string generateUsagePatterns();
    std::string generateRecommendations();
    std::string generateTimelineReport(int days = 7);
    std::string generateCommandAnalysis(const std::string& command);
    std::string generateErrorReport();
    std::string generateSlowCommandsReport(double threshold_ms = 1000.0);
    std::string generateWorkflowAnalysis();
    std::string generateRepositoryHealthReport();
    std::string generateActivitySummary();
    
    // Utility functions
    bool exportActivityLog(const std::string& file_path);
    bool clearOldLogs(int days_to_keep = 30);
    bool backupDatabase(const std::string& backup_path);
    std::string getDatabaseStats();
    bool exportToCSV(const std::string& file_path);
    std::string getLogFileContents(const std::string& log_type);
}; 