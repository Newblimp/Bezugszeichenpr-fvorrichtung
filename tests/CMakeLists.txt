# Test suite for Bezugszeichenprüfvorrichtung

# Include directories from main project
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/img)

# Test executable
add_executable(
  unit_tests
  test_german_analyzer.cpp
  test_english_analyzer.cpp
  test_re2_regex_helper.cpp
  test_utils.cpp
  # Source files needed for testing (non-UI components)
  ${CMAKE_SOURCE_DIR}/src/GermanTextAnalyzer.cpp
  ${CMAKE_SOURCE_DIR}/src/EnglishTextAnalyzer.cpp
  ${CMAKE_SOURCE_DIR}/src/RE2RegexHelper.cpp
  ${CMAKE_SOURCE_DIR}/src/utils_core.cpp
  ${CMAKE_SOURCE_DIR}/src/stem_collector.cpp
)

# Link against Google Test and RE2
target_link_libraries(
  unit_tests
  GTest::gtest_main
  GTest::gmock
  re2
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)

# Add coverage flags to test executable if coverage is enabled
if(CMAKE_CXX_FLAGS MATCHES "--coverage")
  target_compile_options(unit_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
  target_link_options(unit_tests PRIVATE --coverage)
endif()
