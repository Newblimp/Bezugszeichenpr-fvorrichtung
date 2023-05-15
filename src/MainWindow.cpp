#include "MainWindow.h"
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include "utils.h"
#include "wx/event.h"
#include "wx/gdicmn.h"
#include "wx/timer.h"
#include <algorithm>
#include <chrono>
#include <locale>
#include <regex>
#include <string>
#include <wx/bitmap.h>
#include <wx/regex.h>
#include <wx/richtext/richtextctrl.h>

MainWindow::MainWindow() : wxFrame(nullptr, wxID_ANY, "My App") {
  // Create a panel to hold our controls
  wxPanel *panel = new wxPanel(this, wxID_ANY);

  // Create a horizontal box sizer to arrange the controls
  wxBoxSizer *viewSizer = new wxBoxSizer(wxHORIZONTAL);
  panel->SetSizer(viewSizer);

  // Create a vertical box sizer to keep the text, list, and buttons
  wxBoxSizer *outputSizer = new wxBoxSizer(wxVERTICAL);

  // Add a text box to the sizer
  m_textBox = std::make_shared<wxTextCtrl>(
      // m_textBox = std::make_shared<wxRichTextCtrl>(
      panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
      wxRE_MULTILINE | wxTE_RICH2);
  // wxDefaultSize, wxTE_MULTILINE | wxTE_RICH);
  viewSizer->Add(m_textBox.get(), 1, wxEXPAND | wxALL, 10);
  viewSizer->Add(outputSizer, 1, wxEXPAND, 10);

  // Add a dataView to the sizer
  m_treeList = std::make_shared<wxTreeListCtrl>(
      panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  m_treeList->AppendColumn("BZeichen");
  m_treeList->AppendColumn("Merkmal");
  m_treeList->SetItemComparator(&m_BZComparator);
  outputSizer->Add(m_treeList.get(), 2, wxEXPAND | wxALL, 10);

  m_listBox = std::make_shared<wxListCtrl>(panel, wxID_ANY, wxDefaultPosition,
                                           wxDefaultSize, wxLC_REPORT);
  m_listBox->AppendColumn("Double Check", wxLIST_FORMAT_LEFT, 133);
  m_listBox->AppendColumn("Reason", wxLIST_FORMAT_LEFT, 300);
  outputSizer->Add(m_listBox.get(), 1, wxEXPAND | wxALL, 10);

  loadIcons();
  m_treeList->SetImageList(m_imageList.get());

  m_textBox->Bind(wxEVT_TEXT, &MainWindow::debounceFunc, this);

  m_neutral_style.SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
  m_yellow_style.SetBackgroundColour(*wxYELLOW);

  m_debounce_timer.Bind(wxEVT_TIMER, &MainWindow::scanText, this);

  m_treeList->SetSortColumn(0);
}

void MainWindow::debounceFunc(wxCommandEvent &event) {
  m_debounce_timer.StartOnce(333);
}

void MainWindow::scanText(wxTimerEvent &event) {
  setupAndClear();

  std::wsregex_iterator regex_begin(m_fullText.begin(), m_fullText.end(),
                                    m_regex);
  std::wsregex_iterator regex_end;

  std::wstring merkmal;
  while (regex_begin != regex_end) {
    std::wsmatch match = *regex_begin;
    merkmal = (*regex_begin)[1];
    stemWord(merkmal);
    m_graph[(*regex_begin)[2]].emplace(merkmal);
    m_merkmale_to_bz[merkmal].emplace((*regex_begin)[2]);
    m_full_words[(*regex_begin)[2].str()].emplace((*regex_begin)[1].str());
    m_text_positions[(*regex_begin)[2].str()].emplace_back(
        (*regex_begin).position());
    m_text_positions[(*regex_begin)[2].str()].emplace_back(
        (*regex_begin).length());
    ++regex_begin;
  }

  fillListTree();
  findUnnumberedWords();
}

void MainWindow::fillListTree() {
  // Used to refer to the line we add afterwards
  wxTreeListItem item;
  // Iterate over every saved item and insert it
  for (const auto &[bezugszeichen, merkmale] : m_graph) {
    if (isUniquelyAssigned(bezugszeichen)) {
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bezugszeichen, 0,
                                    0);
    } else {
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bezugszeichen, 1,
                                    1);
      for (int i = 0; i < m_text_positions[bezugszeichen].size(); ++i) {
        m_textBox->SetStyle(m_text_positions[bezugszeichen][i],
                            m_text_positions[bezugszeichen][i + 1] +
                                m_text_positions[bezugszeichen][i],
                            m_yellow_style);
        ++i;
      }
    }
    m_treeList->SetItemText(
        item, 1, merkmaleToString(merkmale, m_full_words[bezugszeichen]));
  }
}

bool MainWindow::isUniquelyAssigned(const std::wstring &bz) {
  if (m_graph.at(bz).size() != 1) {
    return false;
  } else {
    for (const auto &word : m_graph.at(bz)) {
      if (m_merkmale_to_bz.at(word).size() != 1) {
        return false;
      }
    }
  }
  return true;
}

void MainWindow::loadIcons() {
  m_imageList = std::make_shared<wxImageList>(16, 16, false, 0);
  wxBitmap check(check_16_xpm);
  wxBitmap warning(warning_16_xpm);

  m_imageList->Add(check);
  m_imageList->Add(warning);
}

void MainWindow::findUnnumberedWords() {
  // Load every word into a container to later find the unnumbered ones
  emplaceAllMerkmale(m_full_words, m_all_merkmale);
  size_t set_size = m_all_merkmale.size();
  if (set_size > 0) {
    std::wstring regexString{L"\\b("};
    appendVectorForRegex(m_all_merkmale, regexString);
    regexString.append(L")\\b(?![[:s:]][[:digit:]])");
    std::wregex unnumbered_regex{regexString,
                                 std::regex_constants::ECMAScript |
                                     std::regex_constants::optimize |
                                     std::regex_constants::icase};

    std::wsregex_iterator regex_begin(m_fullText.begin(), m_fullText.end(),
                                      unnumbered_regex);
    std::wsregex_iterator regex_end;

    while (regex_begin != regex_end) {
      m_textBox->SetStyle(regex_begin->position(),
                          regex_begin->position() + regex_begin->length(),
                          m_yellow_style);
      m_listBox->InsertItem(0, regex_begin->str());
      m_listBox->SetItem(0, 1, "unnumbered");
      ++regex_begin;
    }
  }
}

void MainWindow::setupAndClear() {
  m_fullText = m_textBox->GetValue();

  m_full_words.clear();
  m_graph.clear();
  m_all_merkmale.clear();
  m_merkmale_to_bz.clear();
  m_text_positions.clear();
  m_treeList->DeleteAllItems();
  m_listBox->DeleteAllItems();

  m_textBox->SetStyle(0, m_textBox->GetValue().length(), m_neutral_style);
}

void MainWindow::stemWord(std::wstring &word) { m_germanStemmer(word); }
