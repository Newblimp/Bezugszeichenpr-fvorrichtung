#pragma once
#include <string>

/**
 * @brief Shared regex patterns for reference number scanning
 * 
 * These patterns are used by MainWindow for text scanning and by tests
 * to ensure the regex behavior matches expectations.
 * 
 * All patterns require words to be at least 3 characters long to filter
 * out short articles and prepositions.
 * 
 * All patterns use case-insensitive matching ((?i) prefix).
 */
namespace RegexPatterns {
    // Single word + number: captures (word)(number)
    // Example: "Lager 10" or "lager 10" -> captures ("Lager"/"lager", "10")
    // Pattern: word (3+ chars) followed by whitespace and number
    constexpr const char* SINGLE_WORD_PATTERN = R"((?i)(\p{L}{3,})\s+(\b\d+[a-zA-Z']*\b))";

    // Two words + number: captures (word1)(word2)(number)
    // Example: "erstes Lager 10" -> captures ("erstes", "Lager", "10")
    // Pattern: word (3+ chars) followed by word (3+ chars) followed by number
    constexpr const char* TWO_WORD_PATTERN = R"((?i)(\p{L}{3,})\s+(\p{L}{3,})\s+(\b\d+[a-zA-Z']*\b))";

    // Single word (for finding unnumbered references)
    // Example: "Lager Motor Welle" -> matches each word
    // Pattern: word with 3 or more characters
    constexpr const char* WORD_PATTERN = R"((?i)\p{L}{3,})";
}
