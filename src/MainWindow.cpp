#include "MainWindow.h"
#include "utils.h"
#include "wx/event.h"
#include <algorithm>
#include <locale>
#include <memory>
#include <string>
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

  // Add a list control to the sizer
  /*
  m_listBox = std::make_shared<wxListCtrl>(panel, wxID_ANY, wxDefaultPosition,
                                           wxDefaultSize, wxLC_REPORT);
  m_listBox->InsertColumn(0, "Zeichen");
  m_listBox->InsertColumn(1, "Merkmal", wxLIST_FORMAT_LEFT, 400);
  viewSizer->Add(m_listBox.get(), 1, wxEXPAND | wxALL, 10);
  */

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

  m_scanButton->Bind(wxEVT_BUTTON, &MainWindow::scanText, this);

  m_textBox->Bind(wxEVT_TEXT, &MainWindow::scanText, this);
}

void MainWindow::scanText(wxCommandEvent &event) {

  wxString text = m_textBox->GetValue();
  wxRegEx regex("(\\b\\w+) (\\d+\\b)");

  m_merkmale.clear();

  if (regex.IsValid()) {

    // Loop over the search string, finding matches
    while (regex.Matches(text)) {

      size_t start, len;
      // Get the next match and add it to the collection
      regex.GetMatch(&start, &len, 0);

      wxString bezugszeichen = regex.GetMatch(text, 2);
      if (!inSet(m_merkmale[bezugszeichen], regex.GetMatch(text, 1))) {
        m_merkmale[bezugszeichen].emplace(regex.GetMatch(text, 1));
      }

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
  // m_listBox->DeleteAllItems();
  // int row{0};
  wxTreeListItem item;

  for (const auto &[bezugszeichen, merkmale] : m_merkmale) {
    item = m_treeList->AppendItem(m_treeList->GetRootItem(), bezugszeichen);
    m_treeList->SetItemText(item, 1, merkmaleToString(merkmale));
    for (const auto &merkmal : merkmale) {
      auto row = m_treeList->AppendItem(item, "");
      m_treeList->SetItemText(row, 1, merkmal);
    }
  }
}
