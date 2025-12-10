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
  # Source files needed for testing (non-UI components)
  ${CMAKE_SOURCE_DIR}/src/GermanTextAnalyzer.cpp
  ${CMAKE_SOURCE_DIR}/src/EnglishTextAnalyzer.cpp
  ${CMAKE_SOURCE_DIR}/src/RE2RegexHelper.cpp
  ${CMAKE_SOURCE_DIR}/src/utils_core.cpp
  ${CMAKE_SOURCE_DIR}/src/stem_collector.cpp
  ${CMAKE_SOURCE_DIR}/src/utils.cpp
  ${CMAKE_SOURCE_DIR}/src/TextScanner.cpp
  ${CMAKE_SOURCE_DIR}/src/ErrorDetectorHelper.cpp
  ${CMAKE_SOURCE_DIR}/src/ErrorNavigator.cpp
  ${CMAKE_SOURCE_DIR}/src/MainWindow.cpp
  ${CMAKE_SOURCE_DIR}/src/UIBuilder.cpp
  ${CMAKE_SOURCE_DIR}/src/PDFLoader.cpp
)

# Link against Google Test, RE2, wxWidgets, and muPDF (Windows only)
target_link_libraries(
  unit_tests
  GTest::gtest_main
  GTest::gmock
  re2
  wx::core
  wx::base
  wx::richtext
)

# Link muPDF on Windows
if(WIN32)
  target_link_libraries(unit_tests mupdf)
endif()

# Handle OpenCV and ONNXRuntime for OCR support (tests compile MainWindow.cpp which includes OCREngine.h)
if(ENABLE_OCR)
  target_link_libraries(unit_tests
    opencv_core
    opencv_imgproc
    opencv_dnn
  )
  target_include_directories(unit_tests PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/opencv/modules/core/include
    ${CMAKE_SOURCE_DIR}/opencv/modules/imgproc/include
    ${CMAKE_SOURCE_DIR}/opencv/modules/dnn/include
    ${ONNXRUNTIME_INCLUDE_DIR}
  )
  # Link ONNXRuntime library
  if(WIN32)
    target_link_libraries(unit_tests ${ONNXRUNTIME_LIB_DIR}/onnxruntime.lib)
  else()
    target_link_libraries(unit_tests ${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so)
    set_target_properties(unit_tests PROPERTIES
      BUILD_RPATH "${ONNXRUNTIME_LIB_DIR}"
      INSTALL_RPATH "${ONNXRUNTIME_LIB_DIR}"
    )
  endif()
endif()

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)

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
