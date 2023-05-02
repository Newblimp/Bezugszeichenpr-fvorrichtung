#include "MainWindow.h"
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include "utils.h"
#include "wx/event.h"
#include "wx/gdicmn.h"
#include <algorithm>
#include <chrono>
#include <locale>
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

  // Add two buttons to a buttonSizer
  // m_scanButton = std::make_shared<wxButton>(panel, wxID_ANY, "Scan");
  // wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  // buttonSizer->Add(m_scanButton.get(), 0, wxCENTRE, 10);
  // mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

  loadIcons();
  m_treeList->SetImageList(m_imageList.get());

  // m_scanButton->Bind(wxEVT_BUTTON, &MainWindow::scanText, this);

  m_textBox->Bind(wxEVT_TEXT, &MainWindow::scanText, this);

  m_neutral_style.SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
  m_yellow_style.SetBackgroundColour(*wxYELLOW);
}

void MainWindow::scanText(wxCommandEvent &event) {
  auto start = std::chrono::high_resolution_clock::now();

  m_fullText = m_textBox->GetValue();
  // wxString text = m_textBox->GetValue();
  wxString text = m_fullText;
  m_merkmale.clear();
  m_all_merkmale.clear();

  auto setup_time = std::chrono::high_resolution_clock::now();

  if (m_regex.IsValid()) {
    size_t start, len;
    // Loop over the search string, finding matches
    while (m_regex.Matches(text)) {
      // Get the next match and add it to the collection
      m_regex.GetMatch(&start, &len, 0);

      wxString merkmal{m_regex.GetMatch(text, 1)};
      wxString bezugszeichen{m_regex.GetMatch(text, 2)};

      m_merkmale[bezugszeichen].emplace(merkmal);
      m_all_merkmale.emplace(merkmal);

      // Update the search string to exclude the current match
      text = text.Mid(start + len);
    }
  }

  auto regex_time = std::chrono::high_resolution_clock::now();

  printList();
  auto printlist_time = std::chrono::high_resolution_clock::now();

  m_treeList->SetSortColumn(0);
  checkIfWordMultipleNumbers();
  auto multi_check_time = std::chrono::high_resolution_clock::now();

  auto end = std::chrono::high_resolution_clock::now();

  auto setup_diff =
      std::chrono::duration_cast<std::chrono::milliseconds>(setup_time - start);

  auto regex_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      regex_time - setup_time);

  auto print_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      printlist_time - regex_time);

  auto multicheck_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      multi_check_time - printlist_time);

  std::cout << "The setup took " << setup_diff.count() << std::endl;
  std::cout << "The regex took " << regex_diff.count() << std::endl;
  std::cout << "The printlist took " << print_diff.count() << std::endl;
  std::cout << "The multicheck loop took " << multicheck_diff.count()
            << std::endl;
}

void MainWindow::printList() {
  // Empty the list
  m_treeList->DeleteAllItems();
  m_listBox->DeleteAllItems();

  m_textBox->SetStyle(0, m_textBox->GetValue().length(), m_neutral_style);

  // Used to refer to the line we add afterwards
  wxTreeListItem item;
  // Iterate over every saved item and insert it
  for (const auto &[bezugszeichen, merkmale] : m_merkmale) {
    // At highest level, add all merkmale together
    if (areAllWordsSame(merkmale)) {
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bezugszeichen, 0,
                                    0);
    } else {
      markWordsNumConflict(merkmale);
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bezugszeichen, 1,
                                    1);
      // Add the items to the "Double Check"-list
      m_listBox->InsertItem(0, bezugszeichen);
      m_listBox->SetItem(0, 1, "different words for the same number");
    }
    m_treeList->SetItemText(item, 1, merkmaleToString(merkmale));

    // Then add them all seperately on a lower level
    for (const auto &merkmal : merkmale) {
      auto row = m_treeList->AppendItem(item, "");
      m_treeList->SetItemText(row, 1, merkmal);
    }
  }
  // Add BZ without a number to the suggestion list
  auto start = std::chrono::high_resolution_clock::now();
  findUnnumberedWords();
  auto end = std::chrono::high_resolution_clock::now();

  auto diff =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "The findUnnumberedWords() method took " << diff.count()
            << std::endl;
}

void MainWindow::loadIcons() {
  m_imageList = std::make_shared<wxImageList>(16, 16, false, 0);
  wxBitmap check(check_16_xpm);
  wxBitmap warning(warning_16_xpm);

  m_imageList->Add(check);
  m_imageList->Add(warning);
}

void MainWindow::markWordsNumConflict(const std::set<wxString> &strings) {
  wxTextAttr style;
  style.SetBackgroundColour(*wxYELLOW);
  wxString currentText;

  int pos, offset{0};
  for (const auto &string : strings) {
    // Set up text and indices
    currentText = m_textBox->GetValue();
    pos = currentText.Find(string);
    offset = 0;

    // Loop the indices over the text
    while (pos != wxNOT_FOUND) {
      // Apply the red and bold style to the selected text
      m_textBox->SetStyle(pos + offset, pos + string.length() + offset, style);

      currentText = currentText.Mid(pos + string.length());
      offset += pos + string.length();

      // Find the position of the string in the text
      pos = currentText.Find(string);
    }
  }
}

void MainWindow::findUnnumberedWords() {

  wxString currentText = m_fullText;

  int offset{0};
  size_t pos{0};
  bool found_first_unnumbered_word{false};

  size_t set_size = m_all_merkmale.size();
  if (set_size > 0) {
    wxString regexString{"((?i)\\b"};
    auto set_iter = m_all_merkmale.begin();

    for (int i = 0; i < (m_all_merkmale.size() - 1); ++i) {
      regexString.Append(*set_iter + "|");
      ++set_iter;
    }
    regexString.Append(*set_iter);
    regexString.Append(")\\b(?!\\s\\(?\\d)");

    wxRegEx unnumbered_regex{regexString};

    std::set<wxString> unnumbered_strings;

    if (unnumbered_regex.IsValid()) {
      size_t len;

      // Loop over the search string, finding matches
      while (unnumbered_regex.Matches(currentText)) {
        // Get the next match and add it to the collection
        unnumbered_regex.GetMatch(&pos, &len, 1);

        m_textBox->SetStyle(pos + offset, pos + offset + len, m_yellow_style);
        // Update the search string to exclude the current match
        offset += pos + len;

        unnumbered_strings.emplace(currentText.SubString(pos, pos + len - 1));

        currentText = currentText.Mid(pos + len);
      }
    }

    for (const auto &string : unnumbered_strings) {
      m_listBox->InsertItem(0, string);
      m_listBox->SetItem(0, 1, "unnumbered");
    }
  }
}

void MainWindow::checkIfWordMultipleNumbers() {
  if (m_merkmale.size() < 2) {
    return;
  }
  auto iterator = m_merkmale.begin();
  auto first_word = *iterator->second.begin();
  ++iterator;

  for (; iterator != m_merkmale.end(); ++iterator) {
    if (areSameWord(first_word, *iterator->second.begin())) {
      m_listBox->InsertItem(0, first_word);
      m_listBox->SetItem(0, 1, "word assigned to multiple numbers");
    }
    first_word = *iterator->second.begin();
  }
}
