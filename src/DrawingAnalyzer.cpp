#ifdef HAVE_OCR_SUPPORT

#include "DrawingAnalyzer.h"
#include "utils.h"  // For BZComparatorForMap
#include <algorithm>

void DrawingAnalyzer::setReferenceDatabase(const ReferenceDatabase* db) {
    m_refDb = db;
}

void DrawingAnalyzer::validateResults(OcrResult& ocrResult) {
    if (!m_refDb) {
        // No database available - mark all as NOT_VALIDATED
        for (auto& ref : ocrResult.references) {
            ref.status = DetectedReference::ValidationStatus::NOT_VALIDATED;
            ref.validationMessage = L"No text analysis database available";
        }
        return;
    }

    for (auto& ref : ocrResult.references) {
        if (existsInTextAnalysis(ref.text)) {
            ref.status = DetectedReference::ValidationStatus::VALID;

            // Get associated terms for display
            auto terms = getTermsForBz(ref.text);
            if (!terms.empty()) {
                ref.validationMessage = L"Found in text (BZ " + ref.text + L")";
            } else {
                ref.validationMessage = L"Valid BZ " + ref.text;
            }
        } else {
            ref.status = DetectedReference::ValidationStatus::MISSING_IN_TEXT;
            ref.validationMessage = L"BZ " + ref.text +
                L" found in drawing but not referenced in text";
        }
    }
}

std::vector<std::wstring> DrawingAnalyzer::findMissingInDrawing(
    const OcrResult& ocrResult) const {

    std::vector<std::wstring> missing;

    if (!m_refDb) {
        return missing;
    }

    // Build set of detected BZs
    std::unordered_set<std::wstring> detectedBzs;
    for (const auto& ref : ocrResult.references) {
        detectedBzs.insert(ref.text);
    }

    // Check all BZs from text analysis
    for (const auto& [bz, stems] : m_refDb->bzToStems) {
        if (detectedBzs.find(bz) == detectedBzs.end()) {
            missing.push_back(bz);
        }
    }

    // Sort numerically using BZComparatorForMap logic
    std::sort(missing.begin(), missing.end(), BZComparatorForMap());

    return missing;
}

DrawingAnalyzer::ValidationStats DrawingAnalyzer::getStats(
    const OcrResult& ocrResult) const {

    ValidationStats stats;
    stats.totalDetected = ocrResult.references.size();

    for (const auto& ref : ocrResult.references) {
        switch (ref.status) {
            case DetectedReference::ValidationStatus::VALID:
                stats.validCount++;
                break;
            case DetectedReference::ValidationStatus::MISSING_IN_TEXT:
                stats.missingInTextCount++;
                break;
            case DetectedReference::ValidationStatus::CONFLICT:
                stats.conflictCount++;
                break;
            default:
                break;
        }
    }

    // Count missing in drawing
    stats.missingInDrawingCount = findMissingInDrawing(ocrResult).size();

    return stats;
}

bool DrawingAnalyzer::existsInTextAnalysis(const std::wstring& bz) const {
    if (!m_refDb) return false;
    return m_refDb->bzToStems.find(bz) != m_refDb->bzToStems.end();
}

std::unordered_set<StemVector, StemVectorHash>
DrawingAnalyzer::getTermsForBz(const std::wstring& bz) const {
    if (!m_refDb) return {};

    auto it = m_refDb->bzToStems.find(bz);
    if (it != m_refDb->bzToStems.end()) {
        return it->second;
    }
    return {};
}

#endif // HAVE_OCR_SUPPORT
