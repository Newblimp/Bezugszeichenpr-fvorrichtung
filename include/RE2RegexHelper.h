#pragma once
#include <re2/re2.h>
#include <string>
#include <vector>
#include <codecvt>
#include <locale>

/**
 * @brief Helper class for using RE2 with wide strings
 *
 * RE2 works with UTF-8 strings, but wxWidgets and our codebase use wstring.
 * This class handles conversions and position mapping between the two formats.
 */
class RE2RegexHelper {
public:
    /**
     * @brief Match result containing captured groups and position information
     */
    struct MatchResult {
        std::vector<std::wstring> groups;  // Captured groups (group 0 is full match)
        size_t position;                    // Start position in wstring
        size_t length;                      // Length in wstring characters

        // Access captured groups by index
        const std::wstring& operator[](size_t idx) const {
            return groups[idx];
        }
    };

    /**
     * @brief Iterator-like interface for finding all matches in text
     */
    class MatchIterator {
    public:
        MatchIterator(const std::wstring& text, const RE2& pattern);

        bool hasNext() const { return m_hasMore; }
        MatchResult next();

    private:
        std::string m_utf8Text;
        const RE2& m_pattern;
        size_t m_currentPos;
        bool m_hasMore;
        std::vector<size_t> m_wcharPositions;  // Maps UTF-8 byte pos -> wchar pos

        void buildPositionMap(const std::wstring& text);
        size_t utf8PosToWcharPos(size_t utf8Pos) const;
    };

    /**
     * @brief Convert wstring to UTF-8 string
     */
    static std::string wstringToUtf8(const std::wstring& wstr);

    /**
     * @brief Convert UTF-8 string to wstring
     */
    static std::wstring utf8ToWstring(const std::string& str);
};
