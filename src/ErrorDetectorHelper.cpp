#include "ErrorDetectorHelper.h"
#include "GermanTextAnalyzer.h"
#include "EnglishTextAnalyzer.h"
#include "MainWindow.h"
#include "RE2RegexHelper.h"
#include "TimerHelper.h"
#include <algorithm>
#include <iostream>

void ErrorDetectorHelper::findUnnumberedWords(
    const std::wstring& fullText,
    const re2::RE2& wordRegex,
    const std::unordered_set<std::wstring>& multiWordBaseStems,
    const std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions,
    const std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
    const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
    wxRichTextCtrl* textBox,
    const wxTextAttr& warningStyle,
    std::vector<std::pair<int, int>>& noNumberPositions,
    std::vector<std::pair<int, int>>& allErrorsPositions
) {
    // Collect start positions of all valid references
    std::unordered_set<size_t> validStarts;
    for (const auto &[stem, positions] : stemToPositions) {
        for (const auto [start, len] : positions) {
            validStarts.insert(start);
        }
    }

    // Helper to check if a position is followed by whitespace + number
    auto isFollowedByNumber = [&fullText](size_t wordEnd) -> bool {
        // Skip whitespace after the word
        size_t pos = wordEnd;
        while (pos < fullText.length() && std::iswspace(fullText[pos])) {
            pos++;
        }
        // Check if next character is a digit
        return pos < fullText.length() && std::iswdigit(fullText[pos]);
    };

    // Collect all words NOT followed by numbers
    struct WordMatch {
        std::wstring word;
        size_t position;
        size_t length;
    };
    std::vector<WordMatch> wordsWithoutNumbers;
    wordsWithoutNumbers.reserve(1000);

    {
        RE2RegexHelper::MatchIterator iter(fullText, wordRegex);
        while (iter.hasNext()) {
            auto match = iter.next();
            size_t pos = match.position;
            size_t len = match.length;

            // Skip if already part of a valid reference
            if (validStarts.count(pos)) {
                continue;
            }

            // Skip if followed by a number
            if (isFollowedByNumber(pos + len)) {
                continue;
            }

            wordsWithoutNumbers.push_back({std::move(match.groups[0]), pos, len});
        }
    }

    // Check for two-word patterns (consecutive words without numbers)
    for (size_t i = 0; i + 1 < wordsWithoutNumbers.size(); ++i) {
        const auto& word1Match = wordsWithoutNumbers[i];
        const auto& word2Match = wordsWithoutNumbers[i + 1];

        // Check if these words are actually adjacent in the text
        size_t expectedGap = word2Match.position - (word1Match.position + word1Match.length);
        if (expectedGap > 10) {
            continue; // Too far apart
        }

        std::wstring word1 = word1Match.word;
        std::wstring word2 = word2Match.word;

        // Only flag if this is a known multi-word combination
        if (MainWindow::isCurrentMultiWordBase(word2, multiWordBaseStems)) {
            StemVector stemVec = MainWindow::createCurrentMultiWordStemVector(word1, word2);

            if (stemToBz.count(stemVec)) {
                size_t startPos = word1Match.position;
                size_t endPos = word2Match.position + word2Match.length;
                if (!isPositionCleared(clearedTextPositions, startPos, endPos)) {
                    noNumberPositions.emplace_back(startPos, endPos);
                    allErrorsPositions.emplace_back(startPos, endPos);
                    textBox->SetStyle(startPos, endPos, warningStyle);
                }
            }
        }
    }

    // Check for single words without numbers
    for (const auto& wordMatch : wordsWithoutNumbers) {
        StemVector stemVec = MainWindow::createCurrentStemVector(wordMatch.word);

        // Check if this stem is known from valid references
        if (stemToBz.count(stemVec)) {
            size_t start = wordMatch.position;
            size_t end = wordMatch.position + wordMatch.length;
            if (!isPositionCleared(clearedTextPositions, start, end)) {
                noNumberPositions.emplace_back(start, end);
                allErrorsPositions.emplace_back(start, end);
                textBox->SetStyle(start, end, warningStyle);
            }
        }
    }
}

void ErrorDetectorHelper::checkArticleUsage(
    const std::wstring& fullText,
    const std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions,
    const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
    wxRichTextCtrl* textBox,
    const wxTextAttr& articleWarningStyle,
    std::vector<std::pair<int, int>>& wrongArticlePositions,
    std::vector<std::pair<int, int>>& allErrorsPositions
) {
    struct OccurrenceInfo {
        size_t position;
        size_t length;
        StemVector stem;
    };

    std::vector<OccurrenceInfo> allOccurrences;

    // Reserve capacity to reduce allocations
    size_t totalOccurrences = 0;
    for (const auto &[stem, positions] : stemToPositions) {
        totalOccurrences += positions.size();
    }
    allOccurrences.reserve(totalOccurrences);

    for (const auto &[stem, positions] : stemToPositions) {
        for (const auto [start, len] : positions) {
            allOccurrences.push_back({start, len, stem});
        }
    }

    // Sort by position
    std::sort(allOccurrences.begin(), allOccurrences.end(),
              [](const OccurrenceInfo &a, const OccurrenceInfo &b) {
                  return a.position < b.position;
              });

    // Track which stems we've seen
    std::unordered_set<StemVector, StemVectorHash> seenStems;

    for (const auto &occ : allOccurrences) {
        auto [precedingWord, precedingPos] =
            MainWindow::findCurrentPrecedingWord(fullText, occ.position);

        if (precedingWord.empty()) {
            seenStems.insert(occ.stem);
            continue;
        }

        bool isFirstOccurrence = (seenStems.count(occ.stem) == 0);
        size_t articleEnd = precedingPos + precedingWord.length();

        if (isFirstOccurrence) {
            // First occurrence: should not be definite article
            if (MainWindow::isCurrentDefiniteArticle(precedingWord)) {
                if (!isPositionCleared(clearedTextPositions, precedingPos, articleEnd)) {
                    wrongArticlePositions.emplace_back(precedingPos, articleEnd);
                    allErrorsPositions.emplace_back(precedingPos, articleEnd);
                    textBox->SetStyle(precedingPos, articleEnd, articleWarningStyle);
                }
            }
            seenStems.insert(occ.stem);
        } else {
            // Subsequent occurrence: should have definite article
            if (MainWindow::isCurrentIndefiniteArticle(precedingWord)) {
                if (!isPositionCleared(clearedTextPositions, precedingPos, articleEnd)) {
                    wrongArticlePositions.emplace_back(precedingPos, articleEnd);
                    allErrorsPositions.emplace_back(precedingPos, articleEnd);
                    textBox->SetStyle(precedingPos, articleEnd, articleWarningStyle);
                }
            }
        }
    }
}

bool ErrorDetectorHelper::isUniquelyAssigned(
    const std::wstring& bz,
    const std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>& bzToStems,
    const std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
    const std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>& bzToPositions,
    const std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash>& stemToPositions,
    const std::unordered_set<std::wstring>& clearedErrors,
    const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
    wxRichTextCtrl* textBox,
    const wxTextAttr& conflictStyle,
    std::vector<std::pair<int, int>>& wrongTermBzPositions,
    std::vector<std::pair<int, int>>& allErrorsPositions
) {
    // Check if this error has been cleared by user
    if (clearedErrors.count(bz) > 0) {
        return true; // Treat as no error
    }

    const auto &stems = bzToStems.at(bz);

    // Check if multiple different stems are assigned to this BZ
    if (stems.size() > 1) {
        // Highlight all occurrences of this BZ
        const auto &positions = bzToPositions.at(bz);
        for (const auto i : positions) {
            size_t start = i.first;
            size_t len = i.second;
            if (!isPositionCleared(clearedTextPositions, start, start + len)) {
                wrongTermBzPositions.emplace_back(start, start + len);
                allErrorsPositions.emplace_back(start, start + len);
                textBox->SetStyle(start, start + len, conflictStyle);
            }
        }
        return false;
    }

    // Check if the stem is also used with other BZs
    for (const auto &stem : stems) {
        if (stemToBz.at(stem).size() > 1) {
            // This stem maps to multiple BZs - highlight occurrences
            const auto &positions = stemToPositions.at(stem);
            for (const auto i : positions) {
                size_t start = i.first;
                size_t len = i.second;

                // Avoid duplicates in the merged list
                auto pos_pair = std::make_pair(static_cast<int>(start),
                                               static_cast<int>(start + len));
                if (std::find(wrongTermBzPositions.begin(),
                              wrongTermBzPositions.end(),
                              pos_pair) == wrongTermBzPositions.end() &&
                    !isPositionCleared(clearedTextPositions, start, start + len)) {
                    wrongTermBzPositions.emplace_back(start, start + len);
                    allErrorsPositions.emplace_back(start, start + len);
                    textBox->SetStyle(start, start + len, conflictStyle);
                }
            }
            return false;
        }
    }

    return true;
}

bool ErrorDetectorHelper::isPositionCleared(
    const std::set<std::pair<size_t, size_t>>& clearedTextPositions,
    size_t start,
    size_t end
) {
    return clearedTextPositions.count({start, end}) > 0;
}
