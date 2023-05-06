#include "stem_collector.h"

std::wstring StemCollector::get_stem() const { return m_stem; }

std::unordered_set<std::wstring> StemCollector::get_full_words() const {
  return m_full_words;
}

void StemCollector::add_word(const std::wstring &word) {
  m_full_words.emplace(word);
}
