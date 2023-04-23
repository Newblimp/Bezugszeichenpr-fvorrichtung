#include <set>
#include <wx/string.h>

wxString merkmaleToString(const std::set<wxString> &bezugszeichen);

bool areAllWordsSame(const std::set<wxString> &set);

bool areSameWord(const wxString &word1, const wxString &word2);

bool inSet(const std::set<wxString> &set, const wxString &string);

void normalizeString(std::wstring &input);
