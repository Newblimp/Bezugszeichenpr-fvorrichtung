#pragma once
#include "german_stem_umlaut_preserving.h"
#include "utils_core.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <locale>

/**
 * @brief German language text analysis utilities
 *
 * Handles German-specific text processing including:
 * - Stemming (using Oleander library with caching)
 * - Article detection (definite/indefinite)
 * - Word extraction and validation
 * - Proper German locale handling for case conversion
 */
class GermanTextAnalyzer {
public:
    GermanTextAnalyzer();

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

    // Text utilities
    static std::pair<std::wstring, size_t> findPrecedingWord(const std::wstring& text, size_t pos);

    // Cache management (for diagnostics)
    size_t getCacheSize() const { return m_stemCache.size(); }
    void clearCache() { m_stemCache.clear(); }

private:
    stemming::german_stem_umlaut_preserving<> m_germanStemmer;
    
    // German locale for proper character handling (ä, ö, ü, ß)
    std::locale m_germanLocale;
    const std::ctype<wchar_t>* m_ctypeFacet;
    
    // Cache for stemming results to avoid repeated expensive operations
    // Key: normalized word (first char lowercased), Value: stemmed word
    mutable std::unordered_map<std::wstring, std::wstring> m_stemCache;

    // Static sets for fast article lookup
    static const std::unordered_set<std::wstring> s_indefiniteArticles;
    static const std::unordered_set<std::wstring> s_definiteArticles;
};
