cmake_minimum_required(VERSION 3.10)
project(zeichen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
set(wxBUILD_SHARED OFF)
add_definitions(-DwxUSE_UNICODE_WCHAR=1; -DwxUSE_RICHEDIT=1; -DwxUSE_WINRT=1; -DwxUSE_STL=1; -DwxUSE_STD_STRING=1; -DwxUSE_GUI=1; -DwxUSE_GLCANVAS=1)

add_subdirectory(libs/wxWidgets)
include_directories(include img)

# Add executable target
if(WIN32)
  add_executable(zeichen WIN32 main.cpp src/MainWindow.cpp src/BZComparator.cpp src/utils.cpp img/check_16.xpm)
else()
  add_executable(zeichen main.cpp src/MainWindow.cpp src/BZComparator.cpp src/utils.cpp img/check_16.xpm)
endif()
# Link against wxWidgets libraries
target_link_libraries(zeichen wx::net wx::core wx::base)

