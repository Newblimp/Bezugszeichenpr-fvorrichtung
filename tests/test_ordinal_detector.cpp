#include <gtest/gtest.h>
#include "OrdinalDetector.h"
#include "RegexPatterns.h"
#include <re2/re2.h>

/**
 * Test suite for OrdinalDetector class
 * Tests automatic detection of multi-word terms with ordinal prefixes
 */

class OrdinalDetectorTest : public ::testing::Test {
protected:
  re2::RE2 twoWordRegex{RegexPatterns::TWO_WORD_PATTERN};

  void SetUp() override {
    // Ensure regexes compiled successfully
    ASSERT_TRUE(twoWordRegex.ok()) << "Two-word regex failed to compile";
  }
};

/**
 * Test: Detects German ordinal patterns (first + second)
 * Expected: "lager" (or its stem) is detected as multi-word base
 */
TEST_F(OrdinalDetectorTest, DetectsGermanFirstSecond) {
  std::wstring text = L"erste Lager 10 zweite Lager 20";

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should detect at least one stem (may be "lager", "lag", etc. depending on stemmer)
  EXPECT_EQ(detected.size(), 1);
  EXPECT_GE(detected.size(), 1);  // At least one stem should be detected
}

/**
 * Test: Detects English ordinal patterns (first + second)
 * Expected: "bearing" is detected as multi-word base
 */
TEST_F(OrdinalDetectorTest, DetectsEnglishFirstSecond) {
  std::wstring text = L"first bearing 10 second bearing 20";

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, false);

  // Should detect "bearing" (stemmed form)
  EXPECT_EQ(detected.size(), 1);
  EXPECT_TRUE(detected.count(L"bear") > 0 || detected.count(L"bearing") > 0);
}

/**
 * Test: Ignores single ordinal
 * Expected: No detection if only "first" appears, no "second"
 */
TEST_F(OrdinalDetectorTest, IgnoresSingleOrdinal) {
  std::wstring text = L"erste Lager 10 dritte Welle 20";  // erste without zweite

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should NOT detect "lager" (only "erste", no "zweite")
  EXPECT_EQ(detected.count(L"lager"), 0);
}

/**
 * Test: Ignores different base stems
 * Expected: No detection when first/second are used with different words
 */
TEST_F(OrdinalDetectorTest, IgnoresDifferentBaseStems) {
  std::wstring text = L"erste Lager 10 zweite Welle 20";  // Different base words

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should NOT detect anything (different base words)
  EXPECT_EQ(detected.size(), 0);
}

/**
 * Test: Handles German declensions
 * Expected: Detects "lager" stem even with different declensions (ersten, zweiten)
 */
TEST_F(OrdinalDetectorTest, HandlesDeclensions) {
  std::wstring text = L"ersten Lager 10 zweiten Lager 20";  // Different declensions

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should detect at least one stem (declensions should be recognized)
  EXPECT_GE(detected.size(), 1);
}

/**
 * Test: Handles case-insensitive matching
 * Expected: Detects ordinals regardless of case
 */
TEST_F(OrdinalDetectorTest, HandlesCaseInsensitive) {
  std::wstring text = L"ERSTE Lager 10 ZWEITE Lager 20";  // Uppercase

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should still detect the stem (case-insensitive)
  EXPECT_GE(detected.size(), 1);
}

/**
 * Test: Multiple different base stems
 * Expected: Detects multiple stems that each have first and second
 */
TEST_F(OrdinalDetectorTest, MultipleBaseStemsDetected) {
  std::wstring text = L"erste Lager 10 zweite Lager 20 erste Welle 30 zweite Welle 40";

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should detect both "lager" and "welle" stems (may be shortened versions)
  EXPECT_GE(detected.size(), 2);
}

/**
 * Test: Empty text
 * Expected: No detection in empty text
 */
TEST_F(OrdinalDetectorTest, EmptyText) {
  std::wstring text = L"";

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  EXPECT_EQ(detected.size(), 0);
}

/**
 * Test: Text without ordinals
 * Expected: No detection if no ordinals in text
 */
TEST_F(OrdinalDetectorTest, NoOrdinalPatterns) {
  std::wstring text = L"Lager 10 Welle 20 Zeige 30";  // No ordinals

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  EXPECT_EQ(detected.size(), 0);
}

/**
 * Test: Only first ordinal (no second)
 * Expected: No detection if only "first" appears
 */
TEST_F(OrdinalDetectorTest, OnlyFirstOrdinal) {
  std::wstring text = L"erste Lager 10 erstes Lager 20";  // Only first (different declensions)

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should NOT detect (only "erste", no "zweite")
  EXPECT_EQ(detected.size(), 0);
}

/**
 * Test: Third ordinal without second
 * Expected: No detection if only third appears (requirement is first AND second)
 */
TEST_F(OrdinalDetectorTest, ThirdOrdinalWithoutSecond) {
  std::wstring text = L"erste Lager 10 dritte Lager 20";  // erste and dritte, missing zweite

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, true);

  // Should NOT detect (missing "zweite")
  EXPECT_EQ(detected.size(), 0);
}

/**
 * Test: English with multiple terms
 * Expected: Detects all terms that have both first and second
 */
TEST_F(OrdinalDetectorTest, EnglishMultipleTerms) {
  std::wstring text = L"first bearing 10 second bearing 20 first gear 30 second gear 40";

  auto detected = OrdinalDetector::detectOrdinalPatterns(text, twoWordRegex, false);

  // Should detect at least "bearing" (and possibly "gear")
  EXPECT_GE(detected.size(), 1);
}
