#include "fakegit.hpp"


int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <command>" << std::endl;
        std::cout << "Commands: init, push <message>, pull <hash>, log" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    std::string path = ".";
    
    FakeGit fakegit(path);
    
    if (command == "init") {
        fakegit.Init();
        std::cout << "Initialized" << std::endl;
    }
    else if (command == "push") {
        if (argc < 3) {
            std::cout << "Need message" << std::endl;
            return 1;
        }
        fakegit.Push(argv[2]);
        std::cout << "Pushed" << std::endl;
    }
    else if (command == "pull") {
        if (argc < 3) {
            std::cout << "Need commit hash" << std::endl;
            return 1;
        }
        fakegit.Pull(argv[2]);
        std::cout << "Pulled" << std::endl;
    }
    else if (command == "log") {
        fakegit.Log();
    }
    else {
        std::cout << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}