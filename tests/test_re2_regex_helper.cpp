#include <gtest/gtest.h>
#include "RE2RegexHelper.h"
#include "RegexPatterns.h"
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

// Test 3-character minimum word matching using actual MainWindow patterns
TEST_F(RE2RegexHelperTest, ThreeCharMinimum_SingleWord) {
  std::wstring text = L"ab 10 abc 20 abcd 30";
  re2::RE2 pattern(RegexPatterns::SINGLE_WORD_PATTERN);

  RE2RegexHelper::MatchIterator iter(text, pattern);

  std::vector<std::wstring> words;
  while (iter.hasNext()) {
    auto match = iter.next();
    words.push_back(match[1]);
  }

  ASSERT_EQ(words.size(), 2);
  EXPECT_EQ(words[0], L"abc");
  EXPECT_EQ(words[1], L"abcd");
}

TEST_F(RE2RegexHelperTest, ThreeCharMinimum_ExactlyThree) {
  std::wstring text = L"rod 10";
  re2::RE2 pattern(RegexPatterns::SINGLE_WORD_PATTERN);

  RE2RegexHelper::MatchIterator iter(text, pattern);

  ASSERT_TRUE(iter.hasNext());
  auto match = iter.next();
  EXPECT_EQ(match[1], L"rod");
}

TEST_F(RE2RegexHelperTest, ThreeCharMinimum_TwoWords) {
  std::wstring text = L"ab cd 10 abc def 20 abcd efgh 30";
  re2::RE2 pattern(RegexPatterns::TWO_WORD_PATTERN);

  RE2RegexHelper::MatchIterator iter(text, pattern);

  std::vector<std::pair<std::wstring, std::wstring>> wordPairs;
  while (iter.hasNext()) {
    auto match = iter.next();
    wordPairs.push_back({match[1], match[2]});
  }

  ASSERT_EQ(wordPairs.size(), 2);
  EXPECT_EQ(wordPairs[0].first, L"abc");
  EXPECT_EQ(wordPairs[0].second, L"def");
  EXPECT_EQ(wordPairs[1].first, L"abcd");
  EXPECT_EQ(wordPairs[1].second, L"efgh");
}

TEST_F(RE2RegexHelperTest, ThreeCharMinimum_GermanArticlesIgnored) {
  std::wstring text = L"der 10 die 20 das 30 Lager 40";
  re2::RE2 pattern(RegexPatterns::SINGLE_WORD_PATTERN);

  RE2RegexHelper::MatchIterator iter(text, pattern);

  std::vector<std::wstring> words;
  while (iter.hasNext()) {
    auto match = iter.next();
    words.push_back(match[1]);
  }

  // Should match all: "der", "die", "das", and "Lager" all have 3+ chars
  // But the isIgnoredWord function will filter out articles separately
  ASSERT_EQ(words.size(), 4);
  EXPECT_EQ(words[0], L"der");
  EXPECT_EQ(words[1], L"die");
  EXPECT_EQ(words[2], L"das");
  EXPECT_EQ(words[3], L"Lager");
}

TEST_F(RE2RegexHelperTest, ThreeCharMinimum_OnlyWords) {
  std::wstring text = L"Lager Motor Welle";
  re2::RE2 pattern(RegexPatterns::WORD_PATTERN);

  RE2RegexHelper::MatchIterator iter(text, pattern);

  int count = 0;
  while (iter.hasNext()) {
    iter.next();
    count++;
  }

  EXPECT_EQ(count, 3);
}

TEST_F(RE2RegexHelperTest, ThreeCharMinimum_NoShortWords) {
  std::wstring text = L"a ab abc abcd";
  re2::RE2 pattern(RegexPatterns::WORD_PATTERN);

  RE2RegexHelper::MatchIterator iter(text, pattern);

  std::vector<std::wstring> words;
  while (iter.hasNext()) {
    auto match = iter.next();
    words.push_back(match[0]);
  }

  ASSERT_EQ(words.size(), 2);
  EXPECT_EQ(words[0], L"abc");
  EXPECT_EQ(words[1], L"abcd");
}
