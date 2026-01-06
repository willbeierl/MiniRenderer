cmake_minimum_required(VERSION 3.20)
project(MiniRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MiniRenderer
    src/main.cpp
)

# Find packages provided by vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

target_link_libraries(MiniRenderer PRIVATE glfw glad::glad glm::glm)
