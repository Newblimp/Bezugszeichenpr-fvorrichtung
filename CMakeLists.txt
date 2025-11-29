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
  set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/msvc2022_64")
endif()

# Try to find Qt6 first, then fall back to Qt5
find_package(Qt6 COMPONENTS Core Widgets Gui)
if(NOT Qt6_FOUND)
  find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)
  set(QT_VERSION_MAJOR 5)
else()
  set(QT_VERSION_MAJOR 6)
endif()

include_directories(include img)

# Add executable target
if(WIN32)
  if(MSVC)
	add_compile_options(/Zc:strictStrings- /Zc:__cplusplus)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
  endif()
  add_executable(Bezugszeichenvorrichtung WIN32 main.cpp src/MainWindow.cpp src/utils.cpp src/stem_collector.cpp)
else()
  add_executable(Bezugszeichenvorrichtung main.cpp src/MainWindow.cpp src/utils.cpp src/stem_collector.cpp)
endif()

# Link against Qt libraries
if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(Bezugszeichenvorrichtung Qt6::Core Qt6::Widgets Qt6::Gui)
else()
  target_link_libraries(Bezugszeichenvorrichtung Qt5::Core Qt5::Widgets Qt5::Gui)
endif()

