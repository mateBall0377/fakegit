#include "common.hpp"

Folder::Folder(std::filesystem::path path) : DirectoryObject(path)
{
    for (const std::filesystem::directory_entry& object : std::filesystem::directory_iterator(path))
    {   
        if (object.path().filename().string() == std::string(BASE_FILE_NAME) ||
            object.path().filename().string() == std::string(LOG_FILE_NAME) ||
            object.path().filename().string() == std::string(COMMIT_FOLDER_NAME))
            continue;

        if (object.is_directory() && need_to_add_folder(object.path()))
        {   
            std::unique_ptr<Folder> folder(new Folder(object.path()));
            folder_objects.push_back(std::move(folder));
        }
        else if (!object.is_directory()) {
            if (object.file_size() == 0) continue;
            std::unique_ptr<File> file(new File(object.path()));
            folder_objects.push_back(std::move(file));
        }
    }
    
    if (folder_objects.empty()) {
        memset(hash, 0, 16);
        return;
    }
    
    calc_hash();
}

bool Folder::need_to_add_folder(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        return false;
    }    
    
    for (const std::filesystem::directory_entry& object : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_directory(object.path())) {
            if (need_to_add_folder(object.path())) return true;
        }
        else {
            if (object.file_size() != 0) return true;
        }
    }
    
    return false;  
}

File::File(std::filesystem::path path) : DirectoryObject(path) {
    calc_hash();
}

void File::calc_hash() {
    MD5_CTX mdContext;
    MD5_Init(&mdContext); 
    
    std::ifstream file(path.string(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("file open error: " + path.string());
    }
    
    long filesize = file.tellg();

    if (filesize > 0) {
        char* buffer = new char[filesize];
        file.seekg(0, std::ios::beg);
        file.read(buffer, filesize);
        MD5_Update(&mdContext, buffer, filesize); 
        delete[] buffer;
    }
    
    file.close();
    MD5_Final(hash, &mdContext);
}

void Folder::calc_hash() {   
    MD5_CTX mdContext;
    MD5_Init(&mdContext); 

    for (const auto& obj : folder_objects) {  
        MD5_Update(&mdContext, obj->get_hash(), 16); 
    }

    MD5_Final(hash, &mdContext);
}

bool isFolder(const DirectoryObject* obj) {
    return dynamic_cast<const Folder*>(obj) != nullptr;
}

bool isFile(const DirectoryObject* obj) {
    return dynamic_cast<const File*>(obj) != nullptr;
}

Folder* asFolder(DirectoryObject* obj) {
    return dynamic_cast<Folder*>(obj);
}

File* asFile(DirectoryObject* obj) {
    return dynamic_cast<File*>(obj);
}

void Folder::get_json(json& data) {
    data[name]["hash"] = bytesToHexString(hash);
    data[name]["folder"] = true;
    data[name]["objects"] = json::object();
    
    for (const auto& obj : folder_objects) {
        if (isFolder(obj.get())) {
            asFolder(obj.get())->get_json(data[name]["objects"]);
        } else {
            asFile(obj.get())->get_json(data[name]["objects"]);
        }
    }
}

void File::get_json(json& data) {
    data[name]["folder"] = false;
    data[name]["hash"] = bytesToHexString(hash);
}

std::string bytesToHexString(const unsigned char bytes[16]) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 16; ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    
    return ss.str();
}

void combine_hash(const unsigned char hash1[16], const std::string& str, unsigned char result[16]) {
    MD5_CTX mdContext;
    MD5_Init(&mdContext);
    MD5_Update(&mdContext, hash1, 16);
    MD5_Update(&mdContext, str.c_str(), str.length());
    MD5_Final(result, &mdContext);
}