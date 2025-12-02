#pragma once

#include "utils.h"
#include "RE2RegexHelper.h"
#include <re2/re2.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <set>

/**
 * @brief Handles text scanning and pattern matching for reference numbers
 *
 * This class is responsible for scanning text to identify:
 * - Single-word patterns (e.g., "Lager 10")
 * - Two-word patterns (e.g., "erstes Lager 10")
 * - Building the data structures that map reference numbers to terms
 */
class TextScanner {
public:
    /**
     * @brief Scan text and populate data structures
     * @param fullText The complete text to scan
     * @param singleWordRegex Regex for single-word patterns
     * @param twoWordRegex Regex for two-word patterns
     * @param multiWordBaseStems Set of stems that should trigger multi-word matching
     * @param clearedTextPositions Positions that have been manually cleared by user
     * @param bzToStems Output: mapping from reference numbers to stem vectors
     * @param stemToBz Output: reverse mapping from stem vectors to reference numbers
     * @param bzToOriginalWords Output: mapping from reference numbers to original words
     * @param bzToPositions Output: mapping from reference numbers to text positions
     * @param stemToPositions Output: mapping from stem vectors to text positions
     */
    static void scanText(
        const std::wstring& fullText,
        const re2::RE2& singleWordRegex,
        const re2::RE2& twoWordRegex,
        const std::unordered_set<std::wstring>& multiWordBaseStems,
        const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
        std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
        std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
        std::unordered_map<std::wstring, std::unordered_set<std::wstring>>& bzToOriginalWords,
        std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
        std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions
    );

private:
    /**
     * @brief Scan for two-word patterns
     */
    static void scanTwoWordPatterns(
        const std::wstring& fullText,
        const re2::RE2& twoWordRegex,
        const std::unordered_set<std::wstring>& multiWordBaseStems,
        const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
        std::vector<std::pair<size_t, size_t>>& matchedRanges,
        std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
        std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
        std::unordered_map<std::wstring, std::unordered_set<std::wstring>>& bzToOriginalWords,
        std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
        std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions
    );

    /**
     * @brief Scan for single-word patterns
     */
    static void scanSingleWordPatterns(
        const std::wstring& fullText,
        const re2::RE2& singleWordRegex,
        const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
        std::vector<std::pair<size_t, size_t>>& matchedRanges,
        std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
        std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
        std::unordered_map<std::wstring, std::unordered_set<std::wstring>>& bzToOriginalWords,
        std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
        std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions
    );

    /**
     * @brief Check if a range overlaps with any existing matched ranges
     */
    static bool overlapsExisting(
        const std::vector<std::pair<size_t, size_t>>& matchedRanges,
        size_t start,
        size_t end
    );
};
