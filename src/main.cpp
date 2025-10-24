#include "common.hpp"




int main(int argc, char** argv)
{

    Folder fold(std::filesystem::path("test"));
    std::cout<< bytesToHexString(fold.get_hash());
    

    
    return 0;
}