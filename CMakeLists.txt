cmake_minimum_required(VERSION 3.10)
project(Bezugszeichenvorrichtung)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Force static linking for Qt
set(BUILD_SHARED_LIBS OFF)
set(QT_BUILD_SHARED_LIBS OFF CACHE BOOL "Build Qt statically" FORCE)

# Point to Qt source directory
if(WIN32)
  set(QT_SOURCE_DIR "C:/Qt/6.10.1/Src")

  # Configure Qt to build statically
  set(QT_FEATURE_static ON CACHE BOOL "" FORCE)
  set(QT_FEATURE_shared OFF CACHE BOOL "" FORCE)
  set(QT_FEATURE_framework OFF CACHE BOOL "" FORCE)

  # Disable unnecessary Qt features to speed up build
  set(QT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(QT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(QT_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

  # Only build the modules we need
  set(QT_BUILD_SUBMODULES "qtbase" CACHE STRING "" FORCE)

  # Add Qt source as subdirectory (will build Qt statically)
  add_subdirectory(${QT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/qt6-build EXCLUDE_FROM_ALL)

  set(QT_VERSION_MAJOR 6)
else()
  # Non-Windows: try to find pre-installed Qt
  find_package(Qt6 COMPONENTS Core Widgets Gui)
  if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)
    set(QT_VERSION_MAJOR 5)
  else()
    set(QT_VERSION_MAJOR 6)
  endif()
endif()

# Source files
set(SOURCES
  main.cpp
  src/MainWindow.cpp
  src/utils.cpp
  src/stem_collector.cpp
)

# Header files (needed for AUTOMOC to work properly)
set(HEADERS
  include/MainWindow.h
  include/utils.h
  include/stem_collector.h
)

# Add executable target
if(WIN32)
  if(MSVC)
	add_compile_options(/Zc:strictStrings- /Zc:__cplusplus)
	# Static linking flags
	add_compile_options(/MT$<$<CONFIG:Debug>:d>)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
  endif()
  add_executable(Bezugszeichenvorrichtung WIN32 ${SOURCES} ${HEADERS})
else()
  add_executable(Bezugszeichenvorrichtung ${SOURCES} ${HEADERS})
endif()

# Define QT_STATIC for static builds
target_compile_definitions(Bezugszeichenvorrichtung PRIVATE QT_STATIC)

# Make sure the target knows about the include directory
target_include_directories(Bezugszeichenvorrichtung PRIVATE include img)

# Link against Qt libraries
if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(Bezugszeichenvorrichtung
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
  )

  # For static Qt builds on Windows, we need to link platform plugins and additional libraries
  if(WIN32)
    # Link the Windows platform plugin statically
    target_link_libraries(Bezugszeichenvorrichtung Qt6::QWindowsIntegrationPlugin)

    # Link additional Windows libraries required for static Qt
    target_link_libraries(Bezugszeichenvorrichtung
      dwmapi
      uxtheme
      imm32
      winmm
      wtsapi32
    )
  endif()
else()
  target_link_libraries(Bezugszeichenvorrichtung Qt5::Core Qt5::Widgets Qt5::Gui)
endif()

