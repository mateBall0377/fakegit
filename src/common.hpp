#pragma once
#include <memory>
#include <string>
#include <openssl/md5.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>


class DirectoryObject
{
protected:
    unsigned char hash[16];
    std::string name;
    std::filesystem::path path;
    virtual void calc_hash()=0;
public:
    std::string get_name()const{return name;};
    const unsigned char* get_hash()const{return hash;}
    
    DirectoryObject()=delete;
    DirectoryObject(std::filesystem::path path):path(path),name(path.filename().string()){};

};


class Folder:public DirectoryObject
{
private:
    std::vector<std::unique_ptr<DirectoryObject>> folder_objects;
    void calc_hash()override;
    void add_object(std::unique_ptr<DirectoryObject> new_obj)
    {
        folder_objects.push_back(std::move(new_obj));
        return;
    }
    bool need_to_add_folder(const std::filesystem::path& path);

public:
    Folder()=delete;
    Folder(std::filesystem::path path);

    const std::vector<std::unique_ptr<DirectoryObject>>& get_objects()const{return folder_objects;}
};

class File:public DirectoryObject
{
private:
    void calc_hash()override;

public:
    File()=delete;
    File(std::filesystem::path path);
};




bool isFolder(const DirectoryObject* obj);
bool isFile(const DirectoryObject* obj);
Folder* asFolder(DirectoryObject* obj);
File* asFile(DirectoryObject* obj);

std::string bytesToHexString(const unsigned char bytes[16]);


