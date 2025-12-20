#pragma once
#include "TextAnalyzer.h"
#include "german_stem.h"
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
class GermanTextAnalyzer : public TextAnalyzer {
public:
    GermanTextAnalyzer();

    // Stemming operations (now with caching)
    void stemWord(std::wstring& word) override;

    // Optimized: accepts by value to enable move semantics
    StemVector createStemVector(std::wstring word) override;
    StemVector createMultiWordStemVector(std::wstring firstWord,
                                         std::wstring secondWord) override;
    bool isMultiWordBase(std::wstring word,
                        const std::unordered_set<std::wstring>& multiWordBaseStems) override;

    // Article checking
    bool isIndefiniteArticle(const std::wstring& word) const override;
    bool isDefiniteArticle(const std::wstring& word) const override;

    // Word filtering
    bool isIgnoredWord(const std::wstring& word) const override;

    // Cache management (for diagnostics)
    size_t getCacheSize() const override { return m_stemCache.size(); }
    void clearCache() override { m_stemCache.clear(); }

private:
    stemming::german_stem<> m_germanStemmer;
    
    // German locale for proper character handling (ä, ö, ü, ß)
    std::locale m_germanLocale;
    const std::ctype<wchar_t>* m_ctypeFacet;
    
    // Cache for stemming results to avoid repeated expensive operations
    // Key: normalized word (first char lowercased), Value: stemmed word
    mutable std::unordered_map<std::wstring, std::wstring> m_stemCache;

    // Static sets for fast article lookup
    static const std::unordered_set<std::wstring> s_indefiniteArticles;
    static const std::unordered_set<std::wstring> s_definiteArticles;
    
    // Static set for ignored words
    static const std::unordered_set<std::wstring> s_ignoredWords;
};
