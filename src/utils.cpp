#include "utils.h"
#include <sstream>

wxString stemsToDisplayString(
    const std::unordered_set<StemVector, StemVectorHash>& stems,
    const std::unordered_set<std::wstring>& originalWords)
{
    wxString listing;
    
    // Display the original (unstemmed) words for readability
    for (const auto& word : originalWords) {
        listing.append(word + L"; ");
    }
    
    // Remove trailing "; " if present
    if (listing.length() >= 2) {
        listing.RemoveLast(2);
    }
    
    return listing;
}

std::wstring stemVectorToString(const StemVector& stems)
{
    if (stems.empty()) {
        return L"";
    }
    
    std::wstring result;
    for (size_t i = 0; i < stems.size(); ++i) {
        result += stems[i];
        if (i < stems.size() - 1) {
            result += L" ";
        }
    }
    return result;
}

void collectAllStems(
    const std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>& stemToBz,
    std::unordered_set<StemVector, StemVectorHash>& allStems)
{
    for (const auto& [stem, bzSet] : stemToBz) {
        allStems.insert(stem);
    }
}

void appendAlternationPattern(
    const std::unordered_set<std::wstring>& strings,
    std::wstring& regexString)
{
    if (strings.empty()) {
        return;
    }
    
    auto iter = strings.begin();
    const size_t count = strings.size();
    
    for (size_t i = 0; i < count - 1; ++i) {
        regexString.append(*iter + L"|");
        ++iter;
    }
    regexString.append(*iter);
}
