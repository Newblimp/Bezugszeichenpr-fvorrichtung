#include <gtest/gtest.h>
#include "TextScanner.h"
#include "MainWindow.h"
#include "RegexPatterns.h"
#include <re2/re2.h>

/**
 * Test fixture for TextScanner tests
 * These tests validate the text scanning and pattern matching logic
 */
class TextScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure German mode is active (matching main application behavior)
        MainWindow::s_useGerman = true;

        // Initialize regex patterns
        singleWordRegex = std::make_unique<re2::RE2>(RegexPatterns::SINGLE_WORD_PATTERN);
        twoWordRegex = std::make_unique<re2::RE2>(RegexPatterns::TWO_WORD_PATTERN);

        // Clear all data structures
        clearDataStructures();
    }

    void clearDataStructures() {
        bzToStems.clear();
        stemToBz.clear();
        bzToOriginalWords.clear();
        bzToPositions.clear();
        stemToPositions.clear();
        multiWordBaseStems.clear();
        clearedTextPositions.clear();
    }

    // Regex patterns
    std::unique_ptr<re2::RE2> singleWordRegex;
    std::unique_ptr<re2::RE2> twoWordRegex;

    // Data structures that TextScanner populates
    std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap> bzToStems;
    std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash> stemToBz;
    std::unordered_map<std::wstring, std::unordered_set<std::wstring>> bzToOriginalWords;
    std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>> bzToPositions;
    std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash> stemToPositions;
    std::unordered_set<std::wstring> multiWordBaseStems;
    std::set<std::pair<size_t, size_t>> clearedTextPositions;
};

// Test 1: BasicSingleWordScanning
TEST_F(TextScannerTest, BasicSingleWordScanning) {
    std::wstring text = L"Lager 10 Motor 20";

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // Verify bzToStems mapping for "10"
    ASSERT_TRUE(bzToStems.count(L"10"));
    StemVector lagerStem = MainWindow::createCurrentStemVector(L"Lager");
    EXPECT_TRUE(bzToStems[L"10"].count(lagerStem) > 0);

    // Verify bzToStems mapping for "20"
    ASSERT_TRUE(bzToStems.count(L"20"));
    StemVector motorStem = MainWindow::createCurrentStemVector(L"Motor");
    EXPECT_TRUE(bzToStems[L"20"].count(motorStem) > 0);

    // Verify stemToBz reverse mapping
    ASSERT_TRUE(stemToBz.count(lagerStem));
    EXPECT_TRUE(stemToBz[lagerStem].count(L"10") > 0);

    ASSERT_TRUE(stemToBz.count(motorStem));
    EXPECT_TRUE(stemToBz[motorStem].count(L"20") > 0);
}

// Test 2: TwoWordPatternScanning
TEST_F(TextScannerTest, TwoWordPatternScanning) {
    std::wstring text = L"erstes Lager 10";

    // Enable multi-word matching for "Lager"
    StemVector lagerStem = MainWindow::createCurrentStemVector(L"Lager");
    multiWordBaseStems.insert(lagerStem[0]); // Insert the stem "lag"

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // Create expected two-word stem vector
    StemVector expectedStem = MainWindow::createCurrentMultiWordStemVector(L"erstes", L"Lager");

    // Verify bzToStems contains the two-word stem
    ASSERT_TRUE(bzToStems.count(L"10"));
    EXPECT_TRUE(bzToStems[L"10"].count(expectedStem) > 0);

    // Verify stemToBz reverse mapping
    ASSERT_TRUE(stemToBz.count(expectedStem));
    EXPECT_TRUE(stemToBz[expectedStem].count(L"10") > 0);

    // Verify it's a two-element StemVector
    EXPECT_EQ(expectedStem.size(), 2);
}

// Test 3: BuildBZToStemsMappings
TEST_F(TextScannerTest, BuildBZToStemsMappings) {
    std::wstring text = L"Lager 10 Motor 10";

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // BZ "10" should map to both "Lager" and "Motor" stems
    ASSERT_TRUE(bzToStems.count(L"10"));
    EXPECT_EQ(bzToStems[L"10"].size(), 2);

    StemVector lagerStem = MainWindow::createCurrentStemVector(L"Lager");
    StemVector motorStem = MainWindow::createCurrentStemVector(L"Motor");

    EXPECT_TRUE(bzToStems[L"10"].count(lagerStem) > 0);
    EXPECT_TRUE(bzToStems[L"10"].count(motorStem) > 0);

    // Verify bzToOriginalWords contains both original words
    ASSERT_TRUE(bzToOriginalWords.count(L"10"));
    EXPECT_TRUE(bzToOriginalWords[L"10"].count(L"Lager") > 0);
    EXPECT_TRUE(bzToOriginalWords[L"10"].count(L"Motor") > 0);
}

// Test 4: BuildStemToBZMappings
TEST_F(TextScannerTest, BuildStemToBZMappings) {
    std::wstring text = L"Lager 10 Lager 20";

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // The same stem should map to both "10" and "20"
    StemVector lagerStem = MainWindow::createCurrentStemVector(L"Lager");

    ASSERT_TRUE(stemToBz.count(lagerStem));
    EXPECT_EQ(stemToBz[lagerStem].size(), 2);
    EXPECT_TRUE(stemToBz[lagerStem].count(L"10") > 0);
    EXPECT_TRUE(stemToBz[lagerStem].count(L"20") > 0);
}

// Test 5: PositionTrackingSingleWord
TEST_F(TextScannerTest, PositionTrackingSingleWord) {
    std::wstring text = L"Lager 10 is a bearing";

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // Verify bzToPositions contains correct position for "10"
    ASSERT_TRUE(bzToPositions.count(L"10"));
    ASSERT_FALSE(bzToPositions[L"10"].empty());

    auto [start, len] = bzToPositions[L"10"][0];
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
    StemVector lagerStem = MainWindow::createCurrentStemVector(L"Lager");
    multiWordBaseStems.insert(lagerStem[0]);

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // Create expected two-word stem
    StemVector expectedStem = MainWindow::createCurrentMultiWordStemVector(L"erstes", L"Lager");

    // Verify stemToPositions has correct range
    ASSERT_TRUE(stemToPositions.count(expectedStem));
    ASSERT_FALSE(stemToPositions[expectedStem].empty());

    auto [start, len] = stemToPositions[expectedStem][0];
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
    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // Should only find single-word patterns
    StemVector lagerStem = MainWindow::createCurrentStemVector(L"Lager");
    ASSERT_TRUE(stemToBz.count(lagerStem));
    // Note: "erstes" and "zweites" might be picked up as separate matches

    // Now enable multi-word for "Lager"
    clearDataStructures();
    multiWordBaseStems.insert(lagerStem[0]);

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // Should now find two-word patterns
    StemVector erstesLager = MainWindow::createCurrentMultiWordStemVector(L"erstes", L"Lager");
    StemVector zweitesLager = MainWindow::createCurrentMultiWordStemVector(L"zweites", L"Lager");

    EXPECT_TRUE(stemToBz.count(erstesLager) > 0);
    EXPECT_TRUE(stemToBz.count(zweitesLager) > 0);
}

// Test 8: PreventOverlappingMatches
TEST_F(TextScannerTest, PreventOverlappingMatches) {
    std::wstring text = L"erstes Lager 10";

    // Enable multi-word matching for "Lager"
    StemVector lagerStem = MainWindow::createCurrentStemVector(L"Lager");
    multiWordBaseStems.insert(lagerStem[0]);

    TextScanner::scanText(text, *singleWordRegex, *twoWordRegex, multiWordBaseStems,
                         clearedTextPositions, bzToStems, stemToBz, bzToOriginalWords,
                         bzToPositions, stemToPositions);

    // Should only match "erstes Lager 10" as a two-word pattern
    // NOT "Lager 10" separately
    StemVector twoWordStem = MainWindow::createCurrentMultiWordStemVector(L"erstes", L"Lager");

    // BZ "10" should only map to the two-word stem, not both
    ASSERT_TRUE(bzToStems.count(L"10"));
    EXPECT_EQ(bzToStems[L"10"].size(), 1);
    EXPECT_TRUE(bzToStems[L"10"].count(twoWordStem) > 0);

    // Should NOT contain single-word "Lager" stem
    StemVector singleWordStem = MainWindow::createCurrentStemVector(L"Lager");
    EXPECT_FALSE(bzToStems[L"10"].count(singleWordStem) > 0);
}
