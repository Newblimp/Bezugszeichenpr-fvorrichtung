#pragma once
#include "german_stem.h"
#include "utils.h"
#include <string>
#include <unordered_set>
#include <utility>

/**
 * @brief German language text analysis utilities
 *
 * Handles German-specific text processing including:
 * - Stemming (using Oleander library)
 * - Article detection (definite/indefinite)
 * - Word extraction and validation
 */
class GermanTextAnalyzer {
public:
    GermanTextAnalyzer();

    // Stemming operations
    void stemWord(std::wstring& word);
    
    // Optimized: accepts by value to enable move semantics
    StemVector createStemVector(std::wstring word);
    StemVector createMultiWordStemVector(std::wstring firstWord,
                                         std::wstring secondWord);
    bool isMultiWordBase(std::wstring word,
                        const std::unordered_set<std::wstring>& multiWordBaseStems);

    // Article checking
    static bool isIndefiniteArticle(const std::wstring& word);
    static bool isDefiniteArticle(const std::wstring& word);

    // Text utilities
    static std::pair<std::wstring, size_t> findPrecedingWord(const std::wstring& text, size_t pos);

private:
    stemming::german_stem<> m_germanStemmer;

    // Static sets for fast article lookup
    static const std::unordered_set<std::wstring> s_indefiniteArticles;
    static const std::unordered_set<std::wstring> s_definiteArticles;
};
