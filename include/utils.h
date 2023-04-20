#include <set>
#include <wx/string.h>

wxString merkmaleToString(const std::set<wxString> &bezugszeichen);

bool areSameWord(const wxString &word1, const wxString &word2);

bool inSet(const std::set<wxString> &set, const wxString &string);
