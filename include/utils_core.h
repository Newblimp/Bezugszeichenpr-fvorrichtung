#pragma once
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

// Type alias for a stemmed term (vector of stemmed words)
// Single-word term: {"vorricht"}
// Multi-word term: {"zweit", "lager"}
using StemVector = std::vector<std::wstring>;

// Hash function for StemVector to use in unordered containers
struct StemVectorHash {
    size_t operator()(const StemVector& vec) const {
        size_t seed = vec.size();
        std::hash<std::wstring> hasher;
        for (const auto& stem : vec) {
            // Combine hashes using a technique similar to boost::hash_combine
            seed ^= hasher(stem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

// Comparator for BZ strings - sorts numerically, then by suffix
struct BZComparatorForMap {
    bool operator()(const std::wstring& a, const std::wstring& b) const {
        if (a == b) {
            return false;
        }

        int int1 = std::stoi(a);
        int int2 = std::stoi(b);

        if (int1 != int2) {
            return int1 < int2;
        }

        // Same numeric value, compare by length then lexicographically
        if (a.length() != b.length()) {
            return a.length() < b.length();
        }
        return a < b;
    }
};

// Comparator for StemVector - for use in ordered containers
struct StemVectorComparator {
    bool operator()(const StemVector& a, const StemVector& b) const {
        if (a.size() != b.size()) {
            return a.size() < b.size();
        }
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) {
                return a[i] < b[i];
            }
        }
        return false; // Equal
    }
};

// Convert a single StemVector to a string representation (for debugging/display)
std::wstring stemVectorToString(const StemVector& stems);

// Collect all unique stems from a stem-to-BZ mapping into a set
void collectAllStems(
    const std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
    std::unordered_set<StemVector, StemVectorHash>& allStems);

// Build regex alternation pattern from a set of strings
void appendAlternationPattern(
    const std::unordered_set<std::wstring>& strings,
    std::wstring& regexString);
