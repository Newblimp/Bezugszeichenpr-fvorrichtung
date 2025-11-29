cmake_minimum_required(VERSION 3.10)
project(Bezugszeichenvorrichtung)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
set(wxBUILD_SHARED OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

# Strip symbols in release builds for smaller binary size
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
    endif()
endif()

# Enable dead code elimination (GCC/Clang only)
if(NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
endif()

# Disable unused wxWidgets features to reduce binary size
# Only enable what we actually use: core, base, richtext (which requires HTML)
set(wxUSE_GLCANVAS OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW OFF CACHE BOOL "" FORCE)
set(wxUSE_MEDIACTRL OFF CACHE BOOL "" FORCE)
set(wxUSE_RIBBON OFF CACHE BOOL "" FORCE)
set(wxUSE_AUI OFF CACHE BOOL "" FORCE)
set(wxUSE_PROPGRID OFF CACHE BOOL "" FORCE)
set(wxUSE_XRC OFF CACHE BOOL "" FORCE)
set(wxUSE_STC OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW_WEBKIT OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW_WEBKIT2 OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW_IE OFF CACHE BOOL "" FORCE)
set(wxUSE_XML OFF CACHE BOOL "" FORCE)
# Note: wxUSE_HTML must stay enabled - wxRichTextCtrl depends on it
# But we can disable the help controllers
set(wxUSE_HELP OFF CACHE BOOL "" FORCE)
set(wxUSE_MS_HTML_HELP OFF CACHE BOOL "" FORCE)
set(wxUSE_WXHTML_HELP OFF CACHE BOOL "" FORCE)

add_subdirectory(libs/wxWidgets)
include_directories(include img)

# Add executable target
if(WIN32)
  add_definitions(-DwxUSE_UNICODE_WCHAR=1; -DwxUSE_RICHEDIT=1; -DwxUSE_WINRT=1; -DwxUSE_STL=1; -DwxUSE_STD_STRING=1; -DwxUSE_GUI=1)
  if(MSVC)
	add_compile_options(/Zc:strictStrings-)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
  else()
  endif()
  add_executable(Bezugszeichenvorrichtung WIN32 main.cpp src/MainWindow.cpp src/utils.cpp src/GermanTextAnalyzer.cpp src/ErrorNavigator.cpp img/check_16.xpm img/app_icon.ico src/stem_collector.cpp res.rc) #libs/wxWidgets/include/wx/msw/wx.rc)
else()
  add_compile_options(-Wno-write-strings)
  add_definitions(-DwxUSE_UNICODE_WCHAR=1; -DwxUSE_STL=1; -DwxUSE_STD_STRING=1)
  add_executable(Bezugszeichenvorrichtung main.cpp src/MainWindow.cpp src/utils.cpp src/GermanTextAnalyzer.cpp src/ErrorNavigator.cpp img/check_16.xpm src/stem_collector.cpp)
endif()
# Link against wxWidgets libraries
target_link_libraries(Bezugszeichenvorrichtung wx::core wx::base wx::richtext)

