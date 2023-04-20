#include "utils.h"
#include "wx/unichar.h"
#include <cstdlib>
#include <iostream>
#include <locale>

wxString merkmaleToString(const std::set<wxString> &bezugszeichen) {
  wxString listing;
  if (!areAllWordsSame(bezugszeichen)) {
    listing = "!!";
  }
  for (const auto &element : bezugszeichen) {
    listing.append(element + "; ");
  }
  return listing;
}

bool areAllWordsSame(const std::set<wxString> &set) {
  auto it = set.begin();
  wxString first = *it;
  ++it;
  for (; it != set.end(); ++it) {
    if (!areSameWord(*it, first)) {
      std::cout << "Different words are: " << *it << first << std::endl;
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

  std::string string1 = word1.ToStdString();
  std::string string2 = word2.ToStdString();

  std::cout << word1 << word2 << std::endl;
  std::cout << string1 << string2 << std::endl;

  normalizeString(string1);
  normalizeString(string2);

  // If the lenghts are more than 2 apart, we don't need to check further
  if (std::abs((int)string1.length() - (int)string2.length()) > 2) {
    std::cout << string1 << ": " << string1.length() << "; " << string2
              << string2.length() << std::endl;
    return false;
  }

  // If they are the same up to the end of the shorter word, it is likely a
  // plural
  for (int i = 0; i < std::min(string1.length(), string2.length()); ++i) {
    if (string1[i] != string2[i]) {
      std::cout << "These chars are different: " << string1[i] << " "
                << string2[i] << std::endl;
      return false;
    }
  }
  // TODO: Make sure to normalize the strings (lowercase + ÄÖÜ) before, make
  // own function case should not matter
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

void normalizeString(std::string &input) {
  std::locale loc;

  // Convert all letters to lowercase
  for (char &c : input) {
    c = std::tolower(c, loc);
  }

  // Replace diacritics with ASCII equivalents
  for (std::size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '\xc3') {
      switch (input[i + 1]) {
      case '\x84': // Ä
        input.replace(i, 2, "A");
        break;
      case '\x96': // Ö
        input.replace(i, 2, "O");
        break;
      case '\x9c': // Ü
        input.replace(i, 2, "U");
        break;
      default:
        break;
      }
    }
  }
}
