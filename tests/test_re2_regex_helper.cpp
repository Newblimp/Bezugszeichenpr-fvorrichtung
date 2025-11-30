#include <gtest/gtest.h>
#include "RE2RegexHelper.h"
#include <re2/re2.h>

class RE2RegexHelperTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Common regex patterns for testing
  }
};

// Test MatchIterator basic functionality
TEST_F(RE2RegexHelperTest, MatchIterator_SimpleMatch) {
  std::wstring text = L"Lager 10";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\d+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match[1], L"Lager");
  EXPECT_EQ(match[2], L"10");
  EXPECT_FALSE(iter.hasNext());
}

TEST_F(RE2RegexHelperTest, MatchIterator_MultipleMatches) {
  std::wstring text = L"Lager 10 Motor 20 Welle 30";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\d+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  int count = 0;
  while (iter.hasNext()) {
    auto match = iter.next();
    count++;
  }

  EXPECT_EQ(count, 3);
}

TEST_F(RE2RegexHelperTest, MatchIterator_NoMatches) {
  std::wstring text = L"No numbers here";
  re2::RE2 pattern("(\\d+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  EXPECT_FALSE(iter.hasNext());
}

TEST_F(RE2RegexHelperTest, MatchIterator_Position) {
  std::wstring text = L"Lager 10";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\d+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match.position, 0);
  EXPECT_GT(match.length, 0);
}

TEST_F(RE2RegexHelperTest, MatchIterator_GermanUmlauts) {
  std::wstring text = L"Änderung 15";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\d+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match[1], L"Änderung");
  EXPECT_EQ(match[2], L"15");
}

TEST_F(RE2RegexHelperTest, MatchIterator_TwoWordPattern) {
  std::wstring text = L"erstes Lager 10";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\p{L}+)\\s+(\\d+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match[1], L"erstes");
  EXPECT_EQ(match[2], L"Lager");
  EXPECT_EQ(match[3], L"10");
}

TEST_F(RE2RegexHelperTest, MatchIterator_CaptureGroups) {
  std::wstring text = L"Lager 10a";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\d+[a-zA-Z']*)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match[1], L"Lager");
  EXPECT_EQ(match[2], L"10a");
}

// Test UTF-8 conversion and position mapping
TEST_F(RE2RegexHelperTest, UTF8Conversion_BasicASCII) {
  std::wstring text = L"test";
  re2::RE2 pattern("test");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match.position, 0);
  EXPECT_EQ(match.length, 4);
}

TEST_F(RE2RegexHelperTest, UTF8Conversion_UmlautPosition) {
  std::wstring text = L"Ä test";
  re2::RE2 pattern("test");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  // Position should account for wide character
  EXPECT_EQ(match.position, 2); // "Ä " = 2 wchars
}

TEST_F(RE2RegexHelperTest, UTF8Conversion_MultipleUmlauts) {
  std::wstring text = L"äöü Lager 10";
  re2::RE2 pattern("Lager");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match.position, 4); // "äöü " = 4 wchars
}

// Test edge cases
TEST_F(RE2RegexHelperTest, MatchIterator_EmptyString) {
  std::wstring text = L"";
  re2::RE2 pattern("(\\p{L}+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  EXPECT_FALSE(iter.hasNext());
}

TEST_F(RE2RegexHelperTest, MatchIterator_OnlyWhitespace) {
  std::wstring text = L"   \t\n  ";
  re2::RE2 pattern("(\\p{L}+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  EXPECT_FALSE(iter.hasNext());
}

TEST_F(RE2RegexHelperTest, MatchIterator_OverlappingMatches) {
  std::wstring text = L"aaa";
  re2::RE2 pattern("aa");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  int count = 0;
  while (iter.hasNext()) {
    iter.next();
    count++;
  }

  // RE2 doesn't find overlapping matches by default
  EXPECT_EQ(count, 1);
}

// Test case-insensitive matching
TEST_F(RE2RegexHelperTest, MatchIterator_CaseInsensitive) {
  std::wstring text = L"LAGER 10";
  re2::RE2 pattern("(?i)(lager)\\s+(\\d+)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match[1], L"LAGER"); // Returns original case
}

// Test reference number formats
TEST_F(RE2RegexHelperTest, MatchIterator_ReferenceWithLetter) {
  std::wstring text = L"Lager 10a";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\d+[a-zA-Z]*)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match[2], L"10a");
}

TEST_F(RE2RegexHelperTest, MatchIterator_ReferenceWithApostrophe) {
  std::wstring text = L"Lager 10'";
  re2::RE2 pattern("(\\p{L}+)\\s+(\\d+[a-zA-Z']*)");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();

  EXPECT_EQ(match[2], L"10'");
}

// Test consecutive matches
TEST_F(RE2RegexHelperTest, MatchIterator_ConsecutiveWords) {
  std::wstring text = L"Lager Motor Welle";
  re2::RE2 pattern("\\p{L}+");

  RE2RegexHelper::MatchIterator iter(text, pattern);

  int count = 0;
  while (iter.hasNext()) {
    iter.next();
    count++;
  }

  EXPECT_EQ(count, 3);
}
