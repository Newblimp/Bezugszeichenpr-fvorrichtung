#include <gtest/gtest.h>
#include "ErrorDetectorHelper.h"
#include "TextScanner.h"
#include "MainWindow.h"
#include "RegexPatterns.h"
#include "AnalysisContext.h"
#include <wx/wx.h>
#include <wx/richtext/richtextctrl.h>
#include <re2/re2.h>

/**
 * Minimal wxApp for testing
 * Required to initialize wxWidgets components
 */
class TestApp : public wxApp {
public:
    virtual bool OnInit() override { return true; }
};

/**
 * Test fixture for ErrorDetectorHelper tests
 * Initializes wxWidgets and creates necessary UI components for error detection
 */
class ErrorDetectorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Initialize wxWidgets once for all tests
        int argc = 0;
        char** argv = nullptr;
        if (!wxTheApp) {
            wxApp::SetInstance(new TestApp());
            wxEntryStart(argc, argv);
            wxTheApp->CallOnInit();
        }
    }

    void SetUp() override {
        // Create a frame (required for wxRichTextCtrl)
        frame = new wxFrame(nullptr, wxID_ANY, "Test Frame");

        // Create text box
        textBox = new wxRichTextCtrl(frame, wxID_ANY);

        // Initialize regex patterns
        singleWordRegex = std::make_unique<re2::RE2>(RegexPatterns::SINGLE_WORD_PATTERN);
        twoWordRegex = std::make_unique<re2::RE2>(RegexPatterns::TWO_WORD_PATTERN);
        wordRegex = std::make_unique<re2::RE2>(RegexPatterns::WORD_PATTERN);

        // Create text styles
        warningStyle.SetBackgroundColour(wxColour(255, 255, 0)); // Yellow
        conflictStyle.SetBackgroundColour(wxColour(255, 165, 0)); // Orange
        articleWarningStyle.SetBackgroundColour(wxColour(0, 255, 255)); // Cyan

        // Clear all data structures
        clearDataStructures();
    }

    void TearDown() override {
        // Clean up wx components
        if (textBox) {
            textBox->Destroy();
            textBox = nullptr;
        }
        if (frame) {
            frame->Destroy();
            frame = nullptr;
        }
    }

    void clearDataStructures() {
        ctx.clearResults();
        ctx.multiWordBaseStems.clear();
        ctx.clearedTextPositions.clear();
        ctx.clearedErrors.clear();
        noNumberPositions.clear();
        wrongTermBzPositions.clear();
        wrongArticlePositions.clear();
        allErrorsPositions.clear();
    }

    // Helper to scan text and populate data structures
    void scanText(const std::wstring& text) {
        textBox->SetValue(wxString(text));
        TextScanner::scanText(text, analyzer, *singleWordRegex, *twoWordRegex, ctx);
    }

    // Analyzer
    GermanTextAnalyzer analyzer;

    // Regex patterns
    std::unique_ptr<re2::RE2> singleWordRegex;
    std::unique_ptr<re2::RE2> twoWordRegex;
    std::unique_ptr<re2::RE2> wordRegex;

    // wxWidgets components
    wxFrame* frame = nullptr;
    wxRichTextCtrl* textBox = nullptr;
    wxTextAttr warningStyle;
    wxTextAttr conflictStyle;
    wxTextAttr articleWarningStyle;

    // Data structures
    AnalysisContext ctx;

    // Error position vectors
    std::vector<std::pair<int, int>> noNumberPositions;
    std::vector<std::pair<int, int>> wrongTermBzPositions;
    std::vector<std::pair<int, int>> wrongArticlePositions;
    std::vector<std::pair<int, int>> allErrorsPositions;
};

// Test 9: FindUnnumberedWords_Basic
TEST_F(ErrorDetectorTest, FindUnnumberedWords_Basic) {
    std::wstring text = L"Lager 10 ist ein Lager ohne Nummer";

    // Scan to establish that "Lager" is a known term
    scanText(text);

    // Find unnumbered words
    ErrorDetectorHelper::findUnnumberedWords(text, analyzer, *wordRegex, ctx,
                                            textBox, warningStyle, noNumberPositions,
                                            allErrorsPositions);

    // Should detect the second "Lager" without a number
    EXPECT_FALSE(noNumberPositions.empty());

    // Verify at least one error was found
    EXPECT_GT(noNumberPositions.size(), 0);
}

// Test 10: FindUnnumberedWords_MultiWord
TEST_F(ErrorDetectorTest, FindUnnumberedWords_MultiWord) {
    std::wstring text = L"erstes Lager 10 später ein erstes Lager";

    // Enable multi-word matching
    StemVector lagerStem = analyzer.createStemVector(L"Lager");
    ctx.multiWordBaseStems.insert(lagerStem[0]);

    // Scan to establish "erstes Lager" as known term
    scanText(text);

    // Find unnumbered words
    ErrorDetectorHelper::findUnnumberedWords(text, analyzer, *wordRegex, ctx,
                                            textBox, warningStyle, noNumberPositions,
                                            allErrorsPositions);

    // Should detect "erstes Lager" without number in the second occurrence
    EXPECT_FALSE(noNumberPositions.empty());
}

// Test 11: DetectConflictingAssignments
TEST_F(ErrorDetectorTest, DetectConflictingAssignments) {
    std::wstring text = L"Lager 10 und Lager 20";

    scanText(text);

    // Check each BZ for unique assignment
    for (const auto& [bz, stems] : ctx.db.bzToStems) {
        ErrorDetectorHelper::isUniquelyAssigned(bz, ctx,
                                               textBox, conflictStyle, wrongTermBzPositions,
                                               allErrorsPositions);
    }

    // Should detect conflicting assignment: same stem "lag" with different BZs
    EXPECT_FALSE(wrongTermBzPositions.empty());

    // Both occurrences should be highlighted
    EXPECT_GE(wrongTermBzPositions.size(), 2);
}

// Test 12: DetectSplitAssignments
TEST_F(ErrorDetectorTest, DetectSplitAssignments) {
    std::wstring text = L"Lager 10 und Motor 10";

    scanText(text);

    // Check BZ "10" for unique assignment
    bool isUnique = ErrorDetectorHelper::isUniquelyAssigned(
        L"10", ctx, textBox, conflictStyle,
        wrongTermBzPositions, allErrorsPositions);

    // Should detect split assignment: same BZ "10" for different terms
    EXPECT_FALSE(isUnique);

    // Should highlight as wrong term/BZ (conflict)
    EXPECT_FALSE(wrongTermBzPositions.empty());
}

// Test 13: ArticleValidation_DefiniteVsIndefinite
TEST_F(ErrorDetectorTest, ArticleValidation_DefiniteVsIndefinite) {
    // First occurrence with definite article "der" (WRONG - should be indefinite)
    // Second occurrence with indefinite article "ein" (WRONG - should be definite)
    std::wstring text = L"der Lager 10 ist ein Lager 10";

    scanText(text);

    // Check article usage
    ErrorDetectorHelper::checkArticleUsage(text, analyzer, ctx,
                                          textBox, articleWarningStyle, wrongArticlePositions,
                                          allErrorsPositions);

    // BOTH occurrences should be flagged:
    // 1. First "der" is definite (should be indefinite for first occurrence)
    // 2. Second "ein" is indefinite (should be definite for subsequent occurrence)
    EXPECT_FALSE(wrongArticlePositions.empty());
    EXPECT_EQ(wrongArticlePositions.size(), 2);

    // Verify "der" was flagged (position should be at start)
    bool foundDer = false;
    for (const auto& [start, end] : wrongArticlePositions) {
        if (start < 5) { // "der" is at the beginning
            foundDer = true;
            break;
        }
    }
    EXPECT_TRUE(foundDer);

    // Verify "ein" was also flagged (position should be later in text)
    bool foundEin = false;
    for (const auto& [start, end] : wrongArticlePositions) {
        if (start > 10) { // "ein" is later in the text
            foundEin = true;
            break;
        }
    }
    EXPECT_TRUE(foundEin);
}

// Test 14: ArticleValidation_FirstOccurrenceBaseline
TEST_F(ErrorDetectorTest, ArticleValidation_FirstOccurrenceBaseline) {
    // First occurrence sets indefinite baseline
    // Second with definite should NOT be flagged
    std::wstring text = L"ein Lager 10 dann der Lager 10";

    scanText(text);

    // Check article usage
    ErrorDetectorHelper::checkArticleUsage(text, analyzer, ctx,
                                          textBox, articleWarningStyle, wrongArticlePositions,
                                          allErrorsPositions);

    // First "ein" is indefinite (correct for first occurrence)
    // Second "der" is definite (correct for subsequent occurrence)
    // No errors should be found
    EXPECT_TRUE(wrongArticlePositions.empty());
}

// Test 15: ArticleValidation_NoArticle
TEST_F(ErrorDetectorTest, ArticleValidation_NoArticle) {
    // First occurrence without article, second with definite article
    std::wstring text = L"Lager 10 ist wichtig. der Lager 10";

    scanText(text);

    // Check article usage
    ErrorDetectorHelper::checkArticleUsage(text, analyzer, ctx,
                                          textBox, articleWarningStyle, wrongArticlePositions,
                                          allErrorsPositions);

    // No error should be flagged because the subsequent article "der" is definite (correct)
    // The first occurrence having no article doesn't affect validation
    EXPECT_TRUE(wrongArticlePositions.empty());
}

// Test 16: RespectClearedBZNumbers
TEST_F(ErrorDetectorTest, RespectClearedBZNumbers) {
    std::wstring text = L"Lager 10 und Motor 10";

    scanText(text);

    // Clear the error for BZ "10"
    ctx.clearedErrors.insert(L"10");

    // Check BZ "10" for unique assignment
    bool isUnique = ErrorDetectorHelper::isUniquelyAssigned(
        L"10", ctx, textBox, conflictStyle,
        wrongTermBzPositions, allErrorsPositions);

    // Should treat as unique (no error) because it was cleared
    EXPECT_TRUE(isUnique);

    // No positions should be added
    EXPECT_TRUE(wrongTermBzPositions.empty());
}

// Test 17: RespectClearedTextPositions
TEST_F(ErrorDetectorTest, RespectClearedTextPositions) {
    std::wstring text = L"Lager 10 ist ein Lager ohne Nummer";

    scanText(text);

    // Find the position of the unnumbered "Lager"
    // It should be around position 17 (after "ein ")
    size_t unnumberedPos = text.find(L"ein Lager") + 4; // Position of "Lager"
    size_t unnumberedEnd = unnumberedPos + 5;           // Length of "Lager"

    // Clear this specific position
    ctx.clearedTextPositions.insert({unnumberedPos, unnumberedEnd});

    // Find unnumbered words
    ErrorDetectorHelper::findUnnumberedWords(text, analyzer, *wordRegex, ctx,
                                            textBox, warningStyle, noNumberPositions,
                                            allErrorsPositions);

    // The cleared position should not be re-detected
    bool foundClearedPosition = false;
    for (const auto& [start, end] : noNumberPositions) {
        if (static_cast<size_t>(start) == unnumberedPos &&
            static_cast<size_t>(end) == unnumberedEnd) {
            foundClearedPosition = true;
            break;
        }
    }

    EXPECT_FALSE(foundClearedPosition);
}

// Test 18: ErrorPositionVectorGeneration
TEST_F(ErrorDetectorTest, ErrorPositionVectorGeneration) {
    // Use text with multiple unnumbered occurrences of "Lager"
    std::wstring text = L"Lager 10 Lager Lager Lager";
    textBox->SetValue(wxString(text));

    // Scan
    clearDataStructures();
    scanText(text);

    // Find unnumbered words
    ErrorDetectorHelper::findUnnumberedWords(text, analyzer, *wordRegex, ctx,
                                            textBox, warningStyle, noNumberPositions,
                                            allErrorsPositions);

    // Should have multiple errors (3 unnumbered "Lager" instances)
    EXPECT_GE(noNumberPositions.size(), 3);

    // Verify format: each entry is a (start, end) pair
    for (const auto& [start, end] : noNumberPositions) {
        EXPECT_LT(start, end); // Start should be before end
        EXPECT_GE(start, 0);   // Should be non-negative
    }

    // Verify allErrorsPositions also contains these
    EXPECT_GE(allErrorsPositions.size(), noNumberPositions.size());
}

// Additional test: Verify isPositionCleared helper function
TEST_F(ErrorDetectorTest, IsPositionClearedHelper) {
    std::set<std::pair<size_t, size_t>> cleared;
    cleared.insert({10, 20});
    cleared.insert({30, 40});

    // Test exact match
    EXPECT_TRUE(ErrorDetectorHelper::isPositionCleared(cleared, 10, 20));
    EXPECT_TRUE(ErrorDetectorHelper::isPositionCleared(cleared, 30, 40));

    // Test non-match
    EXPECT_FALSE(ErrorDetectorHelper::isPositionCleared(cleared, 0, 5));
    EXPECT_FALSE(ErrorDetectorHelper::isPositionCleared(cleared, 15, 25));
    EXPECT_FALSE(ErrorDetectorHelper::isPositionCleared(cleared, 10, 21)); // Slightly different end

    // Test empty set
    std::set<std::pair<size_t, size_t>> emptyCleared;
    EXPECT_FALSE(ErrorDetectorHelper::isPositionCleared(emptyCleared, 10, 20));
}

// Test for article validation with multiple terms
TEST_F(ErrorDetectorTest, ArticleValidation_MultipleTerms) {
    // Multiple different terms with correct article usage
    std::wstring text = L"ein Lager 10 ein Motor 20 der Lager 10 der Motor 20";

    scanText(text);

    // Check article usage
    ErrorDetectorHelper::checkArticleUsage(text, analyzer, ctx,
                                          textBox, articleWarningStyle, wrongArticlePositions,
                                          allErrorsPositions);

    // All articles are correct:
    // - "Lager" first: indefinite (ein) ✓, second: definite (der) ✓
    // - "Motor" first: indefinite (ein) ✓, second: definite (der) ✓
    EXPECT_TRUE(wrongArticlePositions.empty());
}

// Test for conflicting assignments with cleared errors
TEST_F(ErrorDetectorTest, ConflictingAssignments_AfterClearing) {
    std::wstring text = L"Lager 10 Lager 20 Lager 30";

    scanText(text);

    // Initially, all should be flagged as conflicts
    for (const auto& [bz, stems] : ctx.db.bzToStems) {
        ErrorDetectorHelper::isUniquelyAssigned(bz, ctx,
                                               textBox, conflictStyle, wrongTermBzPositions,
                                               allErrorsPositions);
    }

    size_t initialErrorCount = wrongTermBzPositions.size();
    EXPECT_GT(initialErrorCount, 0);

    // Clear data and rescan with errors cleared
    clearDataStructures();
    scanText(text);

    // Clear all BZ errors
    ctx.clearedErrors.insert(L"10");
    ctx.clearedErrors.insert(L"20");
    ctx.clearedErrors.insert(L"30");

    // Check again
    for (const auto& [bz, stems] : ctx.db.bzToStems) {
        bool isUnique = ErrorDetectorHelper::isUniquelyAssigned(
            bz, ctx, textBox, conflictStyle,
            wrongTermBzPositions, allErrorsPositions);

        // All should be treated as unique now
        EXPECT_TRUE(isUnique);
    }

    // No new errors should be added
    EXPECT_TRUE(wrongTermBzPositions.empty());
}
