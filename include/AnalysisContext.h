#pragma once

#include "ReferenceDatabase.h"
#include <unordered_set>
#include <set>
#include <string>

/**
 * @brief Global context for text analysis and scanning
 */
struct AnalysisContext {
    ReferenceDatabase db;

    // Set of base word STEMS that should trigger multi-word matching
    std::unordered_set<std::wstring> multiWordBaseStems;

    // Auto-detection tracking for ordinal-based multi-word terms
    std::unordered_set<std::wstring> autoDetectedMultiWordStems;

    // Manual toggles: stems that user explicitly enabled via context menu
    std::unordered_set<std::wstring> manualMultiWordToggles;

    // Manual disables: stems that user explicitly disabled via context menu
    std::unordered_set<std::wstring> manuallyDisabledMultiWord;

    // Set of BZ numbers whose errors have been cleared/ignored by user
    std::unordered_set<std::wstring> clearedErrors;

    // Track cleared text positions (for right-click clear on highlighted text)
    std::set<std::pair<size_t, size_t>> clearedTextPositions;

    void clearResults() {
        db.clear();
    }
};
