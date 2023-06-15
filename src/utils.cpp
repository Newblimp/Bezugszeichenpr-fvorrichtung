#include "utils.h"
#include "wx/unichar.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <locale>

wxString merkmaleToString(const std::unordered_set<std::wstring> &stems,
                          const std::unordered_set<std::wstring> &stem_origins)
{
  wxString listing;
  for (const auto &origin : stem_origins)
  {
    listing.append(origin + ";");
  }
  return listing;
}

void emplaceAllMerkmale(
    const std::unordered_map<std::wstring, std::unordered_set<std::wstring>>
        &stem_list,
    std::unordered_set<std::wstring> &all_merkmale)
{
  for (const auto &[stem, origins] : stem_list)
  {
    all_merkmale.insert(origins.begin(), origins.end());
  }
}

void appendVectorForRegex(const std::unordered_set<std::wstring> &strings,
                          std::wstring &regexString)
{
  auto set_iter = strings.begin();
  const size_t limit{strings.size() - 1};

  for (size_t i = 0; i < limit; ++i)
  {
    regexString.append(*set_iter + "|");
    ++set_iter;
  }
  regexString.append(*set_iter);
}
