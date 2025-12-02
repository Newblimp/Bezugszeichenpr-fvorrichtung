#include "EnglishTextAnalyzer.h"
#include <algorithm>
#include <locale>

// Static member initialization - English articles
const std::unordered_set<std::wstring> EnglishTextAnalyzer::s_indefiniteArticles = {
    L"a", L"an"
};

const std::unordered_set<std::wstring> EnglishTextAnalyzer::s_definiteArticles = {
    L"the"
};

EnglishTextAnalyzer::EnglishTextAnalyzer() {
    // Initialize English locale for proper character handling
    try {
        m_englishLocale = std::locale("en_US.UTF-8");
    } catch (const std::runtime_error&) {
        // Fallback to default locale if English locale not available
        m_englishLocale = std::locale("");
    }
    m_ctypeFacet = &std::use_facet<std::ctype<wchar_t>>(m_englishLocale);
}

void EnglishTextAnalyzer::stemWord(std::wstring& word) {
    if (word.empty())
        return;
    
    // Normalize first character to lowercase using English locale
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
    m_englishStemmer(word);
    
    // Store in cache for future lookups
    m_stemCache[std::move(original)] = word;
}

// Optimized: accept by value and move, avoiding defensive copy
StemVector EnglishTextAnalyzer::createStemVector(std::wstring word) {
    stemWord(word);
    return {std::move(word)};
}

// Optimized: accept by value and move both words
StemVector EnglishTextAnalyzer::createMultiWordStemVector(std::wstring firstWord,
                                                          std::wstring secondWord) {
    stemWord(firstWord);
    stemWord(secondWord);
    return {std::move(firstWord), std::move(secondWord)};
}

// Optimized: accept by value to allow moving from temporaries
bool EnglishTextAnalyzer::isMultiWordBase(std::wstring word,
                                          const std::unordered_set<std::wstring>& multiWordBaseStems) {
    stemWord(word);
    return multiWordBaseStems.count(word) > 0;
}

// Optimized: use manual lowercase loop with early exit
bool EnglishTextAnalyzer::isIndefiniteArticle(const std::wstring& word) {
    // Fast path: check length first (English: "a" or "an")
    if (word.length() < 1 || word.length() > 2) {
        return false;
    }
    
    // Create lowercase version
    std::wstring lower;
    lower.reserve(word.length());
    for (wchar_t c : word) {
        lower.push_back(std::tolower(c));
    }
    return s_indefiniteArticles.count(lower) > 0;
}

// Optimized: use manual lowercase loop with early exit
bool EnglishTextAnalyzer::isDefiniteArticle(const std::wstring& word) {
    // Fast path: check length first (English: "the" is exactly 3 chars)
    if (word.length() != 3) {
        return false;
    }
    
    // Create lowercase version
    std::wstring lower;
    lower.reserve(3);
    for (wchar_t c : word) {
        lower.push_back(std::tolower(c));
    }
    return s_definiteArticles.count(lower) > 0;
}

std::pair<std::wstring, size_t> EnglishTextAnalyzer::findPrecedingWord(const std::wstring& text,
                                                                        size_t pos) {
    if (pos == 0) {
        return {L"", 0};
    }

    // Skip whitespace backwards
    size_t end = pos;
    while (end > 0 && std::iswspace(text[end - 1])) {
        --end;
    }

    if (end == 0) {
        return {L"", 0};
    }

    // Find the start of the word
    size_t start = end;
    while (start > 0 && std::iswalpha(text[start - 1])) {
        --start;
    }

    if (start == end) {
        return {L"", 0};
    }

    return {text.substr(start, end - start), start};
}

// Static member initialization - ignored words
const std::unordered_set<std::wstring> EnglishTextAnalyzer::s_ignoredWords = {
    // Definite article
    L"the",
    // Indefinite articles
    L"a", L"an",
    // Figure references
    L"figure", L"figures"
};

bool EnglishTextAnalyzer::isIgnoredWord(const std::wstring& word) {
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
