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

# Optional PDF analysis feature (requires Tesseract OCR and MuPDF)
option(ENABLE_PDF_ANALYSIS "Enable PDF patent figure analysis with OCR" OFF)

if(ENABLE_PDF_ANALYSIS)
  if(WIN32)
    # Windows: Manual library configuration
    # Users should set these variables or install vcpkg packages
    find_path(TESSERACT_INCLUDE_DIR tesseract/baseapi.h)
    find_library(TESSERACT_LIB tesseract)
    find_path(LEPTONICA_INCLUDE_DIR leptonica/allheaders.h)
    find_library(LEPTONICA_LIB lept)
    find_path(MUPDF_INCLUDE_DIR mupdf/fitz.h)
    find_library(MUPDF_LIB mupdf)
    find_library(MUPDF_THIRD_LIB mupdf-third)

    if(TESSERACT_INCLUDE_DIR AND TESSERACT_LIB AND LEPTONICA_INCLUDE_DIR AND
       LEPTONICA_LIB AND MUPDF_INCLUDE_DIR AND MUPDF_LIB)
      set(PDF_ANALYSIS_AVAILABLE TRUE)
      set(TESSERACT_INCLUDE_DIRS ${TESSERACT_INCLUDE_DIR})
      set(LEPTONICA_INCLUDE_DIRS ${LEPTONICA_INCLUDE_DIR})
      set(TESSERACT_LIBRARIES ${TESSERACT_LIB})
      set(LEPTONICA_LIBRARIES ${LEPTONICA_LIB})
      message(STATUS "PDF Analysis: ENABLED (Windows)")
    else()
      set(PDF_ANALYSIS_AVAILABLE FALSE)
      message(WARNING "PDF Analysis: Dependencies not found. Feature will be disabled.")
      message(STATUS "  Install via vcpkg: vcpkg install tesseract leptonica mupdf")
    endif()
  else()
    # Linux/Unix: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TESSERACT tesseract)
    pkg_check_modules(LEPTONICA lept)
    find_library(MUPDF_LIB mupdf)
    find_library(MUPDF_THIRD_LIB mupdf-third)
    find_path(MUPDF_INCLUDE_DIR mupdf/fitz.h)

    if(TESSERACT_FOUND AND LEPTONICA_FOUND AND MUPDF_LIB AND MUPDF_INCLUDE_DIR)
      set(PDF_ANALYSIS_AVAILABLE TRUE)
      message(STATUS "PDF Analysis: ENABLED (Unix)")
    else()
      set(PDF_ANALYSIS_AVAILABLE FALSE)
      message(WARNING "PDF Analysis: Dependencies not found. Feature will be disabled.")
      message(STATUS "  Install: sudo apt-get install tesseract-ocr libtesseract-dev libleptonica-dev libmupdf-dev")
    endif()
  endif()
else()
  set(PDF_ANALYSIS_AVAILABLE FALSE)
  message(STATUS "PDF Analysis: DISABLED (use -DENABLE_PDF_ANALYSIS=ON to enable)")
endif()

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

# Base include directories
include_directories(include img)

# Add PDF analysis includes if available
if(PDF_ANALYSIS_AVAILABLE)
  include_directories(${TESSERACT_INCLUDE_DIRS} ${LEPTONICA_INCLUDE_DIRS} ${MUPDF_INCLUDE_DIR})
  add_definitions(-DHAVE_PDF_ANALYSIS=1)
endif()

# Common source files
set(COMMON_SOURCES
  main.cpp
  src/MainWindow.cpp
  src/utils.cpp
  src/utils_core.cpp
  src/GermanTextAnalyzer.cpp
  src/EnglishTextAnalyzer.cpp
  src/ErrorNavigator.cpp
  src/RE2RegexHelper.cpp
  src/stem_collector.cpp
  img/check_16.xpm
)

# Add PDF analysis source if available
if(PDF_ANALYSIS_AVAILABLE)
  list(APPEND COMMON_SOURCES src/PDFPanel.cpp)
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
  add_executable(Bezugszeichenvorrichtung WIN32 ${COMMON_SOURCES} img/app_icon.ico res.rc)
else()
  add_compile_options(-Wno-write-strings)
  add_definitions(-DwxUSE_UNICODE_WCHAR=1; -DwxUSE_STL=1; -DwxUSE_STD_STRING=1)
  add_executable(Bezugszeichenvorrichtung ${COMMON_SOURCES})
endif()
# Link against wxWidgets and RE2 (always required)
target_link_libraries(Bezugszeichenvorrichtung
    wx::core wx::base wx::richtext re2
)

# Link PDF analysis libraries if available
if(PDF_ANALYSIS_AVAILABLE)
  target_link_libraries(Bezugszeichenvorrichtung
    ${TESSERACT_LIBRARIES}
    ${LEPTONICA_LIBRARIES}
    ${MUPDF_LIB}
  )
  if(MUPDF_THIRD_LIB)
    target_link_libraries(Bezugszeichenvorrichtung ${MUPDF_THIRD_LIB})
  endif()
endif()

# Add tests subdirectory
add_subdirectory(tests)
