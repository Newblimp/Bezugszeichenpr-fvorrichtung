#pragma once
#include "utils_core.h"
#include <wx/string.h>

// Convert a set of StemVectors to a display string, showing the original words
// This function requires wxString and is only used in GUI code
wxString stemsToDisplayString(
    const std::unordered_set<StemVector, StemVectorHash>& stems,
    const std::unordered_set<std::wstring>& originalWords);
