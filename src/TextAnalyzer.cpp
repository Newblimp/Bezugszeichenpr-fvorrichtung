#include "TextAnalyzer.h"
#include <cwctype>

std::pair<std::wstring, size_t> TextAnalyzer::findPrecedingWord(const std::wstring& text, size_t pos) const {
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
