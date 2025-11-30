# Automated Tests

This directory contains the automated test suite for Bezugszeichenprüfvorrichtung.

## Test Coverage

The test suite covers:

- **GermanTextAnalyzer**: Stemming, article detection, multi-word handling
- **EnglishTextAnalyzer**: English stemming (Porter algorithm), article detection
- **RE2RegexHelper**: Regex matching, UTF-8 handling, position mapping
- **Utils**: StemVector operations, hash functions, BZ comparator

## Running Tests Locally

### Prerequisites

- CMake 3.14 or higher
- C++20 compatible compiler
- Git (for submodules)

### Build and Run Tests

```bash
# From project root
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
cd tests
./unit_tests

# Run tests with verbose output
./unit_tests --gtest_filter=* --gtest_color=yes

# Run specific test suite
./unit_tests --gtest_filter=GermanTextAnalyzerTest.*

# Run specific test
./unit_tests --gtest_filter=GermanTextAnalyzerTest.CreateStemVector_SingleWord
```

### Test Output

Tests use Google Test framework and provide:
- ✓ Pass/fail status for each test
- Detailed failure messages with expected vs actual values
- Test execution time
- Summary statistics

### Continuous Integration

The test suite runs automatically on:
- Every push to any branch
- Every pull request
- Both Linux (Ubuntu) and Windows platforms

See `.github/workflows/ci.yml` for CI configuration.

## Writing New Tests

### Test File Structure

```cpp
#include <gtest/gtest.h>
#include "YourClass.h"

class YourClassTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code before each test
  }

  void TearDown() override {
    // Cleanup code after each test
  }

  YourClass instance; // Test fixture
};

TEST_F(YourClassTest, TestMethodName) {
  // Arrange
  auto input = ...;

  // Act
  auto result = instance.method(input);

  // Assert
  EXPECT_EQ(result, expected);
}
```

### Common Assertions

- `EXPECT_EQ(a, b)` - Expect equal
- `EXPECT_NE(a, b)` - Expect not equal
- `EXPECT_TRUE(condition)` - Expect true
- `EXPECT_FALSE(condition)` - Expect false
- `EXPECT_GT(a, b)` - Expect greater than
- `EXPECT_LT(a, b)` - Expect less than
- `ASSERT_*` variants - Same as EXPECT but stop test on failure

### Test Naming Convention

- Test suite name: `ClassNameTest`
- Test case name: `MethodName_Scenario`
- Examples:
  - `CreateStemVector_SingleWord`
  - `IsDefiniteArticle_Der`
  - `FindPrecedingWord_AtStart`

## Code Coverage

To generate code coverage report locally:

```bash
# Configure with coverage flags
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage"

# Build and run tests
cmake --build build
cd build/tests && ./unit_tests && cd ../..

# Generate coverage report
lcov --capture --directory build --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/build/_deps/*' '*/tests/*' \
  --output-file coverage.info
lcov --list coverage.info

# Generate HTML report (requires genhtml)
genhtml coverage.info --output-directory coverage_html
# Open coverage_html/index.html in browser
```

## Troubleshooting

### Tests fail to compile

- Ensure all submodules are initialized: `git submodule update --init --recursive`
- Check CMake version: `cmake --version` (requires 3.14+)
- Verify C++20 support in your compiler

### Tests fail with "locale" errors

- Install German locale: `sudo locale-gen de_DE.UTF-8`
- Or modify tests to use en_US.UTF-8

### wxWidgets-related errors

- The test suite doesn't require wxWidgets (only tests non-GUI components)
- If build fails, ensure wxWidgets submodule is properly initialized

### RE2 not found

- RE2 is fetched automatically by CMake via FetchContent
- Ensure you have internet connection during first build
- Or pre-download RE2 to avoid network dependency

## Adding Tests to CI

New test files are automatically picked up if:
1. Placed in `tests/` directory
2. Named `test_*.cpp`
3. Added to `tests/CMakeLists.txt`

The CI will build and run all tests on both Linux and Windows.
