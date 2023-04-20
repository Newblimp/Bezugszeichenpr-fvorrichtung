cmake_minimum_required(VERSION 3.10)
project(zeichen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)


add_subdirectory(libs/wxWidgets)
include_directories(include)

# Add executable target
add_executable(zeichen main.cpp src/MainWindow.cpp src/BZComparator.cpp src/utils.cpp)

# Link against wxWidgets libraries
target_link_libraries(zeichen wx::net wx::core wx::base)
