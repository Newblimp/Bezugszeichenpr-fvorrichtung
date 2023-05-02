#pragma once
#include "BZComparator.h"
#include "wx/regex.h"
#include "wx/textctrl.h"
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

  void loadIcons();

  void markWordsNumConflict(const std::set<wxString> &strings);

  void findUnnumberedWords();

  void checkIfWordMultipleNumbers();

  const wxRegEx m_regex{"(\\b\\p{L}+) (\\d+\\b)"};
  std::map<wxString, std::set<wxString>> m_merkmale;
  std::map<std::wstring, std::set<wxString>> m_merkmale_to_bz;
  std::set<wxString> m_all_merkmale;
  // std::shared_ptr<wxButton> m_scanButton;
  std::shared_ptr<wxTextCtrl> m_textBox;
  std::shared_ptr<wxImageList> m_imageList;
  std::shared_ptr<wxListCtrl> m_listBox;
  std::shared_ptr<wxTreeListCtrl> m_treeList;
  BZComparator m_BZComparator;
};
