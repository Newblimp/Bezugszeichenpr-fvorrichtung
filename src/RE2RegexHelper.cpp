#include "RE2RegexHelper.h"
#include <codecvt>
#include <locale>

// Thread-local cached converter to avoid repeated allocation
static thread_local std::wstring_convert<std::codecvt_utf8<wchar_t>> t_converter;

std::string RE2RegexHelper::wstringToUtf8(const std::wstring& wstr) {
    return t_converter.to_bytes(wstr);
}

std::wstring RE2RegexHelper::utf8ToWstring(const std::string& str) {
    return t_converter.from_bytes(str);
}

RE2RegexHelper::MatchIterator::MatchIterator(const std::wstring& text, const RE2& pattern)
    : m_pattern(pattern), m_currentPos(0), m_hasMore(true) {
    m_utf8Text = wstringToUtf8(text);
    buildPositionMap(text);

    // Check if there's at least one match
    re2::StringPiece input(m_utf8Text);
    if (!m_pattern.Match(input, 0, m_utf8Text.size(), RE2::UNANCHORED, nullptr, 0)) {
        m_hasMore = false;
    }
}

void RE2RegexHelper::MatchIterator::buildPositionMap(const std::wstring& text) {
    // Build a map from UTF-8 byte position to wchar position
    // Optimized: do single-pass conversion instead of per-character
    m_wcharPositions.clear();
    m_wcharPositions.reserve(m_utf8Text.size() + 1);

    size_t wcharPos = 0;
    size_t utf8Pos = 0;

    // Process text in chunks for better performance
    for (size_t i = 0; i < text.length(); ++i) {
        wchar_t wc = text[i];
        
        // Determine UTF-8 byte length for this character
        size_t utf8Len;
        if (wc <= 0x7F) {
            utf8Len = 1;
        } else if (wc <= 0x7FF) {
            utf8Len = 2;
        } else if (wc <= 0xFFFF) {
            utf8Len = 3;
        } else {
            utf8Len = 4;
        }
        
        // Map each UTF-8 byte position to the current wchar position
        for (size_t j = 0; j < utf8Len; ++j) {
            m_wcharPositions.push_back(wcharPos);
        }
        
        wcharPos++;
    }
    
    // Add final position
    m_wcharPositions.push_back(wcharPos);
}

size_t RE2RegexHelper::MatchIterator::utf8PosToWcharPos(size_t utf8Pos) const {
    if (utf8Pos >= m_wcharPositions.size()) {
        return m_wcharPositions.empty() ? 0 : m_wcharPositions.back();
    }
    return m_wcharPositions[utf8Pos];
}

RE2RegexHelper::MatchResult RE2RegexHelper::MatchIterator::next() {
    if (!m_hasMore) {
        return MatchResult();
    }

    // Prepare capture groups (full match + numbered groups)
    int numGroups = m_pattern.NumberOfCapturingGroups() + 1;
    std::vector<re2::StringPiece> groups(numGroups);

    re2::StringPiece input(m_utf8Text);
    bool found = m_pattern.Match(
        input,
        m_currentPos,
        m_utf8Text.size(),
        RE2::UNANCHORED,
        groups.data(),
        numGroups
    );

    if (!found) {
        m_hasMore = false;
        return MatchResult();
    }

    // Extract match information
    size_t matchStartUtf8 = groups[0].data() - m_utf8Text.data();
    size_t matchEndUtf8 = matchStartUtf8 + groups[0].size();

    MatchResult result;
    result.position = utf8PosToWcharPos(matchStartUtf8);
    result.length = utf8PosToWcharPos(matchEndUtf8) - result.position;

    // Convert captured groups to wstring
    // Optimization: reserve capacity and convert directly from StringPiece
    result.groups.reserve(numGroups);
    for (int i = 0; i < numGroups; ++i) {
        if (!groups[i].empty()) {
            // Direct conversion from StringPiece to avoid temporary std::string
            result.groups.push_back(t_converter.from_bytes(
                groups[i].data(), 
                groups[i].data() + groups[i].size()
            ));
        } else {
            result.groups.emplace_back();  // Use emplace_back for empty strings
        }
    }

    // Move position forward
    m_currentPos = matchEndUtf8;

    // Check if there's another match
    if (!m_pattern.Match(input, m_currentPos, m_utf8Text.size(), RE2::UNANCHORED, nullptr, 0)) {
        m_hasMore = false;
    }

    return result;
}
