#pragma once

#include "utils.h"
#include "RE2RegexHelper.h"
#include "TextAnalyzer.h"
#include "AnalysisContext.h"
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
     * @param analyzer The language-specific text analyzer to use
     * @param singleWordRegex Regex for single-word patterns
     * @param twoWordRegex Regex for two-word patterns
     * @param ctx Scanning context and output database
     */
    static void scanText(
        const std::wstring& fullText,
        TextAnalyzer& analyzer,
        const re2::RE2& singleWordRegex,
        const re2::RE2& twoWordRegex,
        AnalysisContext& ctx
    );

private:
    /**
     * @brief Scan for two-word patterns
     */
    static void scanTwoWordPatterns(
        const std::wstring& fullText,
        TextAnalyzer& analyzer,
        const re2::RE2& twoWordRegex,
        AnalysisContext& ctx,
        std::vector<std::pair<size_t, size_t>>& matchedRanges
    );

    /**
     * @brief Scan for single-word patterns
     */
    static void scanSingleWordPatterns(
        const std::wstring& fullText,
        TextAnalyzer& analyzer,
        const re2::RE2& singleWordRegex,
        AnalysisContext& ctx,
        std::vector<std::pair<size_t, size_t>>& matchedRanges
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
