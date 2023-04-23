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

  // Create a vertical box sizer to keep the text, list, and buttons
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
  panel->SetSizer(mainSizer);

  // Create a horizontal box sizer to arrange the controls
  wxBoxSizer *viewSizer = new wxBoxSizer(wxHORIZONTAL);
  mainSizer->Add(viewSizer, 1, wxEXPAND, 10);

  // Add a text box to the sizer
  m_textBox = std::make_shared<wxTextCtrl>(
      panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
  viewSizer->Add(m_textBox.get(), 1, wxEXPAND | wxALL, 10);

  // Add a dataView to the sizer
  m_treeList = std::make_shared<wxTreeListCtrl>(
      panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  m_treeList->AppendColumn("BZeichen");
  m_treeList->AppendColumn("Merkmal");
  m_treeList->SetItemComparator(&m_BZComparator);
  viewSizer->Add(m_treeList.get(), 1, wxEXPAND | wxALL, 10);

  // Add two buttons to a buttonSizer
  m_scanButton = std::make_shared<wxButton>(panel, wxID_ANY, "Scan");
  wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  buttonSizer->Add(m_scanButton.get(), 0, wxCENTRE, 10);
  mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

  loadIcons();
  m_treeList->SetImageList(m_imageList.get());

  m_scanButton->Bind(wxEVT_BUTTON, &MainWindow::scanText, this);

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

  m_textBox->SetStyle(0, text.length(), m_textBox->GetDefaultStyle());
  printList();
  m_treeList->SetSortColumn(0);
}

void MainWindow::printList() {
  // Empty the list
  m_treeList->DeleteAllItems();

  // Used to refer to the line we add afterwards
  wxTreeListItem item;
  // Iterate over every saved item and insert it
  for (const auto &[bezugszeichen, merkmale] : m_merkmale) {
    // At highest level, add all merkmale together
    if (areAllWordsSame(merkmale)) {
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bezugszeichen, 0,
                                    0);
    } else {
      makeBold(merkmale);
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bezugszeichen, 1,
                                    1);
    }
    m_treeList->SetItemText(item, 1, merkmaleToString(merkmale));

    // Then add them all seperately on a lower level
    for (const auto &merkmal : merkmale) {
      auto row = m_treeList->AppendItem(item, "");
      m_treeList->SetItemText(row, 1, merkmal);
    }
  }
}

void MainWindow::loadIcons() {
  m_imageList = std::make_shared<wxImageList>(16, 16, false, 0);
  wxBitmap check(check_16_xpm);
  wxBitmap warning(warning_16_xpm);

  m_imageList->Add(check);
  m_imageList->Add(warning);
}

void MainWindow::makeBold(const std::set<wxString> &strings) {
  wxTextAttr style;
  style.SetFontWeight(wxFONTWEIGHT_BOLD);
  style.SetTextColour(*wxRED);
  // Get the current text in the wxTestCtrl
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
