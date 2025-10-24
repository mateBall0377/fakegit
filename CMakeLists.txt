
cmake_minimum_required(VERSION 3.16)
project(fakegit)



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-lssl -lcrypto -Wno-deprecated-declarations")

add_subdirectory(libs/json)

file(GLOB files "src/*.cpp" "src/*.hpp")

add_executable(fakegit "src/main.cpp" ${files})


target_link_libraries(fakegit PRIVATE nlohmann_json::nlohmann_json)