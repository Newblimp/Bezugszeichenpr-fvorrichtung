#include "GermanTextAnalyzer.h"
#include <algorithm>
#include <locale>
#include <iostream>

// Static member initialization
const std::unordered_set<std::wstring> GermanTextAnalyzer::s_indefiniteArticles = {
    L"ein", L"eine", L"eines", L"einen", L"einer", L"einem"
};

const std::unordered_set<std::wstring> GermanTextAnalyzer::s_definiteArticles = {
    L"der", L"die", L"das", L"den", L"dem", L"des"
};

GermanTextAnalyzer::GermanTextAnalyzer() {
    // Initialize German locale for proper character handling (ä, ö, ü, ß)
    try {
        m_germanLocale = std::locale("de_DE.UTF-8");
    } catch (const std::runtime_error&) {
        // Fallback to default locale if German locale not available
        m_germanLocale = std::locale("");
    }
    m_ctypeFacet = &std::use_facet<std::ctype<wchar_t>>(m_germanLocale);
}


void GermanTextAnalyzer::stemWord(std::wstring& word) {
    if (word.empty())
        return;
    
    // Normalize first character to lowercase using German locale (proper handling of Ä, Ö, Ü, etc.)
    std::transform(word.begin(), word.end(), word.begin(),
                   [this](wchar_t c) { return m_ctypeFacet->tolower(c); });
    
    // Check cache first
    auto it = m_stemCache.find(word);
    if (it != m_stemCache.end()) {
        // Cache hit - use cached result
        word = it->second;
        return;
    }
    
    // Cache miss - perform expensive stemming operation
    std::wstring original = word;
    m_germanStemmer(word);
    
    // Store in cache for future lookups
    m_stemCache[std::move(original)] = word;
}

// Optimized: accept by value and move, avoiding defensive copy
StemVector GermanTextAnalyzer::createStemVector(std::wstring word) {
    stemWord(word);
    return {std::move(word)};
}

// Optimized: accept by value and move both words
StemVector GermanTextAnalyzer::createMultiWordStemVector(std::wstring firstWord,
                                                         std::wstring secondWord) {
    stemWord(firstWord);
    stemWord(secondWord);
    return {std::move(firstWord), std::move(secondWord)};
}

// Optimized: accept by value to allow moving from temporaries
bool GermanTextAnalyzer::isMultiWordBase(std::wstring word,
                                         const std::unordered_set<std::wstring>& multiWordBaseStems) {
    stemWord(word);
    return multiWordBaseStems.count(word) > 0;
}

// Optimized: use manual lowercase loop with early exit
bool GermanTextAnalyzer::isIndefiniteArticle(const std::wstring& word) const {
    // Fast path: check length first
    if (word.length() < 3 || word.length() > 6) {
        return false;
    }
    
    // Create lowercase version only if length is in valid range
    std::wstring lower;
    lower.reserve(word.length());
    for (wchar_t c : word) {
        lower.push_back(std::tolower(c));
    }
    return s_indefiniteArticles.count(lower) > 0;
}

// Optimized: use manual lowercase loop with early exit
bool GermanTextAnalyzer::isDefiniteArticle(const std::wstring& word) const {
    // Fast path: check length first (all definite articles are exactly 3 chars)
    if (word.length() != 3) {
        return false;
    }
    
    // Create lowercase version only if length is valid
    std::wstring lower;
    lower.reserve(3);
    for (wchar_t c : word) {
        lower.push_back(std::tolower(c));
    }
    return s_definiteArticles.count(lower) > 0;
}

// Static member initialization - ignored words
const std::unordered_set<std::wstring> GermanTextAnalyzer::s_ignoredWords = {
    // Definite articles
    L"der", L"die", L"das", L"den", L"dem", L"des",
    // Indefinite articles
    L"ein", L"eine", L"eines", L"einen", L"einer", L"einem",
    // Figure references
    L"figur", L"figuren",
    // conjunctions
    L"und", L"oder", L"mit"
};

bool GermanTextAnalyzer::isIgnoredWord(const std::wstring& word) const {
    // Words shorter than 3 characters should be ignored
    if (word.length() < 3) {
        return true;
    }
    
    // Create lowercase version
    std::wstring lower;
    lower.reserve(word.length());
    for (wchar_t c : word) {
        lower.push_back(std::tolower(c));
    }
    return s_ignoredWords.count(lower) > 0;
}
