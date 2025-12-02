#include <gtest/gtest.h>
#include "EnglishTextAnalyzer.h"

class EnglishTextAnalyzerTest : public ::testing::Test {
protected:
  EnglishTextAnalyzer analyzer;
  std::unordered_set<std::wstring> multiWordStems;
};

// Test stemming functionality
TEST_F(EnglishTextAnalyzerTest, CreateStemVector_SingleWord) {
  StemVector result = analyzer.createStemVector(L"bearing");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], L"bear");
}

TEST_F(EnglishTextAnalyzerTest, CreateStemVector_Plural) {
  StemVector singular = analyzer.createStemVector(L"bearing");
  StemVector plural = analyzer.createStemVector(L"bearings");
  // Both should stem to the same root
  EXPECT_EQ(singular[0], plural[0]);
}

TEST_F(EnglishTextAnalyzerTest, CreateStemVector_PastTense) {
  StemVector present = analyzer.createStemVector(L"connect");
  StemVector past = analyzer.createStemVector(L"connected");
  // Both should stem to similar root
  EXPECT_EQ(present[0], past[0]);
}

TEST_F(EnglishTextAnalyzerTest, CreateStemVector_CommonWords) {
  StemVector running = analyzer.createStemVector(L"running");
  StemVector run = analyzer.createStemVector(L"run");
  EXPECT_EQ(running[0], run[0]);
}

TEST_F(EnglishTextAnalyzerTest, CreateMultiWordStemVector) {
  StemVector result = analyzer.createMultiWordStemVector(L"first", L"bearing");
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], L"first");
  EXPECT_EQ(result[1], L"bear");
}

TEST_F(EnglishTextAnalyzerTest, CreateMultiWordStemVector_Different) {
  StemVector result1 = analyzer.createMultiWordStemVector(L"first", L"bearing");
  StemVector result2 = analyzer.createMultiWordStemVector(L"second", L"bearing");
  EXPECT_NE(result1, result2); // Different first words
  EXPECT_EQ(result1[1], result2[1]); // Same second word stem
}

// Test multi-word base detection
TEST_F(EnglishTextAnalyzerTest, IsMultiWordBase_EmptySet) {
  bool result = analyzer.isMultiWordBase(L"bearing", multiWordStems);
  EXPECT_FALSE(result);
}

TEST_F(EnglishTextAnalyzerTest, IsMultiWordBase_WordInSet) {
  multiWordStems.insert(L"bear");
  bool result = analyzer.isMultiWordBase(L"bearing", multiWordStems);
  EXPECT_TRUE(result);
}

TEST_F(EnglishTextAnalyzerTest, IsMultiWordBase_CaseInsensitive) {
  multiWordStems.insert(L"bear");
  bool result1 = analyzer.isMultiWordBase(L"bearing", multiWordStems);
  bool result2 = analyzer.isMultiWordBase(L"BEARING", multiWordStems);
  EXPECT_TRUE(result1);
  EXPECT_TRUE(result2);
}

// Test article detection
TEST_F(EnglishTextAnalyzerTest, IsDefiniteArticle_The) {
  EXPECT_TRUE(EnglishTextAnalyzer::isDefiniteArticle(L"the"));
  EXPECT_TRUE(EnglishTextAnalyzer::isDefiniteArticle(L"The"));
  EXPECT_TRUE(EnglishTextAnalyzer::isDefiniteArticle(L"THE"));
}

TEST_F(EnglishTextAnalyzerTest, IsDefiniteArticle_NotArticle) {
  EXPECT_FALSE(EnglishTextAnalyzer::isDefiniteArticle(L"a"));
  EXPECT_FALSE(EnglishTextAnalyzer::isDefiniteArticle(L"an"));
  EXPECT_FALSE(EnglishTextAnalyzer::isDefiniteArticle(L"bearing"));
  EXPECT_FALSE(EnglishTextAnalyzer::isDefiniteArticle(L""));
}

TEST_F(EnglishTextAnalyzerTest, IsIndefiniteArticle_A) {
  EXPECT_TRUE(EnglishTextAnalyzer::isIndefiniteArticle(L"a"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIndefiniteArticle(L"A"));
}

TEST_F(EnglishTextAnalyzerTest, IsIndefiniteArticle_An) {
  EXPECT_TRUE(EnglishTextAnalyzer::isIndefiniteArticle(L"an"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIndefiniteArticle(L"An"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIndefiniteArticle(L"AN"));
}

TEST_F(EnglishTextAnalyzerTest, IsIndefiniteArticle_NotArticle) {
  EXPECT_FALSE(EnglishTextAnalyzer::isIndefiniteArticle(L"the"));
  EXPECT_FALSE(EnglishTextAnalyzer::isIndefiniteArticle(L"bearing"));
  EXPECT_FALSE(EnglishTextAnalyzer::isIndefiniteArticle(L""));
}

// Test preceding word extraction
TEST_F(EnglishTextAnalyzerTest, FindPrecedingWord_Simple) {
  std::wstring text = L"the bearing 10";
  auto [word, pos] = EnglishTextAnalyzer::findPrecedingWord(text, 4);
  EXPECT_EQ(word, L"the");
  EXPECT_EQ(pos, 0);
}

TEST_F(EnglishTextAnalyzerTest, FindPrecedingWord_AtStart) {
  std::wstring text = L"bearing 10";
  auto [word, pos] = EnglishTextAnalyzer::findPrecedingWord(text, 0);
  EXPECT_TRUE(word.empty());
}

TEST_F(EnglishTextAnalyzerTest, FindPrecedingWord_MultipleWords) {
  std::wstring text = L"This is a bearing 10";
  auto [word, pos] = EnglishTextAnalyzer::findPrecedingWord(text, 10);
  EXPECT_EQ(word, L"a");
}

// Test stem caching
TEST_F(EnglishTextAnalyzerTest, StemCaching_Performance) {
  // First call
  StemVector result1 = analyzer.createStemVector(L"bearing");

  // Second call should hit cache
  StemVector result2 = analyzer.createStemVector(L"bearing");

  EXPECT_EQ(result1, result2);
}

TEST_F(EnglishTextAnalyzerTest, StemCaching_DifferentWords) {
  StemVector result1 = analyzer.createStemVector(L"bearing");
  StemVector result2 = analyzer.createStemVector(L"motor");

  EXPECT_NE(result1, result2);
}

// Test edge cases
TEST_F(EnglishTextAnalyzerTest, CreateStemVector_EmptyString) {
  StemVector result = analyzer.createStemVector(L"");
  EXPECT_TRUE(result.empty() || result[0].empty());
}

TEST_F(EnglishTextAnalyzerTest, CreateStemVector_SingleChar) {
  StemVector result = analyzer.createStemVector(L"a");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], L"a");
}

// Test Porter stemmer specific cases
TEST_F(EnglishTextAnalyzerTest, PorterStemmer_ATIONAL_to_ATE) {
  StemVector result = analyzer.createStemVector(L"relational");
  EXPECT_EQ(result[0], L"relat");
}

TEST_F(EnglishTextAnalyzerTest, PorterStemmer_ING_Removal) {
  StemVector result = analyzer.createStemVector(L"housing");
  EXPECT_EQ(result[0], L"hous");
}

TEST_F(EnglishTextAnalyzerTest, PorterStemmer_ED_Removal) {
  StemVector result = analyzer.createStemVector(L"agreed");
  EXPECT_EQ(result[0], L"agre");
}

// Test isIgnoredWord functionality
TEST_F(EnglishTextAnalyzerTest, IsIgnoredWord_DefiniteArticle) {
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"the"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"The"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"THE"));
}

TEST_F(EnglishTextAnalyzerTest, IsIgnoredWord_IndefiniteArticles) {
  // Note: "a" is only 1 character, so it's ignored due to length
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"a"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"A"));
  // "an" is only 2 characters, so it's ignored due to length
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"an"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"An"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"AN"));
}

TEST_F(EnglishTextAnalyzerTest, IsIgnoredWord_Figure) {
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"figure"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"Figure"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"FIGURE"));
}

TEST_F(EnglishTextAnalyzerTest, IsIgnoredWord_Figures) {
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"figures"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"Figures"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"FIGURES"));
}

TEST_F(EnglishTextAnalyzerTest, IsIgnoredWord_NotIgnored) {
  // Regular words should not be ignored
  EXPECT_FALSE(EnglishTextAnalyzer::isIgnoredWord(L"bearing"));
  EXPECT_FALSE(EnglishTextAnalyzer::isIgnoredWord(L"motor"));
  EXPECT_FALSE(EnglishTextAnalyzer::isIgnoredWord(L"shaft"));
  EXPECT_FALSE(EnglishTextAnalyzer::isIgnoredWord(L"housing"));
}

TEST_F(EnglishTextAnalyzerTest, IsIgnoredWord_ShortWords) {
  // Words shorter than 3 characters should always be ignored
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"at"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"in"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"to"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"a"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"is"));
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L""));
}

TEST_F(EnglishTextAnalyzerTest, IsIgnoredWord_ExactlyThreeCharacters) {
  // 3-character articles should be ignored
  EXPECT_TRUE(EnglishTextAnalyzer::isIgnoredWord(L"the"));
  
  // 3-character non-articles should NOT be ignored
  EXPECT_FALSE(EnglishTextAnalyzer::isIgnoredWord(L"rod"));
  EXPECT_FALSE(EnglishTextAnalyzer::isIgnoredWord(L"box"));
  EXPECT_FALSE(EnglishTextAnalyzer::isIgnoredWord(L"pin"));
}
