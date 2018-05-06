cmake_minimum_required(VERSION 3.4)

project(
    blizzbar
    VERSION 1.1.0
    DESCRIPTION "Blizzbar Taskbar Pinning Helper"
    LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(IDE_FOLDER "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(deps)
add_subdirectory(blizzbar-common)
add_subdirectory(blizzbar)
add_subdirectory(blizzbar-hook)