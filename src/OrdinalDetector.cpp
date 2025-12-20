#include "OrdinalDetector.h"
#include <algorithm>
#include <iostream>
#include <cctype>

// German ordinal sets (all common declensions)
const std::unordered_set<std::wstring> OrdinalDetector::s_germanFirstOrdinals = {
    L"erste", L"ersten", L"erstes", L"erster",   // Nominative, accusative, etc.
};

const std::unordered_set<std::wstring> OrdinalDetector::s_germanSecondOrdinals = {
    L"zweite", L"zweiten", L"zweites", L"zweiter",
};

const std::unordered_set<std::wstring> OrdinalDetector::s_germanThirdOrdinals = {
    L"dritte", L"dritten", L"drittes", L"dritter",
};

// English ordinal sets
const std::unordered_set<std::wstring> OrdinalDetector::s_englishFirstOrdinals = {
    L"first"
};

const std::unordered_set<std::wstring> OrdinalDetector::s_englishSecondOrdinals = {
    L"second"
};

const std::unordered_set<std::wstring> OrdinalDetector::s_englishThirdOrdinals = {
    L"third"
};

// Helper function to convert string to lowercase for comparison
static std::wstring toLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](wchar_t c) { return std::tolower(static_cast<unsigned char>(c)); });
    return result;
}

bool OrdinalDetector::isGermanOrdinal(const std::wstring& word, OrdinalType& outType) {
    std::wstring lowerWord = toLower(word);

    if (s_germanFirstOrdinals.count(lowerWord) > 0) {
        outType = OrdinalType::FIRST;
        return true;
    }
    if (s_germanSecondOrdinals.count(lowerWord) > 0) {
        outType = OrdinalType::SECOND;
        return true;
    }
    if (s_germanThirdOrdinals.count(lowerWord) > 0) {
        outType = OrdinalType::THIRD;
        return true;
    }

    return false;
}

bool OrdinalDetector::isEnglishOrdinal(const std::wstring& word, OrdinalType& outType) {
    std::wstring lowerWord = toLower(word);

    if (s_englishFirstOrdinals.count(lowerWord) > 0) {
        outType = OrdinalType::FIRST;
        return true;
    }
    if (s_englishSecondOrdinals.count(lowerWord) > 0) {
        outType = OrdinalType::SECOND;
        return true;
    }
    if (s_englishThirdOrdinals.count(lowerWord) > 0) {
        outType = OrdinalType::THIRD;
        return true;
    }

    return false;
}

std::unordered_set<std::wstring> OrdinalDetector::detectOrdinalPatterns(
    const std::wstring& fullText,
    const re2::RE2& twoWordRegex,
    bool useGerman,
    TextAnalyzer& analyzer
) {
    // Track which ordinals are used with each base stem
    std::unordered_map<std::wstring, std::unordered_set<OrdinalType>> ordinalUsage;

    // Scan text for two-word patterns
    RE2RegexHelper::MatchIterator iter(fullText, twoWordRegex);

    while (iter.hasNext()) {
        auto match = iter.next();
        std::wstring word1 = match[1];  // Potential ordinal
        std::wstring word2 = match[2];  // Potential base word
        // std::wstring bz = match[3];     // Reference number (not used here)

        // Check if word1 is an ordinal (language-specific)
        OrdinalType ordinalType;
        bool isOrdinal = false;

        if (useGerman) {
            isOrdinal = isGermanOrdinal(word1, ordinalType);
        } else {
            isOrdinal = isEnglishOrdinal(word1, ordinalType);
        }

        if (isOrdinal) {
            // Create stem vector for word2 (the base word)
            StemVector word2StemVec = analyzer.createStemVector(word2);

            if (!word2StemVec.empty()) {
                // Use the base stem (last element of StemVector is the base word)
                std::wstring baseStem = word2StemVec.back();
                ordinalUsage[baseStem].insert(ordinalType);
            }
        }
    }

    // Find stems that have BOTH first AND second ordinals
    std::unordered_set<std::wstring> autoDetected;

    for (const auto& [baseStem, ordinalSet] : ordinalUsage) {
        bool hasFirst = ordinalSet.count(OrdinalType::FIRST) > 0;
        bool hasSecond = ordinalSet.count(OrdinalType::SECOND) > 0;

        if (hasFirst && hasSecond) {
            autoDetected.insert(baseStem);
            std::wcout << L"[OrdinalDetector] Auto-detected multi-word stem: " << baseStem << std::endl;
        }
    }

    return autoDetected;
}
