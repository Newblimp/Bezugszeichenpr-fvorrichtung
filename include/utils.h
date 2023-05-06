#include <unordered_map>
#include <unordered_set>
#include <wx/string.h>

wxString merkmaleToString(const std::unordered_set<std::wstring> &stems,
                          const std::unordered_set<std::wstring> &stem_origins);

void emplaceAllMerkmale(
    const std::unordered_map<std::wstring, std::unordered_set<std::wstring>>
        &merkmale,
    std::unordered_set<std::wstring> &merkVector);

void appendVectorForRegex(const std::unordered_set<std::wstring> &strings,
                          std::wstring &regexString);
