#ifdef HAVE_OCR_SUPPORT

#include <gtest/gtest.h>
#include "DrawingAnalyzer.h"
#include "ReferenceDatabase.h"

// Test DrawingAnalyzer instantiation
TEST(DrawingAnalyzerTest, CanInstantiate) {
    EXPECT_NO_THROW({
        DrawingAnalyzer analyzer;
    });
}

// Test validation without database
TEST(DrawingAnalyzerTest, ValidatesWithoutDatabase) {
    DrawingAnalyzer analyzer;
    OcrResult result;

    DetectedReference ref;
    ref.text = L"123";
    ref.x = 10;
    ref.y = 20;
    ref.width = 30;
    ref.height = 40;
    ref.confidence = 0.9f;
    result.references.push_back(ref);

    analyzer.validateResults(result);

    EXPECT_EQ(result.references[0].status, DetectedReference::ValidationStatus::NOT_VALIDATED);
}

// Test validation with database
TEST(DrawingAnalyzerTest, ValidatesWithDatabase) {
    DrawingAnalyzer analyzer;
    ReferenceDatabase db;

    // Add a known reference
    StemVector stems;
    stems.push_back(L"bearing");
    db.bzToStems[L"10"] = {stems};

    analyzer.setReferenceDatabase(&db);

    OcrResult result;
    DetectedReference ref;
    ref.text = L"10";
    ref.confidence = 0.9f;
    result.references.push_back(ref);

    analyzer.validateResults(result);

    EXPECT_EQ(result.references[0].status, DetectedReference::ValidationStatus::VALID);
}

// Test finding missing references
TEST(DrawingAnalyzerTest, FindsMissingInDrawing) {
    DrawingAnalyzer analyzer;
    ReferenceDatabase db;

    StemVector stems;
    stems.push_back(L"bearing");
    db.bzToStems[L"10"] = {stems};
    db.bzToStems[L"20"] = {stems};

    analyzer.setReferenceDatabase(&db);

    OcrResult result;
    DetectedReference ref;
    ref.text = L"10";
    result.references.push_back(ref);

    auto missing = analyzer.findMissingInDrawing(result);

    EXPECT_EQ(missing.size(), 1);
    if (!missing.empty()) {
        EXPECT_EQ(missing[0], L"20");
    }
}

#endif // HAVE_OCR_SUPPORT
