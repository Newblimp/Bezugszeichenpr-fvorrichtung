#include <wx/treelist.h>

class BZComparator : public wxTreeListItemComparator {
  int Compare(wxTreeListCtrl *treelist, unsigned column, wxTreeListItem first,
              wxTreeListItem second) override;
};
