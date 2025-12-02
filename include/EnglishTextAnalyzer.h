#pragma once
#include "english_stem.h"
#include "utils_core.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <locale>

/**
 * @brief English language text analysis utilities
 *
 * Handles English-specific text processing including:
 * - Stemming (using Oleander library with caching)
 * - Article detection (definite/indefinite)
 * - Word extraction and validation
 * - Proper English locale handling for case conversion
 */
class EnglishTextAnalyzer {
public:
    EnglishTextAnalyzer();

    // Stemming operations (now with caching)
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

    // Word filtering
    static bool isIgnoredWord(const std::wstring& word);

    // Text utilities
    static std::pair<std::wstring, size_t> findPrecedingWord(const std::wstring& text, size_t pos);

    // Cache management (for diagnostics)
    size_t getCacheSize() const { return m_stemCache.size(); }
    void clearCache() { m_stemCache.clear(); }

private:
    stemming::english_stem<> m_englishStemmer;
    
    // English locale for proper character handling
    std::locale m_englishLocale;
    const std::ctype<wchar_t>* m_ctypeFacet;
    
    // Cache for stemming results to avoid repeated expensive operations
    mutable std::unordered_map<std::wstring, std::wstring> m_stemCache;

    // Static sets for fast article lookup
    static const std::unordered_set<std::wstring> s_indefiniteArticles;
    static const std::unordered_set<std::wstring> s_definiteArticles;
    
    // Static set for ignored words
    static const std::unordered_set<std::wstring> s_ignoredWords;
};
