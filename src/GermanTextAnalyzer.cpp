#include "GermanTextAnalyzer.h"
#include <algorithm>
#include <locale>

// Static member initialization
const std::unordered_set<std::wstring> GermanTextAnalyzer::s_indefiniteArticles = {
    L"ein", L"eine", L"eines", L"einen", L"einer", L"einem"
};

const std::unordered_set<std::wstring> GermanTextAnalyzer::s_definiteArticles = {
    L"der", L"die", L"das", L"den", L"dem", L"des"
};

GermanTextAnalyzer::GermanTextAnalyzer() = default;

void GermanTextAnalyzer::stemWord(std::wstring& word) {
    if (word.empty())
        return;
    word[0] = std::tolower(word[0]);
    m_germanStemmer(word);
}

StemVector GermanTextAnalyzer::createStemVector(const std::wstring& word) {
    std::wstring stem = word;
    stemWord(stem);
    return {stem};
}

StemVector GermanTextAnalyzer::createMultiWordStemVector(const std::wstring& firstWord,
                                                         const std::wstring& secondWord) {
    std::wstring stem1 = firstWord;
    std::wstring stem2 = secondWord;
    stemWord(stem1);
    stemWord(stem2);
    return {stem1, stem2};
}

bool GermanTextAnalyzer::isMultiWordBase(const std::wstring& word,
                                         const std::unordered_set<std::wstring>& multiWordBaseStems) {
    std::wstring stem = word;
    stemWord(stem);
    return multiWordBaseStems.count(stem) > 0;
}

bool GermanTextAnalyzer::isIndefiniteArticle(const std::wstring& word) {
    std::wstring lower = word;
    for (auto& c : lower) {
        c = std::tolower(c);
    }
    return s_indefiniteArticles.count(lower) > 0;
}

bool GermanTextAnalyzer::isDefiniteArticle(const std::wstring& word) {
    std::wstring lower = word;
    for (auto& c : lower) {
        c = std::tolower(c);
    }
    return s_definiteArticles.count(lower) > 0;
}

std::pair<std::wstring, size_t> GermanTextAnalyzer::findPrecedingWord(const std::wstring& text,
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
