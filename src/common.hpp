#pragma once
#include <memory>
#include <string>
#include <openssl/md5.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <ctime>

using json = nlohmann::json;

#define BASE_FILE_NAME ".fakegit"
#define COMMIT_FOLDER_NAME "objects"
#define LOG_FILE_NAME "fakegit.log"

#define DEFAULT_TIME_TO_LIVE 86400 

class DirectoryObject
{
protected:
    unsigned char hash[16];
    std::string name;
    std::filesystem::path path;
    virtual void calc_hash() = 0;
    virtual void get_json(json& data) = 0;

public:
    std::string get_name() const { return name; };
    const unsigned char* get_hash() const { return hash; }
    std::filesystem::path get_path() const { return path; }
    
    DirectoryObject() = delete;
    DirectoryObject(std::filesystem::path path) : path(path), name(path.filename().string()) {};
    virtual ~DirectoryObject() = default;
};

class Folder : public DirectoryObject
{
private:
    std::vector<std::unique_ptr<DirectoryObject>> folder_objects;
    void calc_hash() override;
    bool need_to_add_folder(const std::filesystem::path& path);
    
public:
    void get_json(json& data) override;
    Folder() = delete;
    Folder(std::filesystem::path path);
    
    const std::vector<std::unique_ptr<DirectoryObject>>& get_objects() const { return folder_objects; }
};

class File : public DirectoryObject
{
private:
    void calc_hash() override;
    
public:
    void get_json(json& data) override;
    File() = delete;
    File(std::filesystem::path path);
};

bool isFolder(const DirectoryObject* obj);
bool isFile(const DirectoryObject* obj);
Folder* asFolder(DirectoryObject* obj);
File* asFile(DirectoryObject* obj);

std::string bytesToHexString(const unsigned char bytes[16]);
void combine_hash(const unsigned char hash1[16], const std::string& str, unsigned char result[16]);