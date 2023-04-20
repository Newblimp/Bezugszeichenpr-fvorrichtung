#include "utils.h"
#include "wx/unichar.h"

wxString merkmaleToString(const std::set<wxString> &bezugszeichen) {
  wxString listing;
  for (const auto &element : bezugszeichen) {
    listing.append(element + "; ");
  }
  return listing;
}

bool areSameWord(const wxString &word1, const wxString &word2) {

  // simplest case
  if (word1 == word2) {
    return true;
  }

  int len1 = word1.Length();
  int len2 = word2.Length();

  // If the lenghts are more than 2 apart, we don't need to check further
  if (abs(len1 - len2) > 2) {
    return false;
  }

  wxString lword1 = word1.Lower();
  wxString lword2 = word2.Lower();

  // If they are the same up to the end of the shorter word, it is likely a
  // plural
  for (int i = 0; i < std::min(len1, len2); ++i) {
    if (lword1[i] != lword2[i]) {
      return false;
    }
  }
  // TODO: Make sure to normalize the strings (lowercase + ÄÖÜ) before, make
  // own function case should not matter
  return false;
}

bool inSet(const std::set<wxString> &set, const wxString &string) {
  for (const auto &word : set) {
    if (areSameWord(word, string)) {
      return true;
    }
  }
  return false;
}
