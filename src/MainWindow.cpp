#include "MainWindow.h"
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include "utils.h"
#include "wx/event.h"
#include "wx/gdicmn.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <string>
#include <wx/bitmap.h>
#include <wx/regex.h>

MainWindow::MainWindow() : wxFrame(nullptr, wxID_ANY, "My App") {
  // Create a panel to hold our controls
  wxPanel *panel = new wxPanel(this, wxID_ANY);

  // Create a horizontal box sizer to arrange the controls
  wxBoxSizer *viewSizer = new wxBoxSizer(wxHORIZONTAL);
  panel->SetSizer(viewSizer);

  // Create a vertical box sizer to keep the text, list, and buttons
  wxBoxSizer *outputSizer = new wxBoxSizer(wxVERTICAL);

  // Add a text box to the sizer
  m_textBox =
      std::make_shared<wxTextCtrl>(panel, wxID_ANY, "", wxDefaultPosition,
                                   wxDefaultSize, wxTE_MULTILINE | wxTE_RICH);
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
}

void MainWindow::scanText(wxCommandEvent &event) {

  wxString text = m_textBox->GetValue();
  wxRegEx regex("(\\b\\p{L}+) (\\d+\\b)");
  m_merkmale.clear();

  if (regex.IsValid()) {
    // Loop over the search string, finding matches
    while (regex.Matches(text)) {
      size_t start, len;
      // Get the next match and add it to the collection
      regex.GetMatch(&start, &len, 0);

      m_merkmale[regex.GetMatch(text, 2)].emplace(regex.GetMatch(text, 1));

      // Update the search string to exclude the current match
      text = text.Mid(start + len);
    }
  }

  printList();
  m_treeList->SetSortColumn(0);
}

void MainWindow::printList() {
  // Empty the list
  m_treeList->DeleteAllItems();
  m_listBox->DeleteAllItems();

  wxTextAttr neutralStyle;
  neutralStyle.SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
  m_textBox->SetStyle(0, m_textBox->GetValue().length(), neutralStyle);

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
      for (const auto &merkmal : merkmale) {
        m_listBox->InsertItem(0, merkmal);
        m_listBox->SetItem(0, 1, "multiple words for the same number");
      }
    }
    m_treeList->SetItemText(item, 1, merkmaleToString(merkmale));

    // Then add them all seperately on a lower level
    for (const auto &merkmal : merkmale) {
      auto row = m_treeList->AppendItem(item, "");
      m_treeList->SetItemText(row, 1, merkmal);
    }

    // Add BZ without a number to the suggestion list
    findUnnumberedWords(merkmale);
  }
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

void MainWindow::findUnnumberedWords(const std::set<wxString> &bezugszeichen) {
  wxTextAttr style;
  style.SetBackgroundColour(*wxYELLOW);

  wxString currentText;

  int pos, offset{0};
  for (const auto &bz : bezugszeichen) {
    // Set up text and indices
    currentText = m_textBox->GetValue();
    pos = currentText.Find(bz);
    offset = 0;

    // Loop the indices over the text
    while (pos != wxNOT_FOUND) {
      if ((!wxIsdigit(currentText[pos + bz.length() + 1])) ||
          (currentText.length() <= (pos + bz.length() + 1))) {
        m_listBox->InsertItem(0, bz);
        m_listBox->SetItem(0, 1, "unnumbered");
        m_textBox->SetStyle(pos + offset, pos + bz.length() + offset, style);
      }
      currentText = currentText.Mid(pos + bz.length());
      offset += pos + bz.length();

      // Find the position of the string in the text
      pos = currentText.Find(bz);
    }
  }
}
