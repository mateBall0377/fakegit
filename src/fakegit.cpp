#include "fakegit.hpp"

using json = nlohmann::json;

void FakeGit::Init()
{   
    if (is_it_rep()) {
        throw std::runtime_error("Repository already initialized");
    }
    init_base_files();
    std::cout << "Repository initialized successfully" << std::endl;
}

void FakeGit::init_base_files() {
    std::filesystem::create_directory(path / COMMIT_FOLDER_NAME);
    
    json base_data;
    base_data["current_commit"] = "";
    base_data["commits"] = json::object();
    base_data["commits_list"] = json::array();
    
    std::ofstream base_file(path / BASE_FILE_NAME);
    base_file << base_data.dump(4);
    base_file.close();

    json log_data;
    log_data["commits"] = json::array();
    
    std::ofstream log_file(path / LOG_FILE_NAME);
    log_file << log_data.dump(4);
    log_file.close();
}

void FakeGit::check_rep() {
    if (!is_it_rep()) {
        throw std::runtime_error("Not a repository");
    }
}

bool FakeGit::is_it_rep() {
    return std::filesystem::exists(path / BASE_FILE_NAME);
}

void FakeGit::Push(const std::string& message, int time_to_live) {
    check_rep();
    cleanup_expired_commits();
    
    std::ifstream file(path / BASE_FILE_NAME);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open base file");
    }
    
    json data = json::parse(file);
    file.close();
    
    Folder new_snapshot(path);
    json previous_state = get_previous_commit_state();
    
    unsigned char commit_hash[16];
    combine_hash(new_snapshot.get_hash(), message, commit_hash);
    std::string current_hash = bytesToHexString(commit_hash);
    
    std::time_t timestamp = std::time(nullptr);
    std::time_t expiry_time = timestamp + time_to_live;
    
    json commit_data;
    new_snapshot.get_json(commit_data);
    commit_data["timestamp"] = timestamp;
    commit_data["expiry_time"] = expiry_time;
    commit_data["message"] = message;
    commit_data["time_to_live"] = time_to_live;
    
    data["current_commit"] = current_hash;
    data["commits"][current_hash] = commit_data;
    data["commits_list"].push_back(current_hash);
    
    std::filesystem::path commit_dir = path / COMMIT_FOLDER_NAME / current_hash;
    std::filesystem::create_directories(commit_dir);
    copy_changed_files(path, commit_dir, previous_state);
    
    std::ofstream out_file(path / BASE_FILE_NAME);
    out_file << data.dump(4);
    out_file.close();
    
    update_log_file(current_hash, message, timestamp, expiry_time);
    
    std::cout << "Commit created: " << current_hash << std::endl;
}

void FakeGit::Pull(const std::string& commit_hash) {
    check_rep();
    cleanup_expired_commits();
    
    std::ifstream file(path / BASE_FILE_NAME);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open base file");
    }
    
    json data = json::parse(file);
    file.close();
    
    if (!data["commits"].contains(commit_hash)) {
        throw std::runtime_error("Commit not found: " + commit_hash);
    }
    
    std::time_t expiry_time = data["commits"][commit_hash]["expiry_time"];
    if (std::time(nullptr) > expiry_time) {
        throw std::runtime_error("Commit has expired: " + commit_hash);
    }
    
    restore_from_commit(commit_hash);
    data["current_commit"] = commit_hash;
    
    std::ofstream out_file(path / BASE_FILE_NAME);
    out_file << data.dump(4);
    out_file.close();
    
    std::cout << "Restored to commit: " << commit_hash << std::endl;
}

void FakeGit::Log() {
    check_rep();
    cleanup_expired_commits();
    
    std::ifstream log_file(path / LOG_FILE_NAME);
    if (!log_file.is_open()) {
        throw std::runtime_error("Cannot open log file");
    }
    
    json log_data = json::parse(log_file);
    log_file.close();
    
    if (log_data["commits"].empty()) {
        std::cout << "No commits found" << std::endl;
        return;
    }
    
    std::cout << "Commits:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& commit : log_data["commits"]) {
        std::time_t timestamp = commit["timestamp"];
        std::time_t expiry_time = commit["expiry_time"];
        
        std::cout << "ID: " << commit["id"] << std::endl;
        std::cout << "Message: " << commit["message"] << std::endl;
        std::cout << "Created: " << std::ctime(&timestamp);
        std::cout << "Expires: " << std::ctime(&expiry_time);
        std::cout << "----------------------------------------" << std::endl;
    }
}

void FakeGit::copy_changed_files(std::filesystem::path src, 
                                std::filesystem::path dst, 
                                const json& previous_state) {
    std::filesystem::create_directories(dst);
    
    if (previous_state.empty()) {
        std::cout << "First commit - saving all files" << std::endl;
        copy_directory_recursive(src, dst, src);
        return;
    }
    
    std::vector<std::filesystem::path> changed_files;
    find_changed_files(src, src, previous_state, changed_files);
    
    std::cout << "Found " << changed_files.size() << " changed files:" << std::endl;
    
    for (const auto& file_path : changed_files) {
        std::filesystem::path src_file = src / file_path;
        std::filesystem::path dst_file = dst / file_path;
        
        if (std::filesystem::exists(src_file) && std::filesystem::is_regular_file(src_file)) {
            std::filesystem::create_directories(dst_file.parent_path());
            std::filesystem::copy_file(src_file, dst_file, 
                                     std::filesystem::copy_options::overwrite_existing);
            std::cout << "  - " << file_path.string() << std::endl;
        }
    }
    
    if (changed_files.empty()) {
        std::cout << "No files changed" << std::endl;
    }
}

void FakeGit::find_changed_files(const std::filesystem::path& current_dir,
                                const std::filesystem::path& base_dir,
                                const json& previous_state,
                                std::vector<std::filesystem::path>& changed_files) {
    for (const auto& entry : std::filesystem::directory_iterator(current_dir)) {
        std::string filename = entry.path().filename().string();
        
        if (filename == BASE_FILE_NAME || 
            filename == LOG_FILE_NAME || 
            filename == COMMIT_FOLDER_NAME) {
            continue;
        }
        
        std::filesystem::path relative_path = std::filesystem::relative(entry.path(), base_dir);
        
        if (entry.is_directory()) {
            find_changed_files(entry.path(), base_dir, previous_state, changed_files);
        } else if (entry.is_regular_file()) {
            File current_file(entry.path());
            std::string current_hash = bytesToHexString(current_file.get_hash());
            
            std::string previous_hash = get_file_hash_from_state(relative_path, previous_state);
            std::cout << "previous " << previous_hash << "    current " << current_hash << std::endl;
            
            if (previous_hash.empty() || previous_hash != current_hash) {
                changed_files.push_back(relative_path);
            }
        }
    }
}

std::string FakeGit::get_file_hash_from_state(const std::filesystem::path& file_path, 
                                             const json& state) {
    if (state.empty()) return "";
    
    std::ifstream file(path / BASE_FILE_NAME);
    if (!file.is_open()) return "";
    
    json data = json::parse(file);
    file.close();
    
    if (!data.contains("commits_list") || !data["commits_list"].is_array()) {
        return "";
    }
    
    std::vector<std::string> commits_list = data["commits_list"];
    
    std::string current_commit_hash = "";
    if (state.contains("hash_from_commit")) {
        current_commit_hash = state["hash_from_commit"];
    } else {
        for (const auto& commit_hash : commits_list) {
            if (data["commits"].contains(commit_hash) && data["commits"][commit_hash] == state) {
                current_commit_hash = commit_hash;
                break;
            }
        }
    }
    
    if (current_commit_hash.empty()) {
        return "";
    }
    
    auto current_it = std::find(commits_list.begin(), commits_list.end(), current_commit_hash);
    if (current_it == commits_list.end()) {
        return "";
    }
    
    for (auto it = current_it; it != commits_list.begin() - 1; --it) {
        std::string commit_hash = *it;
        
        if (data["commits"].contains(commit_hash)) {
            json commit_data = data["commits"][commit_hash]["."];
            std::string result = get_file_hash_from_commit(commit_data, file_path);
            
            if (result != "empty") {
                return result;
            }
        }
        
        if (it == commits_list.begin()) {
            break;
        }
    }
    
    //std::cout << "drun ch fog";
    return "";
}

std::string FakeGit::get_file_hash_from_commit(const json& commit_data, 
                                              const std::filesystem::path& file_path) {
    std::vector<std::string> path_parts;
    for (const auto& part : file_path) {
        path_parts.push_back(part.string());
    }
    
    const json* current = &commit_data;
    
    for (const auto& part : path_parts) {
        if (current->contains("objects") && (*current)["objects"].contains(part)) {
            current = &(*current)["objects"][part];
        } else {
            return "empty";
        }
    }
    
    if (current->contains("hash") && current->contains("folder") && (*current)["folder"] == false) {
        return (*current)["hash"];
    }
    
    return "empty";
}

void FakeGit::copy_directory_recursive(const std::filesystem::path& src, 
                                      const std::filesystem::path& dst,
                                      const std::filesystem::path& base_src) {
    for (const auto& entry : std::filesystem::directory_iterator(src)) {
        std::string filename = entry.path().filename().string();
        
        if (filename == BASE_FILE_NAME || 
            filename == LOG_FILE_NAME || 
            filename == COMMIT_FOLDER_NAME) {
            continue;
        }
        
        std::filesystem::path relative_path = std::filesystem::relative(entry.path(), base_src);
        std::filesystem::path target_path = dst / relative_path;
        
        if (entry.is_directory()) {
            std::filesystem::create_directories(target_path);
            copy_directory_recursive(entry.path(), dst, base_src);
        } else if (entry.is_regular_file()) {
            std::filesystem::create_directories(target_path.parent_path());
            std::filesystem::copy_file(entry.path(), target_path, 
                                     std::filesystem::copy_options::overwrite_existing);
        }
    }
}

void FakeGit::restore_from_commit(const std::string& commit_hash) {
    std::filesystem::path commit_dir = path / COMMIT_FOLDER_NAME / commit_hash;
    
    if (!std::filesystem::exists(commit_dir)) {
        throw std::runtime_error("Commit directory not found: " + commit_hash);
    }
    //pochti ne raboet
    for (const auto& entry : std::filesystem::recursive_directory_iterator(commit_dir)) {
        if (entry.is_regular_file()) {
            std::filesystem::path relative_path = std::filesystem::relative(entry.path(), commit_dir);
            std::filesystem::path target_path = path / relative_path;
            
            std::filesystem::create_directories(target_path.parent_path());
            std::filesystem::copy_file(entry.path(), target_path, 
                                     std::filesystem::copy_options::overwrite_existing);
        }
    }
}

void FakeGit::cleanup_expired_commits() {
    std::ifstream file(path / BASE_FILE_NAME);
    if (!file.is_open()) return;
    
    json data = json::parse(file);
    file.close();
    
    std::time_t current_time = std::time(nullptr);
    std::vector<std::string> expired_commits;
    
    for (auto it = data["commits"].begin(); it != data["commits"].end(); ++it) {
        std::time_t expiry_time = it.value()["expiry_time"];
        if (current_time > expiry_time) {
            expired_commits.push_back(it.key());
        }
    }
    
    for (const auto& commit_hash : expired_commits) {
        std::filesystem::path commit_dir = path / COMMIT_FOLDER_NAME / commit_hash;
        if (std::filesystem::exists(commit_dir)) {
            std::filesystem::remove_all(commit_dir);
        }
        data["commits"].erase(commit_hash);
        
        auto& commits_list = data["commits_list"];
        commits_list.erase(std::remove(commits_list.begin(), commits_list.end(), commit_hash), 
                          commits_list.end());
    }
    
    std::ofstream out_file(path / BASE_FILE_NAME);
    out_file << data.dump(4);
    out_file.close();
}

std::string FakeGit::get_previous_commit_hash() {
    std::ifstream file(path / BASE_FILE_NAME);
    if (!file.is_open()) return "";
    
    json data = json::parse(file);
    file.close();
    
    return data["current_commit"].is_string() ? data["current_commit"] : "";
}

json FakeGit::get_previous_commit_state() {
    std::string previous_commit = get_previous_commit_hash();
    if (previous_commit.empty()) {
        return json();
    }
    
    std::ifstream file(path / BASE_FILE_NAME);
    if (!file.is_open()) return json();
    
    json data = json::parse(file);
    file.close();
    
    if (data["commits"].contains(previous_commit)) {
        return data["commits"][previous_commit];
    }
    
    return json();
}

void FakeGit::update_log_file(const std::string& commit_hash, 
                             const std::string& message, 
                             std::time_t timestamp, 
                             std::time_t expiry_time) {
    std::ifstream log_file(path / LOG_FILE_NAME);
    json log_data;
    
    if (log_file.is_open()) {
        log_data = json::parse(log_file);
        log_file.close();
    }
    
    json new_commit;
    new_commit["id"] = commit_hash;
    new_commit["message"] = message;
    new_commit["timestamp"] = timestamp;
    new_commit["expiry_time"] = expiry_time;
    
    log_data["commits"].push_back(new_commit);
    
    std::ofstream out_log(path / LOG_FILE_NAME);
    out_log << log_data.dump(4);
    out_log.close();
}
//<-431...