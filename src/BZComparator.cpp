#include "BZComparator.h"

int BZComparator::Compare(wxTreeListCtrl *treelist, unsigned column,
                          wxTreeListItem first, wxTreeListItem second) {

  // Get the strings
  wxString text1 = treelist->GetItemText(first, column),
           text2 = treelist->GetItemText(second, column);

  long long1{0}, long2{0};

  text1.ToLong(&long1);
  text2.ToLong(&long2);

  if (long1 < long2) {
    return -1;
  } else if (long2 < long1) {
    return 1;
  } else {
    if (text1.length() < text2.length()) {
      return -1;
    } else if (text2.length() < text1.length()) {
      return 1;
    }
  }
  return 0;
}
