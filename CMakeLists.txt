cmake_minimum_required(VERSION 3.10)
project(Bezugszeichenvorrichtung)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SHARED_LINKER_FLAGS "-s")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")

# Configure FLTK options
set(FLTK_BUILD_TEST OFF CACHE BOOL "Build FLTK test programs")
set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "Build FLTK example programs")
set(OPTION_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

# Add FLTK subdirectory
add_subdirectory(libs/fltk)
include_directories(${CMAKE_BINARY_DIR}/libs/fltk ${CMAKE_SOURCE_DIR}/libs/fltk)
include_directories(include img)

# Add executable target
if(WIN32)
  if(MSVC)
	add_compile_options(/Zc:strictStrings-)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
  else()
  endif()
  add_executable(Bezugszeichenvorrichtung WIN32 main.cpp src/MainWindow.cpp src/utils.cpp src/stem_collector.cpp res.rc)
else()
  add_compile_options(-Wno-write-strings)
  add_executable(Bezugszeichenvorrichtung main.cpp src/MainWindow.cpp src/utils.cpp src/stem_collector.cpp)
endif()

# Link against FLTK libraries
target_link_libraries(Bezugszeichenvorrichtung fltk fltk_images)

