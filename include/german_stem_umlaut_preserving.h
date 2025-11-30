#pragma once

#include "german_stem.h"
#include <vector>
#include <utility>

namespace stemming {

/**
 * @brief German stemmer that preserves umlauts in output
 *
 * This is a wrapper around the standard german_stem that prevents
 * the removal of umlauts (ä, ö, ü) in the final stemmed output.
 *
 * The standard Oleander german_stem always calls remove_german_umlauts()
 * at the end, which converts ä->a, ö->o, ü->u. For our German patent
 * application, we want to preserve the umlauts for better readability
 * and distinction between terms.
 */
template <typename string_typeT = std::wstring>
class german_stem_umlaut_preserving final {
public:
    /** @param[in,out] text string to stem, with umlauts preserved */
    void operator()(string_typeT& text) {
        if (text.empty()) {
            return;
        }

        // Store positions and types of umlauts before stemming
        std::vector<std::pair<size_t, wchar_t>> umlaut_positions;

        for (size_t i = 0; i < text.length(); ++i) {
            wchar_t ch = text[i];
            // Lowercase umlauts
            if (ch == 0xE4 || // ä
                ch == 0xF6 || // ö
                ch == 0xFC) { // ü
                umlaut_positions.push_back({i, ch});
            }
            // Uppercase umlauts
            else if (ch == 0xC4 || // Ä
                     ch == 0xD6 || // Ö
                     ch == 0xDC) { // Ü
                umlaut_positions.push_back({i, ch});
            }
        }

        // Apply standard German stemming
        m_stemmer(text);

        // Re-apply umlauts to corresponding positions if they were converted to base letters
        // Note: This is a heuristic since stemming may change word length
        for (const auto& [pos, original_char] : umlaut_positions) {
            if (pos < text.length()) {
                wchar_t current = text[pos];

                // If ä was converted to 'a', restore it
                if ((original_char == 0xE4 || original_char == 0xC4) &&
                    (current == L'a' || current == L'A')) {
                    text[pos] = (current == L'A') ? 0xC4 : 0xE4;
                }
                // If ö was converted to 'o', restore it
                else if ((original_char == 0xF6 || original_char == 0xD6) &&
                         (current == L'o' || current == L'O')) {
                    text[pos] = (current == L'O') ? 0xD6 : 0xF6;
                }
                // If ü was converted to 'u', restore it
                else if ((original_char == 0xFC || original_char == 0xDC) &&
                         (current == L'u' || current == L'U')) {
                    text[pos] = (current == L'U') ? 0xDC : 0xFC;
                }
            }
        }
    }

private:
    german_stem<string_typeT> m_stemmer;
};

} // namespace stemming
