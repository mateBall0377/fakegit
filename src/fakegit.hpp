#pragma once
#include "common.hpp"

class FakeGit
{
private:
    std::filesystem::path path;

    bool is_it_rep();
    void check_rep();
    void init_base_files();
    
    void copy_changed_files(std::filesystem::path src, 
                           std::filesystem::path dst, 
                           const json& previous_state);
    
    void copy_directory_recursive(const std::filesystem::path& src, 
                                 const std::filesystem::path& dst,
                                 const std::filesystem::path& base_src);
    
    void find_changed_files(const std::filesystem::path& current_dir,
                           const std::filesystem::path& base_dir,
                           const json& previous_state,
                           std::vector<std::filesystem::path>& changed_files);
    
    std::string get_file_hash_from_state(const std::filesystem::path& file_path, 
                                        const json& state);
    
    std::string get_file_hash_from_commit(const json& commit_data, 
                                         const std::filesystem::path& file_path);
    
    void restore_from_commit(const std::string& commit_hash);
    void cleanup_expired_commits();
    std::string get_previous_commit_hash();
    
    void update_log_file(const std::string& commit_hash, 
                        const std::string& message, 
                        std::time_t timestamp, 
                        std::time_t expiry_time);
    
    json get_previous_commit_state();

public:
    FakeGit() = delete;
    FakeGit(std::filesystem::path path) : path(path) {};
    
    void Init();
    void Push(const std::string& message, int time_to_live = DEFAULT_TIME_TO_LIVE);
    void Pull(const std::string& commit_hash);
    void Log();
};