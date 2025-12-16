#pragma once

#include "utils_core.h"
#include "RE2RegexHelper.h"
#include <re2/re2.h>
#include <unordered_set>
#include <unordered_map>
#include <string>

/**
 * @brief Detects ordinal prefix patterns (first/second) for automatic multi-word term detection
 *
 * This class analyzes text to identify base words that appear with both "first" and "second"
 * ordinal prefixes (e.g., "first bearing" and "second bearing"). When both ordinals are found
 * for the same base word, that word is automatically enabled for multi-word matching.
 *
 * Supports language-specific ordinal detection:
 * - English: "first", "second"
 * - German: "erste", "ersten", "zweite", "zweiten", etc. (all declensions)
 */
class OrdinalDetector {
public:
    enum class OrdinalType {
        FIRST,
        SECOND,
        THIRD,
        NONE
    };

    /**
     * @brief Detect base stems that appear with both "first" and "second" ordinals
     *
     * Scans the text for two-word patterns and identifies base words (second word)
     * that appear with both "first" and "second" ordinal prefixes (first word).
     *
     * @param fullText The complete text to analyze
     * @param twoWordRegex Regex for matching two-word patterns (word word number)
     * @param useGerman Whether to use German or English ordinal detection
     * @return Set of base stems that should enable multi-word mode
     */
    static std::unordered_set<std::wstring> detectOrdinalPatterns(
        const std::wstring& fullText,
        const re2::RE2& twoWordRegex,
        bool useGerman
    );

private:
    /**
     * @brief Check if a word is a German ordinal and return its type
     *
     * Checks all common declensions of German ordinals.
     *
     * @param word The word to check (case-insensitive)
     * @param outType Output parameter: the ordinal type if found
     * @return true if the word is a German ordinal
     */
    static bool isGermanOrdinal(const std::wstring& word, OrdinalType& outType);

    /**
     * @brief Check if a word is an English ordinal and return its type
     *
     * @param word The word to check (case-insensitive)
     * @param outType Output parameter: the ordinal type if found
     * @return true if the word is an English ordinal
     */
    static bool isEnglishOrdinal(const std::wstring& word, OrdinalType& outType);

    // Static ordinal word sets for German (all common declensions)
    static const std::unordered_set<std::wstring> s_germanFirstOrdinals;      // erste, ersten, erstes, erster
    static const std::unordered_set<std::wstring> s_germanSecondOrdinals;     // zweite, zweiten, zweites, zweiter
    static const std::unordered_set<std::wstring> s_germanThirdOrdinals;      // dritte, dritten, drittes, dritter

    // Static ordinal word sets for English
    static const std::unordered_set<std::wstring> s_englishFirstOrdinals;     // first
    static const std::unordered_set<std::wstring> s_englishSecondOrdinals;    // second
    static const std::unordered_set<std::wstring> s_englishThirdOrdinals;     // third
};
