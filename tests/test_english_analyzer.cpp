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
