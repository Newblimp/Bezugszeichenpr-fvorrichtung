cmake_minimum_required(VERSION 3.10)
project(zeichen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
set(wxBUILD_SHARED OFF)

# find_package(wxWidgets REQUIRED COMPONENTS richtext)
# include(${wxWidgets_USE_FILE})
add_subdirectory(libs/wxWidgets)
include_directories(include img)

# Add executable target
if(WIN32)
  add_executable(zeichen WIN32 main.cpp src/MainWindow.cpp src/BZComparator.cpp src/utils.cpp img/check_16.xpm)
else()
  add_executable(zeichen main.cpp src/MainWindow.cpp src/BZComparator.cpp src/utils.cpp img/check_16.xpm)
endif()
# Link against wxWidgets libraries
target_link_libraries(zeichen wx::net wx::core wx::base wx::richtext)
