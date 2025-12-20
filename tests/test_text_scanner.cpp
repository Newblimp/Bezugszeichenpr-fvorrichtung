#include <gtest/gtest.h>
#include "TextScanner.h"
#include "MainWindow.h"
#include "RegexPatterns.h"
#include "AnalysisContext.h"
#include <re2/re2.h>

/**
 * Test fixture for TextScanner tests
 * These tests validate the text scanning and pattern matching logic
 */
class TextScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize regex patterns
        singleWordRegex = std::make_unique<re2::RE2>(RegexPatterns::SINGLE_WORD_PATTERN);
        twoWordRegex = std::make_unique<re2::RE2>(RegexPatterns::TWO_WORD_PATTERN);

        // Clear all data structures
        clearDataStructures();
    }

    void clearDataStructures() {
        ctx.clearResults();
        ctx.multiWordBaseStems.clear();
        ctx.clearedTextPositions.clear();
    }

    // Analyzer
    GermanTextAnalyzer analyzer;

    // Regex patterns
    std::unique_ptr<re2::RE2> singleWordRegex;
    std::unique_ptr<re2::RE2> twoWordRegex;

    // Data structures that TextScanner populates
    AnalysisContext ctx;
};


// Test 1: BasicSingleWordScanning
TEST_F(TextScannerTest, BasicSingleWordScanning) {
    std::wstring text = L"Lager 10 Motor 20";

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // Verify bzToStems mapping for "10"
    ASSERT_TRUE(ctx.db.bzToStems.count(L"10"));
    StemVector lagerStem = analyzer.createStemVector(L"Lager");
    EXPECT_TRUE(ctx.db.bzToStems[L"10"].count(lagerStem) > 0);

    // Verify bzToStems mapping for "20"
    ASSERT_TRUE(ctx.db.bzToStems.count(L"20"));
    StemVector motorStem = analyzer.createStemVector(L"Motor");
    EXPECT_TRUE(ctx.db.bzToStems[L"20"].count(motorStem) > 0);

    // Verify stemToBz reverse mapping
    ASSERT_TRUE(ctx.db.stemToBz.count(lagerStem));
    EXPECT_TRUE(ctx.db.stemToBz[lagerStem].count(L"10") > 0);

    ASSERT_TRUE(ctx.db.stemToBz.count(motorStem));
    EXPECT_TRUE(ctx.db.stemToBz[motorStem].count(L"20") > 0);
}

// Test 2: TwoWordPatternScanning
TEST_F(TextScannerTest, TwoWordPatternScanning) {
    std::wstring text = L"erstes Lager 10";

    // Enable multi-word matching for "Lager"
    StemVector lagerStem = analyzer.createStemVector(L"Lager");
    ctx.multiWordBaseStems.insert(lagerStem[0]); // Insert the stem "lag"

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // Create expected two-word stem vector
    StemVector expectedStem = analyzer.createMultiWordStemVector(L"erstes", L"Lager");

    // Verify bzToStems contains the two-word stem
    ASSERT_TRUE(ctx.db.bzToStems.count(L"10"));
    EXPECT_TRUE(ctx.db.bzToStems[L"10"].count(expectedStem) > 0);

    // Verify stemToBz reverse mapping
    ASSERT_TRUE(ctx.db.stemToBz.count(expectedStem));
    EXPECT_TRUE(ctx.db.stemToBz[expectedStem].count(L"10") > 0);

    // Verify it's a two-element StemVector
    EXPECT_EQ(expectedStem.size(), 2);
}

// Test 3: BuildBZToStemsMappings
TEST_F(TextScannerTest, BuildBZToStemsMappings) {
    std::wstring text = L"Lager 10 Motor 10";

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // BZ "10" should map to both "Lager" and "Motor" stems
    ASSERT_TRUE(ctx.db.bzToStems.count(L"10"));
    EXPECT_EQ(ctx.db.bzToStems[L"10"].size(), 2);

    StemVector lagerStem = analyzer.createStemVector(L"Lager");
    StemVector motorStem = analyzer.createStemVector(L"Motor");

    EXPECT_TRUE(ctx.db.bzToStems[L"10"].count(lagerStem) > 0);
    EXPECT_TRUE(ctx.db.bzToStems[L"10"].count(motorStem) > 0);

    // Verify bzToOriginalWords contains both original words
    ASSERT_TRUE(ctx.db.bzToOriginalWords.count(L"10"));
    EXPECT_TRUE(ctx.db.bzToOriginalWords[L"10"].count(L"Lager") > 0);
    EXPECT_TRUE(ctx.db.bzToOriginalWords[L"10"].count(L"Motor") > 0);
}

// Test 4: BuildStemToBZMappings
TEST_F(TextScannerTest, BuildStemToBZMappings) {
    std::wstring text = L"Lager 10 Lager 20";

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // The same stem should map to both "10" and "20"
    StemVector lagerStem = analyzer.createStemVector(L"Lager");

    ASSERT_TRUE(ctx.db.stemToBz.count(lagerStem));
    EXPECT_EQ(ctx.db.stemToBz[lagerStem].size(), 2);
    EXPECT_TRUE(ctx.db.stemToBz[lagerStem].count(L"10") > 0);
    EXPECT_TRUE(ctx.db.stemToBz[lagerStem].count(L"20") > 0);
}

// Test 5: PositionTrackingSingleWord
TEST_F(TextScannerTest, PositionTrackingSingleWord) {
    std::wstring text = L"Lager 10 is a bearing";

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // Verify bzToPositions contains correct position for "10"
    ASSERT_TRUE(ctx.db.bzToPositions.count(L"10"));
    ASSERT_FALSE(ctx.db.bzToPositions[L"10"].empty());

    auto [start, len] = ctx.db.bzToPositions[L"10"][0];
    EXPECT_EQ(start, 0); // "Lager" starts at position 0
    EXPECT_GT(len, 0);   // Should have non-zero length

    // Verify the position corresponds to "Lager"
    std::wstring extractedWord = text.substr(start, len);
    EXPECT_TRUE(extractedWord.find(L"Lager") != std::wstring::npos);
}

// Test 6: PositionTrackingTwoWord
TEST_F(TextScannerTest, PositionTrackingTwoWord) {
    std::wstring text = L"erstes Lager 10";

    // Enable multi-word matching for "Lager"
    StemVector lagerStem = analyzer.createStemVector(L"Lager");
    ctx.multiWordBaseStems.insert(lagerStem[0]);

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // Create expected two-word stem
    StemVector expectedStem = analyzer.createMultiWordStemVector(L"erstes", L"Lager");

    // Verify stemToPositions has correct range
    ASSERT_TRUE(ctx.db.stemToPositions.count(expectedStem));
    ASSERT_FALSE(ctx.db.stemToPositions[expectedStem].empty());

    auto [start, len] = ctx.db.stemToPositions[expectedStem][0];
    EXPECT_EQ(start, 0); // "erstes Lager" starts at position 0
    EXPECT_GT(len, 0);

    // Verify the position corresponds to "erstes Lager"
    std::wstring extractedPhrase = text.substr(start, len);
    EXPECT_TRUE(extractedPhrase.find(L"erstes") != std::wstring::npos);
    EXPECT_TRUE(extractedPhrase.find(L"Lager") != std::wstring::npos);
}

// Test 7: MultiWordBaseStemDetection
TEST_F(TextScannerTest, MultiWordBaseStemDetection) {
    std::wstring text = L"Lager 10 erstes Lager 20 zweites Lager 30";

    // Initially scan without multi-word enabled
    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // Should only find single-word patterns
    StemVector lagerStem = analyzer.createStemVector(L"Lager");
    ASSERT_TRUE(ctx.db.stemToBz.count(lagerStem));
    // Note: "erstes" and "zweites" might be picked up as separate matches

    // Now enable multi-word for "Lager"
    clearDataStructures();
    ctx.multiWordBaseStems.insert(lagerStem[0]);

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // Should now find two-word patterns
    StemVector erstesLager = analyzer.createMultiWordStemVector(L"erstes", L"Lager");
    StemVector zweitesLager = analyzer.createMultiWordStemVector(L"zweites", L"Lager");

    EXPECT_TRUE(ctx.db.stemToBz.count(erstesLager) > 0);
    EXPECT_TRUE(ctx.db.stemToBz.count(zweitesLager) > 0);
}

// Test 8: PreventOverlappingMatches
TEST_F(TextScannerTest, PreventOverlappingMatches) {
    std::wstring text = L"erstes Lager 10";

    // Enable multi-word matching for "Lager"
    StemVector lagerStem = analyzer.createStemVector(L"Lager");
    ctx.multiWordBaseStems.insert(lagerStem[0]);

    TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);

    // Should only match "erstes Lager 10" as a two-word pattern
    // NOT "Lager 10" separately
    StemVector twoWordStem = analyzer.createMultiWordStemVector(L"erstes", L"Lager");

    // BZ "10" should only map to the two-word stem, not both
    ASSERT_TRUE(ctx.db.bzToStems.count(L"10"));
    EXPECT_EQ(ctx.db.bzToStems[L"10"].size(), 1);
    EXPECT_TRUE(ctx.db.bzToStems[L"10"].count(twoWordStem) > 0);

    // Should NOT contain single-word "Lager" stem
    StemVector singleWordStem = analyzer.createStemVector(L"Lager");
    EXPECT_FALSE(ctx.db.bzToStems[L"10"].count(singleWordStem) > 0);
}
