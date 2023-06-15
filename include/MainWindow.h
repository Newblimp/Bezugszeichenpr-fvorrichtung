#pragma once
#include "BZComparator.h"
#include "german_stem.h"
#include "wx/richtext/richtextctrl.h"
#include "wx/textctrl.h"
#include "wx/timer.h"
#include "wx/treelist.h"
#include <regex>
#include <wx/dataview.h>
#include <wx/listctrl.h>
#include <wx/wx.h>
#include <map>
#include "utils.h"

class MainWindow : public wxFrame
{
public:
  MainWindow();

private:
  void setupUi();
  void setupBindings();
  void scanText(wxTimerEvent &event);
  void fillListTree();
  void loadIcons();
  bool isUniquelyAssigned(const std::wstring &bz);
  void findUnnumberedWords();
  void findWrongNumberWords();
  void findSplitNumberWords();
  void setupAndClear();
  void stemWord(std::wstring &word);
  void debounceFunc(wxCommandEvent &event);
  void selectNextNoNumber(wxCommandEvent &event);
  void selectPreviousNoNumber(wxCommandEvent &event);
  void selectNextWrongNumber(wxCommandEvent &event);
  void selectPreviousWrongNumber(wxCommandEvent &event);
  void selectNextSplitNumber(wxCommandEvent &event);
  void selectPreviousSplitNumber(wxCommandEvent &event);
  void fillBzList();

  std::wregex m_regex{
      L"(\\b[[:alpha:]äöü]+\\b)[[:s:]](\\b[[:digit:]]+[a-zA-Z']*\\b)",
      std::regex_constants::ECMAScript | std::regex_constants::optimize |
          std::regex_constants::icase};
  stemming::german_stem<> m_germanStemmer;
  std::wstring m_fullText;
  wxTextAttr m_neutral_style;
  wxTextAttr m_yellow_style;
  wxTimer m_debounce_timer;

  std::map<std::wstring, std::unordered_set<std::wstring>, CustomComparator> m_graph;

  std::unordered_map<std::wstring, std::unordered_set<std::wstring>>
      m_merkmale_to_bz;
  std::unordered_map<std::wstring, std::unordered_set<std::wstring>>
      m_full_words;
  std::unordered_map<std::wstring, std::unordered_set<std::wstring>> m_merkmalToFullWords;
  std::unordered_set<std::wstring> m_all_merkmale;
  std::unordered_map<std::wstring, std::vector<size_t>> m_BzToPosition;
  std::unordered_map<std::wstring, std::vector<size_t>> m_StemToPosition;
  std::shared_ptr<wxRichTextCtrl> m_textBox;
  std::shared_ptr<wxRichTextCtrl> m_bzList;
  std::shared_ptr<wxImageList> m_imageList;
  std::shared_ptr<wxTreeListCtrl> m_treeList;
  std::shared_ptr<wxButton> m_ButtonForwardNoNumber;
  std::shared_ptr<wxButton> m_ButtonBackwardNoNumber;
  std::shared_ptr<wxButton> m_ButtonForwardWrongNumber;
  std::shared_ptr<wxButton> m_ButtonBackwardWrongNumber;
  std::shared_ptr<wxButton> m_ButtonForwardSplitNumber;
  std::shared_ptr<wxButton> m_ButtonBackwardSplitNumber;
  BZComparator m_BZComparator;

  std::vector<int> m_noNumberPos;
  int m_noNumberSelected{-2};
  std::shared_ptr<wxStaticText> m_noNumberLabel;
  std::vector<int> m_wrongNumberPos;
  int m_wrongNumberSelected{-2};
  std::shared_ptr<wxStaticText> m_wrongNumberLabel;
  std::vector<int> m_splitNumberPos;
  int m_splitNumberSelected{-2};
  std::shared_ptr<wxStaticText> m_splitNumberLabel;
};
