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
  test_text_scanner.cpp
  test_error_detector.cpp
  test_ordinal_detector.cpp
  test_coverage_gap.cpp
  test_main_window.cpp
  test_image_viewer.cpp
  # test_ui_display.cpp
  # Source files needed for testing (non-UI components)
  ${CMAKE_SOURCE_DIR}/src/GermanTextAnalyzer.cpp
  ${CMAKE_SOURCE_DIR}/src/EnglishTextAnalyzer.cpp
  ${CMAKE_SOURCE_DIR}/src/RE2RegexHelper.cpp
  ${CMAKE_SOURCE_DIR}/src/utils_core.cpp
  ${CMAKE_SOURCE_DIR}/src/stem_collector.cpp
  ${CMAKE_SOURCE_DIR}/src/utils.cpp
  ${CMAKE_SOURCE_DIR}/src/TextScanner.cpp
  ${CMAKE_SOURCE_DIR}/src/ErrorDetectorHelper.cpp
  ${CMAKE_SOURCE_DIR}/src/TextAnalyzer.cpp
  ${CMAKE_SOURCE_DIR}/src/ErrorNavigator.cpp
  ${CMAKE_SOURCE_DIR}/src/OrdinalDetector.cpp
  ${CMAKE_SOURCE_DIR}/src/MainWindow.cpp
  ${CMAKE_SOURCE_DIR}/src/UIBuilder.cpp
  ${CMAKE_SOURCE_DIR}/src/ImageDocument.cpp
  ${CMAKE_SOURCE_DIR}/src/ImageCanvas.cpp
  ${CMAKE_SOURCE_DIR}/src/ImageViewerWindow.cpp
)

# Link against Google Test, RE2, and wxWidgets
target_link_libraries(
  unit_tests
  GTest::gtest_main
  GTest::gmock
  re2
  wx::core
  wx::base
  wx::richtext
)

# Conditionally add PDF support for tests
if(ENABLE_PDF_SUPPORT)
  target_sources(unit_tests PRIVATE
    test_pdf_document.cpp
    ${CMAKE_SOURCE_DIR}/src/PdfDocument.cpp
  )
  target_link_libraries(unit_tests mupdf mupdf-third)
  target_include_directories(unit_tests PRIVATE ${MUPDF_INCLUDE_DIRS})
endif()

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(unit_tests DISCOVERY_TIMEOUT 30)

# Add coverage flags to test executable if coverage is enabled
if(ENABLE_COVERAGE)
  if(MSVC)
    target_compile_options(unit_tests PRIVATE /d2cgsummary)
  else()
    target_compile_options(unit_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
    target_link_options(unit_tests PRIVATE --coverage)
  endif()
endif()
# Also handle if --coverage is in CMAKE_CXX_FLAGS
if(CMAKE_CXX_FLAGS MATCHES "--coverage")
  if(NOT MSVC)
    target_compile_options(unit_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
    target_link_options(unit_tests PRIVATE --coverage)
  endif()
endif()
