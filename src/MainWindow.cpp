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
// #include <wx/richtext/richtextctrl.h>

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, "Bezugszeichenprüfvorrichtung",
              wxDefaultPosition, wxSize(1200, 800)) {
  setupUi();
  loadIcons();
  setupBindings();
}

void MainWindow::debounceFunc(wxCommandEvent &event) {
  m_debounce_timer.Start(500, true);
}

void MainWindow::scanText(wxTimerEvent &event) {
  setupAndClear();

  std::wsregex_iterator regex_begin(m_fullText.begin(), m_fullText.end(),
                                    m_regex);
  std::wsregex_iterator regex_end;
  std::wstring merkmal;
  while (regex_begin != regex_end) {
    merkmal = (*regex_begin)[1];
    stemWord(merkmal);
    m_graph[(*regex_begin)[2]].emplace(merkmal);
    m_merkmale_to_bz[merkmal].emplace((*regex_begin)[2]);
    m_full_words[(*regex_begin)[2].str()].emplace((*regex_begin)[1].str());
    m_BzToPosition[(*regex_begin)[2].str()].emplace_back(
        (*regex_begin).position());
    m_BzToPosition[(*regex_begin)[2].str()].emplace_back(
        (*regex_begin).length());
    m_StemToPosition[merkmal].emplace_back((*regex_begin).position());
    m_StemToPosition[merkmal].emplace_back((*regex_begin).length());
    ++regex_begin;
  }

  fillListTree();
  findUnnumberedWords();
  m_noNumberLabel->SetLabel("0/" + std::to_wstring(m_noNumberPos.size() / 2) +
                            "\t");
  m_wrongNumberLabel->SetLabel(
      "0/" + std::to_wstring(m_wrongNumberPos.size() / 2) + "\t");
  m_splitNumberLabel->SetLabel(
      "0/" + std::to_wstring(m_splitNumberPos.size() / 2) + "\t");

  fillBzList();
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
    }
    m_treeList->SetItemText(
        item, 1, merkmaleToString(merkmale, m_full_words[bezugszeichen]));
  }
}

bool MainWindow::isUniquelyAssigned(const std::wstring &bz) {
  if (m_graph.at(bz).size() > 1) {
    auto positions = m_BzToPosition[bz];
    for (int i = 0; i < positions.size(); ++i) {
      m_wrongNumberPos.emplace_back(positions[i]);
      m_wrongNumberPos.emplace_back(positions[i + 1] + positions[i]);
      m_textBox->SetStyle(positions[i], positions[i + 1] + positions[i],
                          m_yellow_style);
      ++i;
    }
    return false;
  }
  for (const auto &word : m_graph.at(bz)) {
    if (m_merkmale_to_bz.at(word).size() > 1) {
      auto positions = m_StemToPosition[word];
      for (int i = 0; i < positions.size(); ++i) {
        if (std::find(m_splitNumberPos.begin(), m_splitNumberPos.end(),
                      positions[i]) == m_splitNumberPos.end()) {
          m_splitNumberPos.emplace_back(positions[i]);
          m_splitNumberPos.emplace_back(positions[i + 1] + positions[i]);
          m_textBox->SetStyle(positions[i], positions[i + 1] + positions[i],
                              m_yellow_style);
        }
        ++i;
      }
      return false;
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
  m_treeList->SetImageList(m_imageList.get());
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
      m_noNumberPos.emplace_back(regex_begin->position());
      m_noNumberPos.emplace_back(regex_begin->position() +
                                 regex_begin->length());
      m_textBox->SetStyle(regex_begin->position(),
                          regex_begin->position() + regex_begin->length(),
                          m_yellow_style);
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
  m_BzToPosition.clear();
  m_StemToPosition.clear();
  m_treeList->DeleteAllItems();
  m_noNumberPos.clear();
  m_wrongNumberPos.clear();
  m_splitNumberPos.clear();
  m_merkmale_to_bz.clear();
  m_textBox->SetStyle(0, m_textBox->GetValue().length(), m_neutral_style);
}

void MainWindow::stemWord(std::wstring &word) {
  word[0] = std::tolower(word[0]);
  m_germanStemmer(word);
}

void MainWindow::selectNextNoNumber(wxCommandEvent &event) {
  m_noNumberSelected += 2;
  if (m_noNumberSelected >= m_noNumberPos.size() || m_noNumberSelected < 0) {
    m_noNumberSelected = 0;
  }
  if (m_noNumberPos.size()) {
    m_textBox->SetSelection(m_noNumberPos[m_noNumberSelected],
                            m_noNumberPos[m_noNumberSelected + 1]);
    m_textBox->ShowPosition(m_noNumberPos[m_noNumberSelected]);
  }

  if (m_noNumberPos.size()) {
    m_noNumberLabel->SetLabel(std::to_wstring(m_noNumberSelected / 2 + 1) +
                              "/" + std::to_wstring(m_noNumberPos.size() / 2) +
                              "\t");
  }
}

void MainWindow::selectPreviousNoNumber(wxCommandEvent &event) {
  m_noNumberSelected -= 2;
  if (m_noNumberSelected >= m_noNumberPos.size() || m_noNumberSelected < 0) {
    m_noNumberSelected = m_noNumberPos.size() - 2;
  }
  if (m_noNumberPos.size()) {
    m_textBox->SetSelection(m_noNumberPos[m_noNumberSelected],
                            m_noNumberPos[m_noNumberSelected + 1]);
    m_textBox->ShowPosition(m_noNumberPos[m_noNumberSelected]);
  }

  if (m_noNumberPos.size()) {
    m_noNumberLabel->SetLabel(std::to_wstring(m_noNumberSelected / 2 + 1) +
                              "/" + std::to_wstring(m_noNumberPos.size() / 2) +
                              "\t");
  }
}

void MainWindow::selectNextWrongNumber(wxCommandEvent &event) {
  m_wrongNumberSelected += 2;
  if (m_wrongNumberSelected >= m_wrongNumberPos.size() ||
      m_wrongNumberSelected < 0) {
    m_wrongNumberSelected = 0;
  }
  if (m_wrongNumberPos.size()) {
    m_textBox->SetSelection(m_wrongNumberPos[m_wrongNumberSelected],
                            m_wrongNumberPos[m_wrongNumberSelected + 1]);
    m_textBox->ShowPosition(m_wrongNumberPos[m_wrongNumberSelected]);
  }

  if (m_wrongNumberPos.size()) {
    m_wrongNumberLabel->SetLabel(
        std::to_wstring(m_wrongNumberSelected / 2 + 1) + "/" +
        std::to_wstring(m_wrongNumberPos.size() / 2) + "\t");
  }
}

void MainWindow::selectPreviousWrongNumber(wxCommandEvent &event) {
  m_wrongNumberSelected -= 2;
  if (m_wrongNumberSelected >= m_wrongNumberPos.size() ||
      m_wrongNumberSelected < 0) {
    m_wrongNumberSelected = m_wrongNumberPos.size() - 2;
  }
  if (m_wrongNumberPos.size()) {
    m_textBox->SetSelection(m_wrongNumberPos[m_wrongNumberSelected],
                            m_wrongNumberPos[m_wrongNumberSelected + 1]);
    m_textBox->ShowPosition(m_wrongNumberPos[m_wrongNumberSelected]);
  }

  if (m_wrongNumberPos.size()) {
    m_wrongNumberLabel->SetLabel(
        std::to_wstring(m_wrongNumberSelected / 2 + 1) + "/" +
        std::to_wstring(m_wrongNumberPos.size() / 2) + "\t");
  }
}

void MainWindow::selectNextSplitNumber(wxCommandEvent &event) {
  m_splitNumberSelected += 2;
  if (m_splitNumberSelected >= m_splitNumberPos.size() ||
      m_splitNumberSelected < 0) {
    m_splitNumberSelected = 0;
  }
  if (m_splitNumberPos.size()) {
    m_textBox->SetSelection(m_splitNumberPos[m_splitNumberSelected],
                            m_splitNumberPos[m_splitNumberSelected + 1]);
    m_textBox->ShowPosition(m_splitNumberPos[m_splitNumberSelected]);
  }

  if (m_splitNumberPos.size()) {
    m_splitNumberLabel->SetLabel(
        std::to_wstring(m_splitNumberSelected / 2 + 1) + "/" +
        std::to_wstring(m_splitNumberPos.size() / 2) + "\t");
  }
}

void MainWindow::selectPreviousSplitNumber(wxCommandEvent &event) {
  m_splitNumberSelected -= 2;
  if (m_splitNumberSelected >= m_splitNumberPos.size() ||
      m_splitNumberSelected < 0) {
    m_splitNumberSelected = m_splitNumberPos.size() - 2;
  }
  if (m_splitNumberPos.size()) {
    m_textBox->SetSelection(m_splitNumberPos[m_splitNumberSelected],
                            m_splitNumberPos[m_splitNumberSelected + 1]);
    m_textBox->ShowPosition(m_splitNumberPos[m_splitNumberSelected]);
  }

  if (m_splitNumberPos.size()) {
    m_splitNumberLabel->SetLabel(
        std::to_wstring(m_splitNumberSelected / 2 + 1) + "/" +
        std::to_wstring(m_splitNumberPos.size() / 2) + "\t");
  }
}

void MainWindow::setupUi() {
  // Create a panel to hold our controls
  wxPanel *panel = new wxPanel(this, wxID_ANY);

  // Create a horizontal box sizer to arrange the controls
  wxBoxSizer *viewSizer = new wxBoxSizer(wxHORIZONTAL);
  panel->SetSizer(viewSizer);

  // Create a vertical box sizer to keep the text, list, and buttons
  wxBoxSizer *outputSizer = new wxBoxSizer(wxVERTICAL);

  wxBoxSizer *numberSizer = new wxBoxSizer(wxVERTICAL);

  wxBoxSizer *noNumberSizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *wrongNumberSizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *splitNumberSizer = new wxBoxSizer(wxHORIZONTAL);

  m_notebookList = new wxNotebook(panel, wxID_ANY);

  // Add a text box to the sizer
  m_textBox = new wxRichTextCtrl(panel);
  m_bzList =
      std::make_shared<wxRichTextCtrl>(m_notebookList, wxID_ANY, wxEmptyString,
                                       wxDefaultPosition, wxSize(250, -1));
  // viewSizer->Add(m_bzList.get(), 0, wxEXPAND | wxALL, 10);
  viewSizer->Add(m_textBox, 1, wxEXPAND | wxALL, 10);
  viewSizer->Add(outputSizer, 0, wxEXPAND, 10);

  // Add a dataView to the sizer
  m_treeList = std::make_shared<wxTreeListCtrl>(
      m_notebookList, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  m_treeList->AppendColumn("BZeichen");
  m_treeList->AppendColumn("Merkmal");
  // m_treeList->SetItemComparator(&m_BZComparator);
  outputSizer->Add(m_notebookList, 2, wxEXPAND | wxALL, 10);
  m_notebookList->AddPage(m_treeList.get(), "List");
  m_notebookList->AddPage(m_bzList.get(), "Text Edit");
  // outputSizer->Add(m_treeList.get(), 2, wxEXPAND | wxALL, 10);

  // Add the Buttons to cycle through errors
  m_ButtonBackwardNoNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, "<", wxDefaultPosition, wxSize(50, -1));
  m_ButtonForwardNoNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, ">", wxDefaultPosition, wxSize(50, -1));
  m_ButtonBackwardWrongNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, "<", wxDefaultPosition, wxSize(50, -1));
  m_ButtonForwardWrongNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, ">", wxDefaultPosition, wxSize(50, -1));
  m_ButtonBackwardSplitNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, "<", wxDefaultPosition, wxSize(50, -1));
  m_ButtonForwardSplitNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, ">", wxDefaultPosition, wxSize(50, -1));

  // Position the buttons in the sizers
  noNumberSizer->Add(m_ButtonBackwardNoNumber.get());
  noNumberSizer->Add(m_ButtonForwardNoNumber.get());
  auto noNumberDescription =
      new wxStaticText(panel, wxID_ANY, "Unnumbered", wxDefaultPosition,
                       wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
  m_noNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
  noNumberSizer->Add(m_noNumberLabel.get(), 0, wxLEFT, 10);
  noNumberSizer->Add(noNumberDescription, 0, wxLEFT, 0);

  wrongNumberSizer->Add(m_ButtonBackwardWrongNumber.get());
  wrongNumberSizer->Add(m_ButtonForwardWrongNumber.get());
  auto wrongNumberDescription = new wxStaticText(
      panel, wxID_ANY, "Multiple Words per Number", wxDefaultPosition,
      wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
  m_wrongNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
  wrongNumberSizer->Add(m_wrongNumberLabel.get(), 0, wxLEFT, 10);
  wrongNumberSizer->Add(wrongNumberDescription, 0, wxLEFT, 0);

  splitNumberSizer->Add(m_ButtonBackwardSplitNumber.get());
  splitNumberSizer->Add(m_ButtonForwardSplitNumber.get());
  auto splitNumberDescription = new wxStaticText(
      panel, wxID_ANY, "Multiple Numbers per Word", wxDefaultPosition,
      wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
  m_splitNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
  splitNumberSizer->Add(m_splitNumberLabel.get(), 0, wxLEFT, 10);
  splitNumberSizer->Add(splitNumberDescription, 0, wxLEFT, 0);

  numberSizer->Add(noNumberSizer, wxLEFT);
  numberSizer->Add(wrongNumberSizer, wxLEFT);
  numberSizer->Add(splitNumberSizer, wxLEFT);

  outputSizer->Add(numberSizer, 0, wxALL, 10);

  m_neutral_style.SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
  m_yellow_style.SetBackgroundColour(*wxYELLOW);

  // m_treeList->SetSortColumn(0);
}

void MainWindow::setupBindings() {
  m_textBox->Bind(wxEVT_TEXT, &MainWindow::debounceFunc, this);
  m_debounce_timer.Bind(wxEVT_TIMER, &MainWindow::scanText, this);
  m_ButtonBackwardNoNumber->Bind(wxEVT_BUTTON,
                                 &MainWindow::selectPreviousNoNumber, this);
  m_ButtonForwardNoNumber->Bind(wxEVT_BUTTON, &MainWindow::selectNextNoNumber,
                                this);
  m_ButtonBackwardWrongNumber->Bind(
      wxEVT_BUTTON, &MainWindow::selectPreviousWrongNumber, this);
  m_ButtonForwardWrongNumber->Bind(wxEVT_BUTTON,
                                   &MainWindow::selectNextWrongNumber, this);
  m_ButtonBackwardSplitNumber->Bind(
      wxEVT_BUTTON, &MainWindow::selectPreviousSplitNumber, this);
  m_ButtonForwardSplitNumber->Bind(wxEVT_BUTTON,
                                   &MainWindow::selectNextSplitNumber, this);
}

void MainWindow::fillBzList() {
  m_bzList->SetValue("");

  auto treeItem = m_treeList->GetFirstItem();
  std::wstring bz{m_treeList->GetItemText(treeItem, 0)};
  while (treeItem.IsOk()) {
    m_bzList->AppendText(bz + "\t");
    m_bzList->AppendText(
        m_fullText.substr(m_BzToPosition[bz][0],
                          m_BzToPosition[bz][1] - bz.size() - 1) +
        "\n");
    treeItem = m_treeList->GetNextItem(treeItem);
    bz = m_treeList->GetItemText(treeItem, 0);
  }

  /*
    std::size_t bzSize{0};
    for (const auto merkmal : m_merkmalToFullWords)
    {
      m_bzList->AppendText(merkmal.first + "\t(");
      bzSize = m_merkmale_to_bz[merkmal.first].size();
      if (bzSize != 1)
      {
        for (const auto bz : m_merkmale_to_bz[merkmal.first])
        {
          m_bzList->AppendText(bz);
          if (bzSize > 1){
            m_bzList->AppendText(",");
            --bzSize;
          }
        }
      }
      else
      {
        for (const auto bz : m_merkmale_to_bz[merkmal.first])
        {
          m_bzList->AppendText(bz);
        }
      }
      m_bzList->AppendText(")\n");
    }
    */
}
