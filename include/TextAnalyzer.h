#pragma once
#include "utils_core.h"
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * @brief Abstract base class for language-specific text analysis
 */
class TextAnalyzer {
public:
    virtual ~TextAnalyzer() = default;

    // Stemming operations
    virtual void stemWord(std::wstring& word) = 0;

    // Factory methods for StemVectors
    virtual StemVector createStemVector(std::wstring word) = 0;
    virtual StemVector createMultiWordStemVector(std::wstring firstWord, std::wstring secondWord) = 0;
    
    // Check if a word is a base for multi-word terms
    virtual bool isMultiWordBase(std::wstring word, const std::unordered_set<std::wstring>& multiWordBaseStems) = 0;

    // Article checking
    virtual bool isIndefiniteArticle(const std::wstring& word) const = 0;
    virtual bool isDefiniteArticle(const std::wstring& word) const = 0;

    // Word filtering
    virtual bool isIgnoredWord(const std::wstring& word) const = 0;

    // Common text utility (implementation identical across languages)
    std::pair<std::wstring, size_t> findPrecedingWord(const std::wstring& text, size_t pos) const;
    
    // Cache management
    virtual size_t getCacheSize() const = 0;
    virtual void clearCache() = 0;
};
