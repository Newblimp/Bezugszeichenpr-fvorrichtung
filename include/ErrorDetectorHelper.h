#pragma once

#include "utils.h"
#include <wx/richtext/richtextctrl.h>
#include <re2/re2.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>

/**
 * @brief Handles detection of various errors in reference number usage
 *
 * This class is responsible for detecting:
 * - Unnumbered words (terms that should have reference numbers but don't)
 * - Article usage errors (definite vs indefinite articles)
 * - Conflicting assignments (same number for different terms)
 */
class ErrorDetectorHelper {
public:
    /**
     * @brief Find words that should be numbered but aren't
     */
    static void findUnnumberedWords(
        const std::wstring& fullText,
        const re2::RE2& wordRegex,
        const std::unordered_set<std::wstring>& multiWordBaseStems,
        const std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions,
        const std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
        const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
        wxRichTextCtrl* textBox,
        const wxTextAttr& warningStyle,
        std::vector<std::pair<int, int>>& noNumberPositions,
        std::vector<std::pair<int, int>>& allErrorsPositions
    );

    /**
     * @brief Check for incorrect article usage (definite vs indefinite)
     */
    static void checkArticleUsage(
        const std::wstring& fullText,
        const std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions,
        const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
        wxRichTextCtrl* textBox,
        const wxTextAttr& articleWarningStyle,
        std::vector<std::pair<int, int>>& wrongArticlePositions,
        std::vector<std::pair<int, int>>& allErrorsPositions
    );

    /**
     * @brief Check if a uniquely assigned reference number exists
     */
    static bool isUniquelyAssigned(
        const std::wstring& bz,
        const std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
        const std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
        const std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
        const std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions,
        const std::unordered_set<std::wstring>& clearedErrors,
        const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
        wxRichTextCtrl* textBox,
        const wxTextAttr& warningStyle,
        std::vector<std::pair<int, int>>& wrongNumberPositions,
        std::vector<std::pair<int, int>>& splitNumberPositions,
        std::vector<std::pair<int, int>>& allErrorsPositions
    );

    /**
     * @brief Check if a position has been manually cleared by the user
     */
    static bool isPositionCleared(
        const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
        size_t start,
        size_t end
    );
};
