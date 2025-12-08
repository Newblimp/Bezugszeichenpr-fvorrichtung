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

# MSVC optimization flags
if(MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
# Optimization and linker flags (GCC/Clang only, MSVC uses different syntax)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
endif()

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

# Handle code coverage flags
# Enable code coverage if requested via CMAKE_CXX_FLAGS or ENABLE_COVERAGE option
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
if(ENABLE_COVERAGE)
  message(STATUS "Code coverage enabled")
  if(MSVC)
    # MSVC coverage instrumentation
    add_compile_options(/d2cgsummary)
  else()
    # GCC/Clang coverage instrumentation
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()
endif()
# Also handle if --coverage is already in CXX_FLAGS
if(CMAKE_CXX_FLAGS MATCHES "--coverage")
  message(STATUS "Code coverage enabled via CMAKE_CXX_FLAGS")
  if(NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()
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
include_directories(include img resources)

# Build muPDF from source (static linking, same compiler settings)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/MuPDFConfig.cmake)
setup_mupdf()

# ==============================================================================
# OpenCV for OCR (PaddleOCR support)
# ==============================================================================
option(ENABLE_OCR "Enable PaddleOCR support for handwritten number recognition" ON)
if(ENABLE_OCR)
    message(STATUS "OCR support enabled - configuring OpenCV")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenCVConfig.cmake)
    setup_opencv()

    # Include model embedding script
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedModels.cmake)

    # Define OCR sources
    set(OCR_SOURCES
        src/OCREngine.cpp
        src/ModelLoader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/models_compressed.cpp
    )
endif()

# Add executable target
add_definitions(-DwxUSE_UNICODE_WCHAR=1 -DwxUSE_STL=1 -DwxUSE_STD_STRING=1 -DHAVE_MUPDF=1)
if(ENABLE_OCR)
    add_definitions(-DHAVE_OPENCV=1)
endif()
if(WIN32)
  add_definitions(-DwxUSE_RICHEDIT=1 -DwxUSE_WINRT=1 -DwxUSE_GUI=1)
  if(MSVC)
    add_compile_options(/Zc:strictStrings-)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
  else()
  endif()
  add_executable(Bezugszeichenvorrichtung WIN32 main.cpp src/MainWindow.cpp src/utils.cpp src/utils_core.cpp src/GermanTextAnalyzer.cpp src/EnglishTextAnalyzer.cpp src/ErrorNavigator.cpp src/RE2RegexHelper.cpp src/TextScanner.cpp src/ErrorDetectorHelper.cpp src/UIBuilder.cpp src/PDFLoader.cpp img/check_16.xpm img/app_icon.ico src/stem_collector.cpp ${OCR_SOURCES} res.rc) #libs/wxWidgets/include/wx/msw/wx.rc)
else()
  add_compile_options(-Wno-write-strings)
  add_executable(Bezugszeichenvorrichtung main.cpp src/MainWindow.cpp src/utils.cpp src/utils_core.cpp src/GermanTextAnalyzer.cpp src/EnglishTextAnalyzer.cpp src/ErrorNavigator.cpp src/RE2RegexHelper.cpp src/TextScanner.cpp src/ErrorDetectorHelper.cpp src/UIBuilder.cpp src/PDFLoader.cpp img/check_16.xpm src/stem_collector.cpp ${OCR_SOURCES})
endif()
# Link against wxWidgets and RE2
target_link_libraries(Bezugszeichenvorrichtung
  wx::core wx::base wx::richtext re2
)

# Handle muPDF linking separately to resolve circular dependencies
if(MSVC OR WIN32)
  target_link_libraries(Bezugszeichenvorrichtung mupdf)
else()
  # On Linux, manually link muPDF libraries with linker groups for circular dependencies
  target_link_libraries(Bezugszeichenvorrichtung
    "-Wl,--start-group"
    mupdf_fitz
    mupdf_pdf
    "-Wl,--end-group"
    mupdf_harfbuzz
    mupdf_freetype
    mupdf_lcms2
    mupdf_openjpeg
    mupdf_jbig2dec
    mupdf_zlib
    mupdf_resources
  )
endif()

# Handle OpenCV linking for OCR support
if(ENABLE_OCR)
    target_link_libraries(Bezugszeichenvorrichtung
        opencv_core
        opencv_imgproc
        opencv_dnn
    )
    # Add include directories for OpenCV
    target_include_directories(Bezugszeichenvorrichtung PRIVATE
        ${CMAKE_BINARY_DIR}/opencv
        ${CMAKE_CURRENT_SOURCE_DIR}/opencv/modules/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/opencv/modules/imgproc/include
        ${CMAKE_CURRENT_SOURCE_DIR}/opencv/modules/dnn/include
    )
    # Link against zlib from muPDF for model decompression
    target_link_libraries(Bezugszeichenvorrichtung mupdf_zlib)
    target_include_directories(Bezugszeichenvorrichtung PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/mupdf/thirdparty/zlib
    )
endif()

# Add tests subdirectory
add_subdirectory(tests)
