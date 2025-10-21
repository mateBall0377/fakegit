#pragma once 
#include <string>
#include <filesystem>
#include <vector>
#include ""


struct File
{
    std::string name;
    std::string hash;
};


struct Folder
{
    std::string name;
    std::vector<Folder> folders;
    std::vector<File> files;
};

Folder get_snapshot_folder(const std::filesystem::path& path);




