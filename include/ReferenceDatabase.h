#pragma once

#include "utils_core.h"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <set>

/**
 * @brief Consolidates reference number and term mapping data
 */
struct ReferenceDatabase {
    // Main data structure: BZ -> set of StemVectors
    // Example: "10" -> {{"lager"}, {"zweit", "lager"}}
    std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap> bzToStems;

    // Reverse mapping: StemVector -> set of BZs
    // Example: {"zweit", "lager"} -> {"12"}
    std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash> stemToBz;

    // Original (unstemmed) words for display
    // BZ -> set of original word strings
    std::unordered_map<std::wstring, std::unordered_set<std::wstring>> bzToOriginalWords;

    // Position tracking for highlighting and navigation
    // BZ -> list of (start, length) pairs
    std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>> bzToPositions;

    // StemVector -> list of (start, length) pairs
    std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>, StemVectorHash> stemToPositions;

    // Cache of first occurrence words for display
    std::unordered_map<StemVector, std::wstring, StemVectorHash> stemToFirstWord;

    void clear() {
        bzToStems.clear();
        stemToBz.clear();
        bzToOriginalWords.clear();
        bzToPositions.clear();
        stemToPositions.clear();
        stemToFirstWord.clear();
    }
};
