#include "directorywork.hpp"
#include <filesystem>



Folder get_snapshot_folder(const std::filesystem::path& path)
{
    Folder folder;
    folder.name=path.filename().string();

    for(const std::filesystem::directory_entry& object:std::filesystem::directory_iterator(path)){  
        if(object.is_directory())
        {
            folder.folders.push_back(get_snapshot_folder(object.path()));
        }
        else {
            File file;
            file.name=object.path().filename().string();
            // file.hash=hash_calc
            folder.files.push_back(file);    
        }
    }
    
    return folder;
}