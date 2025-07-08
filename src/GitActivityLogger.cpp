#include "headers/GitActivityLogger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <sqlite3.h>

GitActivityLogger::GitActivityLogger(const std::string& git_dir) {
    log_dir = git_dir + "/../.mgit";
    activity_log_path = log_dir + "/activity.log";
    performance_log_path = log_dir + "/performance.log";
    error_log_path = log_dir + "/errors.log";
    
    if (!initializeLogDirectory()) {
        std::cerr << "Failed to initialize activity log directory: " << log_dir << std::endl;
    }
    if (!openDatabase()) {
        std::cerr << "Failed to open SQLite activity database." << std::endl;
    } else {
        if (!initializeDatabase()) {
            std::cerr << "Failed to initialize activity_log table in SQLite database." << std::endl;
        }
    }
}

GitActivityLogger::~GitActivityLogger() {
    closeDatabase();
}

bool GitActivityLogger::initializeLogDirectory() {
    try {
        if (!std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create log directory: " << e.what() << std::endl;
        return false;
    }
}

bool GitActivityLogger::openDatabase() {
    std::string db_path = log_dir + "/activity.db";
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open SQLite database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
        return false;
    }
    return true;
}

void GitActivityLogger::closeDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool GitActivityLogger::initializeDatabase() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS activity_log ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT,"
        "event_type TEXT,"
        "command TEXT,"
        "arguments TEXT,"
        "result TEXT,"
        "exit_code INTEGER,"
        "error_message TEXT,"
        "working_directory TEXT,"
        "git_dir TEXT,"
        "current_branch TEXT,"
        "user_agent TEXT,"
        "execution_time_ms REAL,"
        "object_hashes_created TEXT,"
        "files_modified TEXT,"
        "branches_affected TEXT,"
        "merge_conflicts TEXT,"
        "performance_metrics TEXT,"
        "context_data TEXT"
        ");";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to create activity_log table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::string GitActivityLogger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string GitActivityLogger::serializeMap(const std::map<std::string, std::string>& data) {
    std::stringstream ss;
    for (const auto& pair : data) {
        if (!ss.str().empty()) ss << "|";
        ss << pair.first << ":" << pair.second;
    }
    return ss.str();
}

std::map<std::string, std::string> GitActivityLogger::deserializeMap(const std::string& data) {
    std::map<std::string, std::string> result;
    std::stringstream ss(data);
    std::string item;
    while (std::getline(ss, item, '|')) {
        size_t pos = item.find(':');
        if (pos != std::string::npos) {
            result[item.substr(0, pos)] = item.substr(pos + 1);
        }
    }
    return result;
}

void GitActivityLogger::writeToLog(const std::string& log_path, const std::string& entry) {
    // Only migrate activity_log_path to SQLite for now
    if (db && log_path == activity_log_path) {
        // Parse entry: timestamp|EVENT|...|...
        std::vector<std::string> tokens;
        std::stringstream ss(entry);
        std::string token;
        while (std::getline(ss, token, '|')) {
            tokens.push_back(token);
        }
        if (tokens.size() >= 3) {
            std::string timestamp = tokens[0];
            std::string event_type = tokens[1];
            std::string command = tokens[2];
            std::string arguments = tokens.size() > 3 ? tokens[3] : "";
            std::string result = tokens.size() > 4 ? tokens[4] : "";
            int exit_code = tokens.size() > 5 ? std::stoi(tokens[5]) : 0;
            std::string error_message = tokens.size() > 6 ? tokens[6] : "";
            std::string working_directory = tokens.size() > 7 ? tokens[7] : "";
            std::string git_dir = tokens.size() > 8 ? tokens[8] : "";
            std::string current_branch = tokens.size() > 9 ? tokens[9] : "";
            std::string user_agent = tokens.size() > 10 ? tokens[10] : "";
            double execution_time_ms = tokens.size() > 11 ? std::stod(tokens[11]) : 0.0;
            std::string object_hashes_created = tokens.size() > 12 ? tokens[12] : "";
            std::string files_modified = tokens.size() > 13 ? tokens[13] : "";
            std::string branches_affected = tokens.size() > 14 ? tokens[14] : "";
            std::string merge_conflicts = tokens.size() > 15 ? tokens[15] : "";
            std::string performance_metrics = tokens.size() > 16 ? tokens[16] : "";
            std::string context_data = tokens.size() > 17 ? tokens[17] : "";
            const char* sql =
                "INSERT INTO activity_log (timestamp, event_type, command, arguments, result, exit_code, error_message, working_directory, git_dir, current_branch, user_agent, execution_time_ms, object_hashes_created, files_modified, branches_affected, merge_conflicts, performance_metrics, context_data) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, event_type.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, command.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 4, arguments.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 5, result.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 6, exit_code);
                sqlite3_bind_text(stmt, 7, error_message.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 8, working_directory.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 9, git_dir.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 10, current_branch.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 11, user_agent.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(stmt, 12, execution_time_ms);
                sqlite3_bind_text(stmt, 13, object_hashes_created.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 14, files_modified.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 15, branches_affected.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 16, merge_conflicts.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 17, performance_metrics.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 18, context_data.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
        return;
    }
    // Fallback: for other logs or if db is not available, use file
    try {
        std::ofstream log_file(log_path, std::ios::app);
        if (log_file.is_open()) {
            log_file << entry << std::endl;
            log_file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to write to log: " << e.what() << std::endl;
    }
}

void GitActivityLogger::startCommand(const std::string& command, const std::vector<std::string>& args) {
    command_start_time = std::chrono::high_resolution_clock::now();
    
    std::string args_str;
    for (const auto& arg : args) {
        if (!args_str.empty()) args_str += " ";
        args_str += arg;
    }
    
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|START|" << command << "|" << args_str << "|" 
          << std::filesystem::current_path().string();
    
    writeToLog(activity_log_path, entry.str());
}

void GitActivityLogger::endCommand(const std::string& result, int exit_code, const std::string& error_msg) {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - command_start_time);
    double execution_time_ms = duration.count() / 1000.0;
    
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|END|" << result << "|" << exit_code << "|" 
          << error_msg << "|" << std::fixed << std::setprecision(2) << execution_time_ms;
    
    writeToLog(activity_log_path, entry.str());
}

void GitActivityLogger::logRepositoryState() {
    // For now, just log that we're tracking repository state
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|REPO_STATE|" << "tracking";
    writeToLog(activity_log_path, entry.str());
}

void GitActivityLogger::logObjectCreation(const std::string& object_type, const std::string& hash, const std::string& path) {
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|OBJECT_CREATED|" << object_type << "|" << hash << "|" << path;
    writeToLog(activity_log_path, entry.str());
}

void GitActivityLogger::logFileModification(const std::string& file_path, const std::string& operation) {
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|FILE_MODIFIED|" << operation << "|" << file_path;
    writeToLog(activity_log_path, entry.str());
}

void GitActivityLogger::logBranchOperation(const std::string& branch_name, const std::string& operation) {
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|BRANCH_OP|" << operation << "|" << branch_name;
    writeToLog(activity_log_path, entry.str());
}

void GitActivityLogger::logMergeOperation(const std::string& target_branch, const std::string& status, const std::vector<std::string>& conflicts) {
    std::string conflicts_str;
    for (size_t i = 0; i < conflicts.size(); ++i) {
        if (i > 0) conflicts_str += ",";
        conflicts_str += conflicts[i];
    }
    
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|MERGE_OP|" << target_branch << "|" << status << "|" << conflicts_str;
    writeToLog(activity_log_path, entry.str());
}

void GitActivityLogger::logPerformanceMetrics(const PerformanceMetrics& metrics) {
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|PERF|" << metrics.memory_usage_mb << "|" 
          << metrics.cpu_usage_percent << "|" << metrics.files_processed << "|" 
          << metrics.objects_created << "|" << metrics.database_operations << "|" 
          << metrics.network_latency_ms;
    writeToLog(performance_log_path, entry.str());
}

void GitActivityLogger::logError(const std::string& error_type, const std::string& error_message, const std::string& stack_trace) {
    std::stringstream entry;
    entry << getCurrentTimestamp() << "|ERROR|" << error_type << "|" << error_message << "|" << stack_trace;
    writeToLog(error_log_path, entry.str());
}

std::vector<ActivityRecord> GitActivityLogger::getRecentActivity(int limit) {
    std::vector<ActivityRecord> records;
    if (db) {
        std::string sql = "SELECT id, timestamp, command, arguments, result, exit_code, error_message, working_directory, git_dir, current_branch, user_agent, execution_time_ms, object_hashes_created, files_modified, branches_affected, merge_conflicts, performance_metrics, context_data FROM activity_log ORDER BY id DESC LIMIT ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, limit);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                ActivityRecord record;
                record.id = sqlite3_column_int(stmt, 0);
                record.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                record.command = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                record.arguments = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                record.result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                record.exit_code = sqlite3_column_int(stmt, 5);
                record.error_message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
                record.working_directory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
                record.git_dir = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
                record.current_branch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
                record.user_agent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
                record.execution_time_ms = sqlite3_column_double(stmt, 11);
                record.object_hashes_created = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
                record.files_modified = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
                record.branches_affected = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
                record.merge_conflicts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
                record.performance_metrics = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
                record.context_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
                records.push_back(record);
            }
            sqlite3_finalize(stmt);
        }
        std::reverse(records.begin(), records.end()); // To match file order (oldest first)
        return records;
    }
    // Fallback: file-based log
    try {
        std::ifstream log_file(activity_log_path);
        if (!log_file.is_open()) {
            return records;
        }
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(log_file, line)) {
            lines.push_back(line);
        }
        int start = std::max(0, static_cast<int>(lines.size()) - limit);
        for (size_t i = start; i < lines.size(); ++i) {
            ActivityRecord record;
            std::stringstream ss(lines[i]);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, '|')) {
                tokens.push_back(token);
            }
            if (tokens.size() >= 3) {
                record.timestamp = tokens[0];
                record.command = tokens[2];
                if (tokens.size() > 3) record.arguments = tokens[3];
                if (tokens.size() > 4) record.working_directory = tokens[4];
                if (tokens.size() > 5) record.result = tokens[5];
                if (tokens.size() > 6) record.exit_code = std::stoi(tokens[6]);
                if (tokens.size() > 7) record.error_message = tokens[7];
                if (tokens.size() > 8) record.execution_time_ms = std::stod(tokens[8]);
                records.push_back(record);
            }
        }
    } catch (...) {}
    return records;
}

RepositoryState GitActivityLogger::getCurrentRepositoryState() {
    RepositoryState state;
    // This would be populated by reading actual repository state
    // For now, return empty state
    return state;
}

std::map<std::string, int> GitActivityLogger::getCommandUsageStats() {
    std::map<std::string, int> stats;
    
    try {
        std::ifstream log_file(activity_log_path);
        if (!log_file.is_open()) {
            return stats;
        }
        
        std::string line;
        while (std::getline(log_file, line)) {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            
            while (std::getline(ss, token, '|')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 3 && tokens[1] == "START") {
                stats[tokens[2]]++;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading command stats: " << e.what() << std::endl;
    }
    
    return stats;
}

LogSummary GitActivityLogger::generateLogSummary() {
    LogSummary summary;
    auto activities = getRecentActivity(1000); // Get more data for analysis
    
    summary.total_commands = activities.size();
    summary.successful_commands = 0;
    summary.failed_commands = 0;
    summary.average_execution_time = 0.0;
    summary.longest_execution_time = 0.0;
    
    double total_time = 0.0;
    std::map<std::string, int> command_counts;
    std::map<std::string, double> command_times;
    
    for (const auto& activity : activities) {
        if (activity.exit_code == 0) {
            summary.successful_commands++;
        } else {
            summary.failed_commands++;
            if (summary.recent_errors.size() < 5) {
                summary.recent_errors.push_back(activity.error_message);
            }
        }
        
        command_counts[activity.command]++;
        command_times[activity.command] += activity.execution_time_ms;
        total_time += activity.execution_time_ms;
        
        if (activity.execution_time_ms > summary.longest_execution_time) {
            summary.longest_execution_time = activity.execution_time_ms;
            summary.longest_running_command = activity.command;
        }
        
        // Track last command
        if (activities.empty() || activity.timestamp > summary.last_command_time) {
            summary.last_command = activity.command;
            summary.last_command_time = activity.timestamp;
        }
    }
    
    if (!activities.empty()) {
        summary.average_execution_time = total_time / activities.size();
    }
    
    // Find most used command
    for (const auto& pair : command_counts) {
        if (pair.second > summary.most_used_count) {
            summary.most_used_count = pair.second;
            summary.most_used_command = pair.first;
        }
    }
    
    summary.command_frequency = command_counts;
    
    // Calculate average times per command
    for (const auto& pair : command_counts) {
        summary.command_avg_times[pair.first] = command_times[pair.first] / pair.second;
    }
    
    return summary;
}

std::string GitActivityLogger::generateDetailedSummary() {
    LogSummary summary = generateLogSummary();
    
    std::stringstream ss;
    ss << "=== MGIT ACTIVITY LOG SUMMARY ===\n\n";
    
    // Basic statistics
    ss << "📊 BASIC STATISTICS:\n";
    ss << "  Total Commands: " << summary.total_commands << "\n";
    ss << "  Successful: " << summary.successful_commands << " (" 
       << (summary.total_commands > 0 ? (summary.successful_commands * 100.0 / summary.total_commands) : 0) << "%)\n";
    ss << "  Failed: " << summary.failed_commands << " (" 
       << (summary.total_commands > 0 ? (summary.failed_commands * 100.0 / summary.total_commands) : 0) << "%)\n";
    ss << "  Average Execution Time: " << std::fixed << std::setprecision(2) << summary.average_execution_time << "ms\n\n";
    
    // Command usage
    ss << "🎯 COMMAND USAGE:\n";
    ss << "  Most Used: " << summary.most_used_command << " (" << summary.most_used_count << " times)\n";
    ss << "  Longest Running: " << summary.longest_running_command << " (" 
       << std::fixed << std::setprecision(2) << summary.longest_execution_time << "ms)\n";
    ss << "  Last Command: " << summary.last_command << " at " << summary.last_command_time << "\n\n";
    
    // Top commands
    ss << "📈 TOP COMMANDS:\n";
    std::vector<std::pair<std::string, int>> sorted_commands;
    for (const auto& pair : summary.command_frequency) {
        sorted_commands.push_back(pair);
    }
    std::sort(sorted_commands.begin(), sorted_commands.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(size_t(5), sorted_commands.size()); ++i) {
        ss << "  " << (i+1) << ". " << std::setw(15) << sorted_commands[i].first 
           << ": " << sorted_commands[i].second << " times\n";
    }
    ss << "\n";
    
    // Recent errors
    if (!summary.recent_errors.empty()) {
        ss << "⚠️  RECENT ERRORS:\n";
        for (const auto& error : summary.recent_errors) {
            ss << "  - " << error << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generatePerformanceReport() {
    auto activities = getRecentActivity(1000);
    std::map<std::string, std::vector<double>> command_times;
    
    for (const auto& activity : activities) {
        command_times[activity.command].push_back(activity.execution_time_ms);
    }
    
    std::stringstream ss;
    ss << "=== PERFORMANCE REPORT ===\n\n";
    
    for (const auto& pair : command_times) {
        const auto& times = pair.second;
        if (times.empty()) continue;
        
        double sum = 0.0, min_time = times[0], max_time = times[0];
        for (double time : times) {
            sum += time;
            min_time = std::min(min_time, time);
            max_time = std::max(max_time, time);
        }
        double avg = sum / times.size();
        
        ss << "📊 " << std::setw(15) << pair.first << ":\n";
        ss << "    Count: " << times.size() << " times\n";
        ss << "    Average: " << std::fixed << std::setprecision(2) << avg << "ms\n";
        ss << "    Min: " << std::fixed << std::setprecision(2) << min_time << "ms\n";
        ss << "    Max: " << std::fixed << std::setprecision(2) << max_time << "ms\n";
        ss << "    Total: " << std::fixed << std::setprecision(2) << sum << "ms\n\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateErrorAnalysis() {
    auto activities = getRecentActivity(1000);
    std::map<std::string, int> error_types;
    std::map<std::string, std::vector<std::string>> error_contexts;
    
    for (const auto& activity : activities) {
        if (activity.exit_code != 0 && !activity.error_message.empty()) {
            error_types[activity.error_message]++;
            error_contexts[activity.error_message].push_back(activity.command);
        }
    }
    
    std::stringstream ss;
    ss << "=== ERROR ANALYSIS ===\n\n";
    
    if (error_types.empty()) {
        ss << "✅ No errors found in recent activity!\n";
        return ss.str();
    }
    
    ss << "Found " << error_types.size() << " different error types:\n\n";
    
    for (const auto& pair : error_types) {
        ss << "🚨 " << pair.first << " (" << pair.second << " occurrences)\n";
        ss << "   Commands that caused this error:\n";
        for (const auto& cmd : error_contexts[pair.first]) {
            ss << "     - " << cmd << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateUsagePatterns() {
    auto activities = getRecentActivity(1000);
    std::map<std::string, std::vector<std::string>> command_sequences;
    
    // Analyze command sequences
    for (size_t i = 0; i < activities.size() - 1; ++i) {
        std::string sequence = activities[i].command + " -> " + activities[i + 1].command;
        command_sequences[sequence].push_back(activities[i].timestamp);
    }
    
    std::stringstream ss;
    ss << "=== USAGE PATTERNS ===\n\n";
    
    ss << "🔄 COMMON COMMAND SEQUENCES:\n";
    std::vector<std::pair<std::string, int>> sorted_sequences;
    for (const auto& pair : command_sequences) {
        sorted_sequences.push_back({pair.first, pair.second.size()});
    }
    std::sort(sorted_sequences.begin(), sorted_sequences.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(size_t(10), sorted_sequences.size()); ++i) {
        ss << "  " << (i+1) << ". " << sorted_sequences[i].first 
           << " (" << sorted_sequences[i].second << " times)\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateTimelineReport(int days) {
    auto activities = getRecentActivity(10000); // Get more data for timeline
    std::map<std::string, int> daily_activity;
    std::map<std::string, std::vector<std::string>> daily_commands;
    
    for (const auto& activity : activities) {
        std::string date = activity.timestamp.substr(0, 10); // Extract date part
        daily_activity[date]++;
        daily_commands[date].push_back(activity.command);
    }
    
    std::stringstream ss;
    ss << "=== TIMELINE REPORT (Last " << days << " days) ===\n\n";
    
    std::vector<std::pair<std::string, int>> sorted_days;
    for (const auto& pair : daily_activity) {
        sorted_days.push_back(pair);
    }
    std::sort(sorted_days.begin(), sorted_days.end());
    
    for (const auto& pair : sorted_days) {
        ss << "📅 " << pair.first << ": " << pair.second << " commands\n";
        ss << "   Commands: ";
        for (const auto& cmd : daily_commands[pair.first]) {
            ss << cmd << " ";
        }
        ss << "\n\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateCommandAnalysis(const std::string& command) {
    auto activities = getRecentActivity(1000);
    std::vector<ActivityRecord> command_activities;
    
    for (const auto& activity : activities) {
        if (activity.command == command) {
            command_activities.push_back(activity);
        }
    }
    
    std::stringstream ss;
    ss << "=== COMMAND ANALYSIS: " << command << " ===\n\n";
    
    if (command_activities.empty()) {
        ss << "No activity found for command: " << command << "\n";
        return ss.str();
    }
    
    ss << "📊 STATISTICS:\n";
    ss << "  Total executions: " << command_activities.size() << "\n";
    
    int successful = 0, failed = 0;
    double total_time = 0.0, min_time = command_activities[0].execution_time_ms;
    double max_time = command_activities[0].execution_time_ms;
    
    for (const auto& activity : command_activities) {
        if (activity.exit_code == 0) successful++;
        else failed++;
        
        total_time += activity.execution_time_ms;
        min_time = std::min(min_time, activity.execution_time_ms);
        max_time = std::max(max_time, activity.execution_time_ms);
    }
    
    ss << "  Successful: " << successful << "\n";
    ss << "  Failed: " << failed << "\n";
    ss << "  Success rate: " << (command_activities.size() > 0 ? (successful * 100.0 / command_activities.size()) : 0) << "%\n";
    ss << "  Average time: " << std::fixed << std::setprecision(2) << (total_time / command_activities.size()) << "ms\n";
    ss << "  Min time: " << std::fixed << std::setprecision(2) << min_time << "ms\n";
    ss << "  Max time: " << std::fixed << std::setprecision(2) << max_time << "ms\n\n";
    
    ss << "🕒 RECENT EXECUTIONS:\n";
    for (size_t i = 0; i < std::min(size_t(10), command_activities.size()); ++i) {
        const auto& activity = command_activities[command_activities.size() - 1 - i];
        ss << "  " << activity.timestamp << " - " << activity.execution_time_ms << "ms";
        if (activity.exit_code != 0) ss << " (FAILED)";
        ss << "\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateErrorReport() {
    auto activities = getRecentActivity(1000);
    std::vector<ActivityRecord> errors;
    
    for (const auto& activity : activities) {
        if (activity.exit_code != 0) {
            errors.push_back(activity);
        }
    }
    
    std::stringstream ss;
    ss << "=== ERROR REPORT ===\n\n";
    
    if (errors.empty()) {
        ss << "✅ No errors found in recent activity!\n";
        return ss.str();
    }
    
    ss << "Found " << errors.size() << " errors:\n\n";
    
    for (const auto& error : errors) {
        ss << "❌ " << error.timestamp << " - " << error.command;
        if (!error.arguments.empty()) ss << " " << error.arguments;
        ss << " (exit: " << error.exit_code << ")\n";
        if (!error.error_message.empty()) {
            ss << "   Error: " << error.error_message << "\n";
        }
        ss << "   Time: " << std::fixed << std::setprecision(2) << error.execution_time_ms << "ms\n\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateSlowCommandsReport(double threshold_ms) {
    auto activities = getRecentActivity(1000);
    std::vector<ActivityRecord> slow_commands;
    
    for (const auto& activity : activities) {
        if (activity.execution_time_ms > threshold_ms) {
            slow_commands.push_back(activity);
        }
    }
    
    std::stringstream ss;
    ss << "=== SLOW COMMANDS REPORT (>" << threshold_ms << "ms) ===\n\n";
    
    if (slow_commands.empty()) {
        ss << "✅ No slow commands found!\n";
        return ss.str();
    }
    
    // Sort by execution time (slowest first)
    std::sort(slow_commands.begin(), slow_commands.end(), 
              [](const auto& a, const auto& b) { return a.execution_time_ms > b.execution_time_ms; });
    
    ss << "Found " << slow_commands.size() << " slow commands:\n\n";
    
    for (const auto& cmd : slow_commands) {
        ss << "🐌 " << cmd.timestamp << " - " << cmd.command;
        if (!cmd.arguments.empty()) ss << " " << cmd.arguments;
        ss << " (" << std::fixed << std::setprecision(2) << cmd.execution_time_ms << "ms)\n";
        if (cmd.exit_code != 0) ss << "   Status: FAILED\n";
        ss << "\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateWorkflowAnalysis() {
    auto activities = getRecentActivity(1000);
    std::stringstream ss;
    ss << "=== WORKFLOW ANALYSIS ===\n\n";
    
    if (activities.size() < 2) {
        ss << "Not enough activity for workflow analysis.\n";
        return ss.str();
    }
    
    // Analyze common workflows
    std::map<std::string, int> workflow_patterns;
    for (size_t i = 0; i < activities.size() - 2; ++i) {
        std::string workflow = activities[i].command + " → " + activities[i+1].command + " → " + activities[i+2].command;
        workflow_patterns[workflow]++;
    }
    
    ss << "🔄 COMMON WORKFLOW PATTERNS:\n";
    std::vector<std::pair<std::string, int>> sorted_workflows;
    for (const auto& pair : workflow_patterns) {
        sorted_workflows.push_back(pair);
    }
    std::sort(sorted_workflows.begin(), sorted_workflows.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(size_t(5), sorted_workflows.size()); ++i) {
        ss << "  " << (i+1) << ". " << sorted_workflows[i].first 
           << " (" << sorted_workflows[i].second << " times)\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::generateRepositoryHealthReport() {
    std::stringstream ss;
    ss << "=== REPOSITORY HEALTH REPORT ===\n\n";
    
    // Check if .git directory exists
    if (std::filesystem::exists(".git")) {
        ss << "✅ Git repository found\n";
    } else {
        ss << "❌ No git repository found\n";
        return ss.str();
    }
    
    // Check log files
    if (std::filesystem::exists(activity_log_path)) {
        ss << "✅ Activity logging enabled\n";
        ss << "   Log size: " << std::filesystem::file_size(activity_log_path) << " bytes\n";
    } else {
        ss << "❌ Activity logging not available\n";
    }
    
    if (std::filesystem::exists(error_log_path)) {
        ss << "✅ Error logging enabled\n";
        ss << "   Error log size: " << std::filesystem::file_size(error_log_path) << " bytes\n";
    }
    
    // Get recent activity for health indicators
    auto activities = getRecentActivity(50);
    if (!activities.empty()) {
        ss << "\n📊 RECENT HEALTH INDICATORS:\n";
        int recent_errors = 0;
        double avg_time = 0.0;
        
        for (const auto& activity : activities) {
            if (activity.exit_code != 0) recent_errors++;
            avg_time += activity.execution_time_ms;
        }
        
        avg_time /= activities.size();
        
        ss << "  Recent commands: " << activities.size() << "\n";
        ss << "  Recent errors: " << recent_errors << "\n";
        ss << "  Error rate: " << (activities.size() > 0 ? (recent_errors * 100.0 / activities.size()) : 0) << "%\n";
        ss << "  Average response time: " << std::fixed << std::setprecision(2) << avg_time << "ms\n";
    }
    
    return ss.str();
}

std::string GitActivityLogger::getDatabaseStats() {
    std::stringstream ss;
    ss << "Log File Statistics:\n";
    
    if (std::filesystem::exists(activity_log_path)) {
        ss << "Activity log: " << std::filesystem::file_size(activity_log_path) << " bytes\n";
    }
    if (std::filesystem::exists(performance_log_path)) {
        ss << "Performance log: " << std::filesystem::file_size(performance_log_path) << " bytes\n";
    }
    if (std::filesystem::exists(error_log_path)) {
        ss << "Error log: " << std::filesystem::file_size(error_log_path) << " bytes\n";
    }
    
    ss << "Log directory: " << log_dir << "\n";
    
    return ss.str();
}

std::string GitActivityLogger::generateActivitySummary() {
    return generateDetailedSummary();
}

std::string GitActivityLogger::getLogFileContents(const std::string& log_type) {
    std::string log_path;
    if (log_type == "activity") {
        log_path = activity_log_path;
    } else if (log_type == "performance") {
        log_path = performance_log_path;
    } else if (log_type == "errors") {
        log_path = error_log_path;
    } else {
        return "Unknown log type: " + log_type;
    }
    
    if (!std::filesystem::exists(log_path)) {
        return "Log file not found: " + log_path;
    }
    
    std::ifstream file(log_path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool GitActivityLogger::exportToCSV(const std::string& file_path) {
    try {
        std::ofstream csv_file(file_path);
        if (!csv_file.is_open()) {
            return false;
        }
        
        // Write CSV header
        csv_file << "Timestamp,Command,Arguments,Result,ExitCode,Error,ExecutionTime,WorkingDirectory\n";
        
        auto activities = getRecentActivity(10000);
        for (const auto& activity : activities) {
            csv_file << "\"" << activity.timestamp << "\","
                     << "\"" << activity.command << "\","
                     << "\"" << activity.arguments << "\","
                     << "\"" << activity.result << "\","
                     << activity.exit_code << ","
                     << "\"" << activity.error_message << "\","
                     << activity.execution_time_ms << ","
                     << "\"" << activity.working_directory << "\"\n";
        }
        
        csv_file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to export CSV: " << e.what() << std::endl;
        return false;
    }
} 