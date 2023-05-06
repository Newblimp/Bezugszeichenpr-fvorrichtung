#include <string>
#include <unordered_map>
#include <unordered_set>

class StemCollector {
public:
  StemCollector(std::wstring &stem) : m_stem(stem){};
  std::wstring get_stem() const;
  std::unordered_set<std::wstring> get_full_words() const;
  void add_word(const std::wstring &word);

private:
  std::unordered_set<std::wstring> m_full_words;
  const std::wstring m_stem;
};
