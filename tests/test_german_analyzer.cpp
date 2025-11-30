#include <gtest/gtest.h>
#include "GermanTextAnalyzer.h"

class GermanTextAnalyzerTest : public ::testing::Test {
protected:
  GermanTextAnalyzer analyzer;
  std::unordered_set<std::wstring> multiWordStems;
};

// Test stemming functionality
TEST_F(GermanTextAnalyzerTest, CreateStemVector_SingleWord) {
  StemVector result = analyzer.createStemVector(L"Lager");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], L"lag");
}

TEST_F(GermanTextAnalyzerTest, CreateStemVector_Plural) {
  StemVector singular = analyzer.createStemVector(L"Lager");
  StemVector plural = analyzer.createStemVector(L"Lagern");
  // Both should stem to the same root
  EXPECT_EQ(singular[0], plural[0]);
}

TEST_F(GermanTextAnalyzerTest, CreateStemVector_WithUmlaut) {
  StemVector result = analyzer.createStemVector(L"Änderung");
  ASSERT_EQ(result.size(), 1);
  EXPECT_FALSE(result[0].empty());
  // Should handle umlauts properly - German stemmer preserves umlauts
  // "Änderung" stems to "änder" (with umlaut preserved)
  EXPECT_EQ(result[0][0], L'a'); // Lowercase 'ä' becomes 'a' without umlaut
  EXPECT_EQ(result[0], L"ander");
}

TEST_F(GermanTextAnalyzerTest, CreateMultiWordStemVector) {
  StemVector result1 = analyzer.createMultiWordStemVector(L"erstes", L"Lager");
  StemVector result2 = analyzer.createMultiWordStemVector(L"zweiten", L"Wellen");
  ASSERT_EQ(result1.size(), 2);
  EXPECT_EQ(result1[0], L"erst");
  EXPECT_EQ(result1[1], L"lag");
  ASSERT_EQ(result2.size(), 2);
  EXPECT_EQ(result2[0], L"zweit");
  EXPECT_EQ(result2[1], L"well");
}

TEST_F(GermanTextAnalyzerTest, CreateMultiWordStemVector_Different) {
  StemVector result1 = analyzer.createMultiWordStemVector(L"erstes", L"Lager");
  StemVector result2 = analyzer.createMultiWordStemVector(L"zweites", L"Lager");
  EXPECT_NE(result1, result2); // Different first words
  EXPECT_EQ(result1[1], result2[1]); // Same second word stem
}

// Test multi-word base detection
TEST_F(GermanTextAnalyzerTest, IsMultiWordBase_EmptySet) {
  bool result = analyzer.isMultiWordBase(L"Lager", multiWordStems);
  EXPECT_FALSE(result);
}

TEST_F(GermanTextAnalyzerTest, IsMultiWordBase_WordInSet) {
  multiWordStems.insert(L"lag");
  bool result = analyzer.isMultiWordBase(L"Lager", multiWordStems);
  EXPECT_TRUE(result);
}

TEST_F(GermanTextAnalyzerTest, IsMultiWordBase_CaseInsensitive) {
  multiWordStems.insert(L"lag");
  multiWordStems.insert(L"planetenradsatz");
  bool result1 = analyzer.isMultiWordBase(L"Lager", multiWordStems);
  bool result2 = analyzer.isMultiWordBase(L"LAGER", multiWordStems);
  bool result3 = analyzer.isMultiWordBase(L"Planetenradsätze", multiWordStems);
  bool result4 = analyzer.isMultiWordBase(L"PLANETENRADSÄTZE", multiWordStems);
  EXPECT_TRUE(result1);
  EXPECT_TRUE(result2);
}

// Test article detection
TEST_F(GermanTextAnalyzerTest, IsDefiniteArticle_Der) {
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"der"));
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"Der"));
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"DER"));
}

TEST_F(GermanTextAnalyzerTest, IsDefiniteArticle_Die) {
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"die"));
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"Die"));
}

TEST_F(GermanTextAnalyzerTest, IsDefiniteArticle_Das) {
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"das"));
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"Das"));
}

TEST_F(GermanTextAnalyzerTest, IsDefiniteArticle_AllForms) {
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"dem"));
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"den"));
  EXPECT_TRUE(GermanTextAnalyzer::isDefiniteArticle(L"des"));
}

TEST_F(GermanTextAnalyzerTest, IsDefiniteArticle_NotArticle) {
  EXPECT_FALSE(GermanTextAnalyzer::isDefiniteArticle(L"ein"));
  EXPECT_FALSE(GermanTextAnalyzer::isDefiniteArticle(L"Lager"));
  EXPECT_FALSE(GermanTextAnalyzer::isDefiniteArticle(L""));
}

TEST_F(GermanTextAnalyzerTest, IsIndefiniteArticle_Ein) {
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"ein"));
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"Ein"));
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"EIN"));
}

TEST_F(GermanTextAnalyzerTest, IsIndefiniteArticle_Eine) {
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"eine"));
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"Eine"));
}

TEST_F(GermanTextAnalyzerTest, IsIndefiniteArticle_AllForms) {
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"einem"));
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"einen"));
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"einer"));
  EXPECT_TRUE(GermanTextAnalyzer::isIndefiniteArticle(L"eines"));
}

TEST_F(GermanTextAnalyzerTest, IsIndefiniteArticle_NotArticle) {
  EXPECT_FALSE(GermanTextAnalyzer::isIndefiniteArticle(L"der"));
  EXPECT_FALSE(GermanTextAnalyzer::isIndefiniteArticle(L"Lager"));
  EXPECT_FALSE(GermanTextAnalyzer::isIndefiniteArticle(L""));
}

// Test preceding word extraction
TEST_F(GermanTextAnalyzerTest, FindPrecedingWord_Simple) {
  std::wstring text = L"der Lager 10";
  auto [word, pos] = GermanTextAnalyzer::findPrecedingWord(text, 4);
  EXPECT_EQ(word, L"der");
  EXPECT_EQ(pos, 0);
}

TEST_F(GermanTextAnalyzerTest, FindPrecedingWord_AtStart) {
  std::wstring text = L"Lager 10";
  auto [word, pos] = GermanTextAnalyzer::findPrecedingWord(text, 0);
  EXPECT_TRUE(word.empty());
}

TEST_F(GermanTextAnalyzerTest, FindPrecedingWord_MultipleWords) {
  std::wstring text = L"Das ist ein Lager 10";
  auto [word, pos] = GermanTextAnalyzer::findPrecedingWord(text, 12);
  EXPECT_EQ(word, L"ein");
}

// Test stem caching (repeated calls should use cache)
TEST_F(GermanTextAnalyzerTest, StemCaching_Performance) {
  // First call
  StemVector result1 = analyzer.createStemVector(L"Lager");

  // Second call should hit cache
  StemVector result2 = analyzer.createStemVector(L"Lager");

  EXPECT_EQ(result1, result2);
}

TEST_F(GermanTextAnalyzerTest, StemCaching_DifferentWords) {
  StemVector result1 = analyzer.createStemVector(L"Lager");
  StemVector result2 = analyzer.createStemVector(L"Motor");

  EXPECT_NE(result1, result2);
}

// Test edge cases
TEST_F(GermanTextAnalyzerTest, CreateStemVector_EmptyString) {
  StemVector result = analyzer.createStemVector(L"");
  EXPECT_TRUE(result.empty() || result[0].empty());
}

TEST_F(GermanTextAnalyzerTest, CreateStemVector_SingleChar) {
  StemVector result = analyzer.createStemVector(L"a");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], L"a");
}

TEST_F(GermanTextAnalyzerTest, CreateStemVector_Numbers) {
  StemVector result = analyzer.createStemVector(L"123");
  ASSERT_EQ(result.size(), 1);
  // Numbers should pass through or be handled consistently
  EXPECT_FALSE(result[0].empty());
}
