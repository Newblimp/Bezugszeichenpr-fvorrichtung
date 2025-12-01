cmake_minimum_required(VERSION 3.14)
project(Bezugszeichenvorrichtung)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
set(wxBUILD_SHARED OFF)

# Set source file encoding to UTF-8 for proper handling of German umlauts (ä, ö, ü, ß)
if(MSVC)
  # MSVC: /utf-8 sets both source and execution charset to UTF-8
  add_compile_options(/utf-8)
else()
  # GCC/Clang: explicitly set input and execution charsets
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

# Tell Abseil's CMake to generate install and export targets
set(ABSL_ENABLE_INSTALL ON CACHE BOOL "Enable installation and export of Abseil targets." FORCE)
# Fetch abseil library (required by RE2)
include(FetchContent)
FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20240722.0
)
# Disable abseil testing
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(absl)

# Fetch RE2 library for faster regex performance
FetchContent_Declare(
  re2
  GIT_REPOSITORY https://github.com/google/re2.git
  GIT_TAG 2024-07-02
)
# Build RE2 as static library
set(RE2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(re2)

# Fetch Google Test for unit testing
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

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
set(wxUSE_WEBVIEW_WEBKIT OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW_WEBKIT2 OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW_IE OFF CACHE BOOL "" FORCE)
## Note: wxUSE_HTML must stay enabled - wxRichTextCtrl depends on it
## But we can disable the help controllers
set(wxUSE_HELP OFF CACHE BOOL "" FORCE)
set(wxUSE_MS_HTML_HELP OFF CACHE BOOL "" FORCE)
set(wxUSE_WXHTML_HELP OFF CACHE BOOL "" FORCE)

add_subdirectory(libs/wxWidgets)
include_directories(include img)

# Find MuPDF library
find_package(unofficial-mupdf CONFIG)
if(unofficial-mupdf_FOUND)
    set(MUPDF_LIBRARIES unofficial::mupdf unofficial::mupdf-third)
    add_definitions(-DHAVE_MUPDF=1)
    message(STATUS "MuPDF found via vcpkg")
else()
    # Try pkg-config as fallback
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(MUPDF mupdf)
        if(MUPDF_FOUND)
            add_definitions(-DHAVE_MUPDF=1)
            include_directories(${MUPDF_INCLUDE_DIRS})
            link_directories(${MUPDF_LIBRARY_DIRS})
            message(STATUS "MuPDF found via pkg-config")
        endif()
    endif()
endif()

# Find Tesseract and Leptonica libraries
find_package(Tesseract CONFIG)
if(Tesseract_FOUND)
    set(TESSERACT_LIBRARIES libtesseract)
    add_definitions(-DHAVE_TESSERACT=1)
    message(STATUS "Tesseract found via vcpkg")
else()
    # Try pkg-config as fallback
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(TESSERACT tesseract)
        if(TESSERACT_FOUND)
            add_definitions(-DHAVE_TESSERACT=1)
            include_directories(${TESSERACT_INCLUDE_DIRS})
            link_directories(${TESSERACT_LIBRARY_DIRS})
            set(TESSERACT_LIBRARIES ${TESSERACT_LIBRARIES})
            message(STATUS "Tesseract found via pkg-config")
        endif()
    endif()
endif()

find_package(Leptonica CONFIG)
if(Leptonica_FOUND)
    set(LEPTONICA_LIBRARIES leptonica)
    message(STATUS "Leptonica found via vcpkg")
else()
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(LEPTONICA lept)
        if(LEPTONICA_FOUND)
            include_directories(${LEPTONICA_INCLUDE_DIRS})
            link_directories(${LEPTONICA_LIBRARY_DIRS})
            set(LEPTONICA_LIBRARIES ${LEPTONICA_LIBRARIES})
            message(STATUS "Leptonica found via pkg-config")
        endif()
    endif()
endif()

# Check if PDF processing is available
if(MUPDF_LIBRARIES AND TESSERACT_LIBRARIES AND LEPTONICA_LIBRARIES)
    message(STATUS "PDF processing will be ENABLED (MuPDF + Tesseract + Leptonica)")
    add_definitions(-DENABLE_PDF_PROCESSING=1)
else()
    message(WARNING "PDF processing will be DISABLED. Missing libraries:")
    if(NOT MUPDF_LIBRARIES)
        message(WARNING "  - MuPDF not found")
    endif()
    if(NOT TESSERACT_LIBRARIES)
        message(WARNING "  - Tesseract not found")
    endif()
    if(NOT LEPTONICA_LIBRARIES)
        message(WARNING "  - Leptonica not found")
    endif()
    message(WARNING "To enable PDF processing, install via vcpkg:")
    message(WARNING "  vcpkg install mupdf tesseract leptonica")
endif()

# Add executable target
if(WIN32)
  add_definitions(-DwxUSE_UNICODE_WCHAR=1; -DwxUSE_RICHEDIT=1; -DwxUSE_WINRT=1; -DwxUSE_STL=1; -DwxUSE_STD_STRING=1; -DwxUSE_GUI=1)
  if(MSVC)
    add_compile_options(/Zc:strictStrings-)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
  else()
  endif()
  # Windows executable with conditional PDFPanel
  if(ENABLE_PDF_PROCESSING)
    add_executable(Bezugszeichenvorrichtung WIN32 main.cpp src/MainWindow.cpp src/utils.cpp src/utils_core.cpp src/GermanTextAnalyzer.cpp src/EnglishTextAnalyzer.cpp src/ErrorNavigator.cpp src/RE2RegexHelper.cpp img/check_16.xpm img/app_icon.ico src/stem_collector.cpp src/PDFPanel.cpp res.rc)
  else()
    add_executable(Bezugszeichenvorrichtung WIN32 main.cpp src/MainWindow.cpp src/utils.cpp src/utils_core.cpp src/GermanTextAnalyzer.cpp src/EnglishTextAnalyzer.cpp src/ErrorNavigator.cpp src/RE2RegexHelper.cpp img/check_16.xpm img/app_icon.ico src/stem_collector.cpp res.rc)
  endif()
else()
  add_compile_options(-Wno-write-strings)
  add_definitions(-DwxUSE_UNICODE_WCHAR=1; -DwxUSE_STL=1; -DwxUSE_STD_STRING=1)
  # Linux executable with conditional PDFPanel
  if(ENABLE_PDF_PROCESSING)
    add_executable(Bezugszeichenvorrichtung main.cpp src/MainWindow.cpp src/utils.cpp src/utils_core.cpp src/GermanTextAnalyzer.cpp src/EnglishTextAnalyzer.cpp src/ErrorNavigator.cpp src/RE2RegexHelper.cpp img/check_16.xpm src/stem_collector.cpp src/PDFPanel.cpp)
  else()
    add_executable(Bezugszeichenvorrichtung main.cpp src/MainWindow.cpp src/utils.cpp src/utils_core.cpp src/GermanTextAnalyzer.cpp src/EnglishTextAnalyzer.cpp src/ErrorNavigator.cpp src/RE2RegexHelper.cpp img/check_16.xpm src/stem_collector.cpp)
  endif()
endif()

# Link against wxWidgets and RE2 libraries
target_link_libraries(Bezugszeichenvorrichtung wx::core wx::base wx::richtext re2)

# Link PDF processing libraries if available
if(ENABLE_PDF_PROCESSING)
    target_link_libraries(Bezugszeichenvorrichtung
        ${MUPDF_LIBRARIES}
        ${TESSERACT_LIBRARIES}
        ${LEPTONICA_LIBRARIES}
    )
endif()

# Add tests subdirectory
add_subdirectory(tests)
