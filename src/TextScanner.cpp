#include "TextScanner.h"
#include "GermanTextAnalyzer.h"
#include "EnglishTextAnalyzer.h"
#include "MainWindow.h"
#include "TimerHelper.h"
#include <iostream>

void TextScanner::scanText(
    const std::wstring& fullText,
    const re2::RE2& singleWordRegex,
    const re2::RE2& twoWordRegex,
    const std::unordered_set<std::wstring>& multiWordBaseStems,
    const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
    std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
    std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
    std::unordered_map<std::wstring, std::unordered_set<std::wstring>>& bzToOriginalWords,
    std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
    std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions
) {
    // Track matched positions to avoid duplicate processing
    std::vector<std::pair<size_t, size_t>> matchedRanges;

    // First pass: scan for two-word patterns
    Timer t_twoWordScan;
    scanTwoWordPatterns(fullText, twoWordRegex, multiWordBaseStems, clearedTextPositions,
                        matchedRanges, bzToStems, stemToBz, bzToOriginalWords,
                        bzToPositions, stemToPositions);
    std::cout << "Time for two word scan: " << t_twoWordScan.elapsed() << " milliseconds\n";

    // Second pass: scan for single-word patterns
    Timer t_oneWordScan;
    scanSingleWordPatterns(fullText, singleWordRegex, clearedTextPositions,
                           matchedRanges, bzToStems, stemToBz, bzToOriginalWords,
                           bzToPositions, stemToPositions);
    std::cout << "Time for one word scan: " << t_oneWordScan.elapsed() << " milliseconds\n";
}

void TextScanner::scanTwoWordPatterns(
    const std::wstring& fullText,
    const re2::RE2& twoWordRegex,
    const std::unordered_set<std::wstring>& multiWordBaseStems,
    const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
    std::vector<std::pair<size_t, size_t>>& matchedRanges,
    std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
    std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
    std::unordered_map<std::wstring, std::unordered_set<std::wstring>>& bzToOriginalWords,
    std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
    std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions
) {
    RE2RegexHelper::MatchIterator iter(fullText, twoWordRegex);

    while (iter.hasNext()) {
        auto match = iter.next();
        size_t pos = match.position;
        size_t len = match.length;
        size_t endPos = pos + len;

        std::wstring word1 = match[1];
        std::wstring word2 = match[2];
        std::wstring bz = match[3];

        // Check if word2's stem is marked for multi-word matching
        if (MainWindow::isCurrentMultiWordBase(word2, multiWordBaseStems)) {
            if (!overlapsExisting(matchedRanges, pos, endPos) &&
                !clearedTextPositions.count({pos, endPos})) {
                matchedRanges.emplace_back(pos, endPos);

                // Build original phrase first (before moving words)
                std::wstring originalPhrase;
                originalPhrase.reserve(word1.length() + 1 + word2.length());
                originalPhrase = word1 + L" " + word2;

                // Create stem vector with both words
                StemVector stemVec =
                    MainWindow::createCurrentMultiWordStemVector(std::move(word1), std::move(word2));

                // Store mappings
                bzToStems[bz].insert(stemVec);
                stemToBz[stemVec].insert(bz);
                bzToOriginalWords[bz].insert(originalPhrase);

                // Track positions
                bzToPositions[bz].push_back({pos, len});
                stemToPositions[stemVec].push_back({pos, len});
            }
        }
    }
}

void TextScanner::scanSingleWordPatterns(
    const std::wstring& fullText,
    const re2::RE2& singleWordRegex,
    const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
    std::vector<std::pair<size_t, size_t>>& matchedRanges,
    std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
    std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
    std::unordered_map<std::wstring, std::unordered_set<std::wstring>>& bzToOriginalWords,
    std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
    std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions
) {
    RE2RegexHelper::MatchIterator iter(fullText, singleWordRegex);

    while (iter.hasNext()) {
        auto match = iter.next();
        if (MainWindow::isCurrentIgnoredWord(match[1])) {
            continue; // Skip ignored words
        }
        size_t pos = match.position;
        size_t len = match.length;
        size_t endPos = pos + len;

        if (!overlapsExisting(matchedRanges, pos, endPos) &&
            !clearedTextPositions.count({pos, endPos})) {
            matchedRanges.emplace_back(pos, endPos);

            std::wstring word = match[1];
            std::wstring originalWord = word;  // Keep copy for storage
            std::wstring bz = match[2];

            // Create single-element stem vector
            StemVector stemVec = MainWindow::createCurrentStemVector(std::move(word));

            // Store mappings
            bzToStems[bz].insert(stemVec);
            stemToBz[stemVec].insert(bz);
            bzToOriginalWords[bz].insert(std::move(originalWord));

            // Track positions
            bzToPositions[bz].push_back({pos, len});
            stemToPositions[stemVec].push_back({pos, len});
        }
    }
}

bool TextScanner::overlapsExisting(
    const std::vector<std::pair<size_t, size_t>>& matchedRanges,
    size_t start,
    size_t end
) {
    for (const auto &range : matchedRanges) {
        if (!(end <= range.first || start >= range.second)) {
            return true;
        }
    }
    return false;
}
