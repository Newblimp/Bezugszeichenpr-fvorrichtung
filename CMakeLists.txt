cmake_minimum_required(VERSION 3.10)
project(Bezugszeichenvorrichtung)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set Qt installation path for Windows
if(WIN32)
  # For static builds, point to your static Qt build directory
  # Change this path to where you built static Qt
  set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1-static/msvc2022_64")
endif()

# Force static linking
set(BUILD_SHARED_LIBS OFF)

# Try to find Qt6 first, then fall back to Qt5
find_package(Qt6 COMPONENTS Core Widgets Gui)
if(NOT Qt6_FOUND)
  find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)
  set(QT_VERSION_MAJOR 5)
else()
  set(QT_VERSION_MAJOR 6)
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

