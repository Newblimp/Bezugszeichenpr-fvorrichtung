#pragma once
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cwctype>

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
        if (a.empty() && b.empty()) {
            return false;
        }

        if (a == b) {
            return false;
        }

        // Check if first character is numeric before calling std::stoi
        bool a_starts_with_digit = !a.empty() && std::iswdigit(a[0]);
        bool b_starts_with_digit = !b.empty() && std::iswdigit(b[0]);

        // If one starts with a digit and the other doesn't, digit-starting comes first
        if (a_starts_with_digit && !b_starts_with_digit) {
            return false;
        }
        if (!a_starts_with_digit && b_starts_with_digit) {
            return true;
        }

        // Both start with digits - compare numerically
        if (a_starts_with_digit && b_starts_with_digit) {
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

        // Neither starts with a digit - compare lexicographically
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
