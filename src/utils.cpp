#include "utils.h"
#include "wx/unichar.h"
#include <cstdlib>
#include <iostream>
#include <locale>

wxString merkmaleToString(const std::set<wxString> &bezugszeichen) {
  wxString listing;
  if (bezugszeichen.size() > 1) {
    for (const auto &element : bezugszeichen) {
      listing.append(element + "; ");
    }
  } else {
    for (const auto &element : bezugszeichen) {
      listing.append(element);
    }
  }
  return listing;
}

bool areAllWordsSame(const std::set<wxString> &set) {
  auto it = set.begin();
  wxString first = *it;
  ++it;
  for (; it != set.end(); ++it) {
    if (!areSameWord(*it, first)) {
      return false;
    }
  }
  return true;
}

bool areSameWord(const wxString &word1, const wxString &word2) {

  // simplest case
  // do not need to check for, because words should already be unique in set
  /*
  if (word1 == word2) {
    return true;
  }*/

  std::wstring string1 = word1.ToStdWstring();
  std::wstring string2 = word2.ToStdWstring();

  normalizeString(string1);
  normalizeString(string2);

  // If the lenghts are more than 2 apart, we don't need to check further
  if (std::abs((int)string1.length() - (int)string2.length()) > 2) {
    return false;
  }

  // If they are the same up to the end of the shorter word, it is likely a
  // plural
  for (int i = 0; i < std::min(string1.length(), string2.length()); ++i) {
    if (string1[i] != string2[i]) {
      return false;
    }
  }
  return true;
}

bool inSet(const std::set<wxString> &set, const wxString &string) {
  for (const auto &word : set) {
    if (areSameWord(word, string)) {
      return true;
    }
  }
  return false;
}

void normalizeString(std::wstring &input) {
  std::locale loc;

  // Convert all letters to lowercase
  // Remove diacritics
  for (auto &c : input) {
    c = std::tolower(c, loc);

    if ((c == L'Ä') || (c == L'ä')) {
      c = L'a';
    } else if ((c == L'Ö') || (c == L'ö')) {
      c = L'o';
    } else if ((c == L'Ü') || (c == L'ü'))
      c = L'u';
  }
}
