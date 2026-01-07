cmake_minimum_required(VERSION 3.20)
project(MiniRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MiniRenderer
    src/main.cpp
    src/gfx/ShaderProgram.cpp
    src/gfx/ShaderProgram.h
    src/gfx/Buffer.h
    src/gfx/Buffer.cpp
    src/gfx/VertexArray.h
    src/gfx/VertexArray.cpp
)

# Find packages provided by vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

target_link_libraries(MiniRenderer PRIVATE glfw glad::glad glm::glm)
target_compile_definitions(MiniRenderer PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")


add_custom_command(TARGET MiniRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:MiniRenderer>/assets
)