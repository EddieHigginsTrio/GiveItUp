cmake_minimum_required(VERSION 3.16)
project(TileMapEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(TileMapEditor
    src/main.cpp
    src/Editor.cpp
)

target_include_directories(TileMapEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/../src
)

target_link_libraries(TileMapEditor PRIVATE sfml-graphics)
