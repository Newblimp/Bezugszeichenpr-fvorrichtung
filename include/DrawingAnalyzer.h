#pragma once

#ifdef HAVE_OCR_SUPPORT

#include "IOcrEngine.h"
#include "ReferenceDatabase.h"
#include <memory>
#include <vector>

/**
 * @brief Coordinates OCR processing and cross-validation with text analysis
 *
 * Workflow:
 * 1. Receives OcrResult from NcnnOcrEngine
 * 2. Accesses ReferenceDatabase from MainWindow (via pointer)
 * 3. Validates each detected reference against known BZ→Term mappings
 * 4. Identifies discrepancies:
 *    - References in drawing but not in text (MISSING_IN_TEXT)
 *    - References in text but not in drawing (MISSING_IN_DRAWING)
 *    - Mismatched confidence levels
 */
class DrawingAnalyzer {
public:
    DrawingAnalyzer() = default;

    // Set reference to MainWindow's database for cross-validation
    void setReferenceDatabase(const ReferenceDatabase* db);

    // Validate OCR results against text analysis
    // Modifies OcrResult in-place to set validation status
    void validateResults(OcrResult& ocrResult);

    // Find references present in text but missing from drawing
    std::vector<std::wstring> findMissingInDrawing(const OcrResult& ocrResult) const;

    // Statistics
    struct ValidationStats {
        size_t totalDetected{0};
        size_t validCount{0};
        size_t missingInTextCount{0};
        size_t missingInDrawingCount{0};
        size_t conflictCount{0};
    };

    ValidationStats getStats(const OcrResult& ocrResult) const;

private:
    // Check if a BZ exists in the text analysis database
    bool existsInTextAnalysis(const std::wstring& bz) const;

    // Get terms associated with a BZ from text analysis
    std::unordered_set<StemVector, StemVectorHash>
        getTermsForBz(const std::wstring& bz) const;

    const ReferenceDatabase* m_refDb{nullptr};
};

#endif // HAVE_OCR_SUPPORT
