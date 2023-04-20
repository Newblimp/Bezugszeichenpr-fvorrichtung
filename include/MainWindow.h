#pragma once
#include "BZComparator.h"
#include "wx/treelist.h"
#include <map>
#include <set>
#include <wx/dataview.h>
#include <wx/listctrl.h>
#include <wx/wx.h>

class MainWindow : public wxFrame {
public:
  MainWindow();

private:
  void scanText(wxCommandEvent &event);

  void printList();

  void normalizeText();

  std::map<wxString, std::set<wxString>> m_merkmale;
  std::shared_ptr<wxButton> m_scanButton;
  std::shared_ptr<wxTextCtrl> m_textBox;
  // std::shared_ptr<wxListCtrl> m_listBox;
  std::shared_ptr<wxTreeListCtrl> m_treeList;
  BZComparator m_BZComparator;
};
