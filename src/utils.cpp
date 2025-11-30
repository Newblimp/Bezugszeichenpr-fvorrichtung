#include "utils.h"

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
