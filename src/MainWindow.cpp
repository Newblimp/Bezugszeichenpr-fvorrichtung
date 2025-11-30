#include "MainWindow.h"
#include "ErrorNavigator.h"
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include "utils.h"
#include "wx/event.h"
#include "wx/gdicmn.h"
#include "wx/timer.h"
#include <algorithm>
#include <locale>
#include <string>
#include <wx/bitmap.h>

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY,
              wxString::FromUTF8("Bezugszeichenprüfvorrichtung"),
              wxDefaultPosition, wxSize(1200, 800)),
      // Initialize RE2 patterns (case-insensitive matching with (?i) prefix)
      m_singleWordRegex("(?i)(\\b\\p{L}+\\b)\\s+(\\b\\d+[a-zA-Z']*\\b)"),
      m_twoWordRegex("(?i)(\\b\\p{L}+\\b)\\s+(\\b\\p{L}+\\b)\\s+(\\b\\d+[a-zA-Z']*\\b)"),
      m_wordRegex("(?i)\\b\\p{L}+\\b"),
      m_twoWordNoNumberRegex("(?i)(\\b\\p{L}+\\b)(?!\\s+\\d)") {
#ifdef _WIN32
  SetIcon(wxIcon("1", wxBITMAP_TYPE_ICO_RESOURCE));
  SetIcon(wxIcon("APP_ICON", wxBITMAP_TYPE_ICO_RESOURCE));
#endif // SetIcon(wxIcon("APP_ICON", wxBITMAP_TYPE_ICO_RESOURCE"));

  // Verify RE2 patterns compiled successfully
  if (!m_singleWordRegex.ok()) {
    wxMessageBox("Failed to compile singleWordRegex: " +
                 wxString::FromUTF8(m_singleWordRegex.error().c_str()),
                 "Regex Error", wxOK | wxICON_ERROR);
  }
  if (!m_twoWordRegex.ok()) {
    wxMessageBox("Failed to compile twoWordRegex: " +
                 wxString::FromUTF8(m_twoWordRegex.error().c_str()),
                 "Regex Error", wxOK | wxICON_ERROR);
  }
  if (!m_wordRegex.ok()) {
    wxMessageBox("Failed to compile wordRegex: " +
                 wxString::FromUTF8(m_wordRegex.error().c_str()),
                 "Regex Error", wxOK | wxICON_ERROR);
  }
  if (!m_twoWordNoNumberRegex.ok()) {
    wxMessageBox("Failed to compile twoWordNoNumberRegex: " +
                 wxString::FromUTF8(m_twoWordNoNumberRegex.error().c_str()),
                 "Regex Error", wxOK | wxICON_ERROR);
  }

  setupUi();
  loadIcons();
  setupBindings();
}

void MainWindow::debounceFunc(wxCommandEvent &event) {
  m_debounceTimer.Start(500, true);
}

void MainWindow::scanText(wxTimerEvent &event) {
  setupAndClear();

  // Track matched positions to avoid duplicate processing
  std::vector<std::pair<size_t, size_t>> matchedRanges;

  auto overlapsExisting = [&matchedRanges](size_t start, size_t end) {
    for (const auto &range : matchedRanges) {
      if (!(end <= range.first || start >= range.second)) {
        return true;
      }
    }
    return false;
  };

  // First pass: scan for two-word patterns
  // Match "[word1] [word2] [number]" and check if word2's stem is in multi-word
  // set
  {
    RE2RegexHelper::MatchIterator iter(m_fullText, m_twoWordRegex);

    while (iter.hasNext()) {
      auto match = iter.next();
      size_t pos = match.position;
      size_t len = match.length;
      size_t endPos = pos + len;

      std::wstring word1 = match[1];
      std::wstring word2 = match[2];
      std::wstring bz = match[3];

      // Check if word2's stem is marked for multi-word matching
      if (m_textAnalyzer.isMultiWordBase(word2, m_multiWordBaseStems)) {
        if (!overlapsExisting(pos, endPos)) {
          matchedRanges.emplace_back(pos, endPos);

          // Create stem vector with both words stemmed separately
          StemVector stemVec = m_textAnalyzer.createMultiWordStemVector(word1, word2);
          std::wstring originalPhrase = word1 + L" " + word2;

          // Store mappings
          m_bzToStems[bz].insert(stemVec);
          m_stemToBz[stemVec].insert(bz);
          m_bzToOriginalWords[bz].insert(originalPhrase);

          // Track positions
          m_bzToPositions[bz].push_back({pos, len});
          m_stemToPositions[stemVec].push_back({pos, len});
        }
      }
    }
  }

  // Second pass: scan for single-word patterns (excluding already matched
  // positions)
  {
    RE2RegexHelper::MatchIterator iter(m_fullText, m_singleWordRegex);

    while (iter.hasNext()) {
      auto match = iter.next();
      size_t pos = match.position;
      size_t len = match.length;
      size_t endPos = pos + len;

      if (!overlapsExisting(pos, endPos)) {
        matchedRanges.emplace_back(pos, endPos);

        std::wstring word = match[1];
        std::wstring bz = match[2];

        // Create single-element stem vector
        StemVector stemVec = m_textAnalyzer.createStemVector(word);

        // Store mappings
        m_bzToStems[bz].insert(stemVec);
        m_stemToBz[stemVec].insert(bz);
        m_bzToOriginalWords[bz].insert(word);

        // Track positions
        m_bzToPositions[bz].push_back({pos, len});
        m_stemToPositions[stemVec].push_back({pos, len});
      }
    }
  }

  // Collect all unique stems
  collectAllStems(m_stemToBz, m_allStems);

  // Update display
  fillListTree();
  findUnnumberedWords();
  checkArticleUsage();

  // Update navigation labels
  m_noNumberLabel->SetLabel(
      L"0/" + std::to_wstring(m_noNumberPositions.size()) + L"\t");
  m_wrongNumberLabel->SetLabel(
      L"0/" + std::to_wstring(m_wrongNumberPositions.size()) + L"\t");
  m_splitNumberLabel->SetLabel(
      L"0/" + std::to_wstring(m_splitNumberPositions.size()) + L"\t");
  m_wrongArticleLabel->SetLabel(
      L"0/" + std::to_wstring(m_wrongArticlePositions.size()) + L"\t");

  fillBzList();
}

void MainWindow::fillListTree() {
  for (const auto &[bz, stems] : m_bzToStems) {
    wxTreeListItem item;

    // Check if error has been cleared by user
    bool isCleared = m_clearedErrors.count(bz) > 0;

    if (isCleared || isUniquelyAssigned(bz)) {
      // Use check icon (0) for cleared errors or no errors
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bz, 0, 0);
    } else {
      // Use warning icon (1) for actual errors
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bz, 1, 1);
    }

    // Display original words for this BZ
    m_treeList->SetItemText(
        item, 1, stemsToDisplayString(stems, m_bzToOriginalWords[bz]));
  }
}

bool MainWindow::isUniquelyAssigned(const std::wstring &bz) {
  // Check if this error has been cleared by user
  if (m_clearedErrors.count(bz) > 0) {
    return true; // Treat as no error
  }

  const auto &stems = m_bzToStems.at(bz);

  // Check if multiple different stems are assigned to this BZ
  if (stems.size() > 1) {
    // Highlight all occurrences of this BZ
    const auto &positions = m_bzToPositions[bz];
    // for (size_t i = 0; i < positions.size(); i += 2) {
    for (const auto i : positions) {
      size_t start = i.first;
      size_t len = i.second;
      m_wrongNumberPositions.emplace_back(start, start + len);
      m_textBox->SetStyle(start, start + len, m_warningStyle);
    }
    return false;
  }

  // Check if the stem is also used with other BZs
  for (const auto &stem : stems) {
    if (m_stemToBz.at(stem).size() > 1) {
      // This stem maps to multiple BZs - highlight occurrences
      const auto &positions = m_stemToPositions[stem];
      // for (size_t i = 0; i < positions.size(); i += 2) {
      for (const auto i : positions) {
        size_t start = i.first;
        size_t len = i.second;

        // Avoid duplicates in the split number list
        auto pos_pair = std::make_pair(static_cast<int>(start), static_cast<int>(start + len));
        if (std::find(m_splitNumberPositions.begin(),
                      m_splitNumberPositions.end(),
                      pos_pair) == m_splitNumberPositions.end()) {
          m_splitNumberPositions.emplace_back(start, start + len);
          m_textBox->SetStyle(start, start + len, m_warningStyle);
        }
      }
      return false;
    }
  }

  return true;
}

void MainWindow::loadIcons() {
  // wxIcon icon;
  // icon.CopyFromBitmap(wxBitmap("APP_ICON", wxBITMAP_TYPE_ICO_RESOURCE));
  // SetIcon(icon);

  // rest of the code
  m_imageList = std::make_shared<wxImageList>(16, 16, false, 0);
  wxBitmap check(check_16_xpm);
  wxBitmap warning(warning_16_xpm);

  m_imageList->Add(check);
  m_imageList->Add(warning);
  m_treeList->SetImageList(m_imageList.get());
}

void MainWindow::findUnnumberedWords() {
  // Collect start positions of all valid references
  std::unordered_set<size_t> validStarts;
  for (const auto &[stem, positions] : m_stemToPositions) {
    for (const auto [start, len] : positions) {
      validStarts.insert(start);
    }
  }

  // Check for two-word patterns without numbers (if they match known multi-word
  // stems)
  {
    RE2RegexHelper::MatchIterator iter(m_fullText, m_twoWordNoNumberRegex);

    if (iter.hasNext()) {
      // Store the previous match manually
      auto prev = iter.next();
      while (iter.hasNext()) {
        auto current = iter.next();
        size_t pos1 = prev.position;
        size_t pos2 = current.position;

        // Skip if already part of a valid reference
        if (validStarts.count(pos1)) {
          prev = current;
          continue;
        }

        std::wstring word1 = prev[1];
        std::wstring word2 = current[1];
        m_textAnalyzer.stemWord(word2);

        // Only flag if this is a known multi-word combination
        if (m_textAnalyzer.isMultiWordBase(word2, m_multiWordBaseStems)) {
          StemVector stemVec = m_textAnalyzer.createMultiWordStemVector(word1, word2);

          if (m_stemToBz.count(stemVec)) {
            size_t len2 = current.length;
            m_noNumberPositions.emplace_back(pos1, pos2 + len2);
            m_textBox->SetStyle(pos1, pos2 + len2, m_warningStyle);
          }
        }
        prev = current;
      }
    }
  }

  // Check for single words without numbers
  {
    RE2RegexHelper::MatchIterator iter(m_fullText, m_wordRegex);

    while (iter.hasNext()) {
      auto match = iter.next();
      size_t pos = match.position;

      // Skip if already part of a valid reference
      if (validStarts.count(pos)) {
        continue;
      }

      std::wstring word = match[0];  // Full match for wordRegex (no capture groups)
      StemVector stemVec = m_textAnalyzer.createStemVector(word);

      // Check if this stem is known from valid references
      if (m_stemToBz.count(stemVec)) {
        size_t len = match.length;
        m_noNumberPositions.emplace_back(pos, pos + len);
        m_textBox->SetStyle(pos, pos + len, m_warningStyle);
      }
    }
  }
}

void MainWindow::checkArticleUsage() {
  // We need to check each stem's occurrences in order of position
  // First occurrence should have indefinite article, subsequent should have
  // definite To avoid unnecessary errors, we only highlight the word if the
  // first article IS definite or the followings are indefinite

  // Build a map of stem -> sorted positions (by position in text)
  struct OccurrenceInfo {
    size_t position;
    size_t length;
    StemVector stem;
  };

  std::vector<OccurrenceInfo> allOccurrences;

  for (const auto &[stem, positions] : m_stemToPositions) {
    for (const auto [start, len] : positions) {
      allOccurrences.push_back({start, len, stem});
    }
  }

  // Sort by position
  std::sort(allOccurrences.begin(), allOccurrences.end(),
            [](const OccurrenceInfo &a, const OccurrenceInfo &b) {
              return a.position < b.position;
            });

  // Track which stems we've seen (first occurrence needs indefinite article)
  std::unordered_set<StemVector, StemVectorHash> seenStems;

  for (const auto &occ : allOccurrences) {
    auto [precedingWord, precedingPos] =
        GermanTextAnalyzer::findPrecedingWord(m_fullText, occ.position);

    if (precedingWord.empty()) {
      // No preceding word found - might be at start of text
      // Mark the stem as seen but we can't highlight an article
      seenStems.insert(occ.stem);
      continue;
    }

    bool isFirstOccurrence = (seenStems.count(occ.stem) == 0);
    size_t articleEnd = precedingPos + precedingWord.length();

    if (isFirstOccurrence) {
      // First occurrence: should not be definite article
      if (GermanTextAnalyzer::isDefiniteArticle(precedingWord)) {
        m_wrongArticlePositions.emplace_back(precedingPos, articleEnd);
        m_textBox->SetStyle(precedingPos, articleEnd, m_articleWarningStyle);
      }
      seenStems.insert(occ.stem);
    } else {
      // Subsequent occurrence: should have definite article
      if (GermanTextAnalyzer::isIndefiniteArticle(precedingWord)) {
        m_wrongArticlePositions.emplace_back(precedingPos, articleEnd);
        m_textBox->SetStyle(precedingPos, articleEnd, m_articleWarningStyle);
      }
    }
  }
}

void MainWindow::setupAndClear() {
  m_fullText = m_textBox->GetValue().ToStdWstring();

  // Clear all data structures
  m_bzToStems.clear();
  m_stemToBz.clear();
  m_bzToOriginalWords.clear();
  m_bzToPositions.clear();
  m_stemToPositions.clear();
  m_allStems.clear();

  m_treeList->DeleteAllItems();

  m_noNumberPositions.clear();
  m_wrongNumberPositions.clear();
  m_splitNumberPositions.clear();
  m_wrongArticlePositions.clear();
  m_bzCurrentOccurrence.clear();

  // Reset text highlighting
  m_textBox->SetStyle(0, m_textBox->GetValue().length(), m_neutralStyle);
}

void MainWindow::selectNextNoNumber(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_noNumberPositions, m_noNumberSelected, m_textBox, m_noNumberLabel.get());
}

void MainWindow::selectPreviousNoNumber(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_noNumberPositions, m_noNumberSelected, m_textBox, m_noNumberLabel.get());
}

void MainWindow::selectNextWrongNumber(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_wrongNumberPositions, m_wrongNumberSelected, m_textBox, m_wrongNumberLabel.get());
}

void MainWindow::selectPreviousWrongNumber(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_wrongNumberPositions, m_wrongNumberSelected, m_textBox, m_wrongNumberLabel.get());
}

void MainWindow::selectNextSplitNumber(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_splitNumberPositions, m_splitNumberSelected, m_textBox, m_splitNumberLabel.get());
}

void MainWindow::selectPreviousSplitNumber(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_splitNumberPositions, m_splitNumberSelected, m_textBox, m_splitNumberLabel.get());
}

void MainWindow::selectNextWrongArticle(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_wrongArticlePositions, m_wrongArticleSelected, m_textBox, m_wrongArticleLabel.get());
}

void MainWindow::selectPreviousWrongArticle(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_wrongArticlePositions, m_wrongArticleSelected, m_textBox, m_wrongArticleLabel.get());
}

void MainWindow::setupUi() {
  wxPanel *panel = new wxPanel(this, wxID_ANY);

  wxBoxSizer *viewSizer = new wxBoxSizer(wxHORIZONTAL);
  panel->SetSizer(viewSizer);

  wxBoxSizer *outputSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *numberSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *noNumberSizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *wrongNumberSizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *splitNumberSizer = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *wrongArticleSizer = new wxBoxSizer(wxHORIZONTAL);

  m_notebookList = new wxNotebook(panel, wxID_ANY);

  // Main text editor
  m_textBox = new wxRichTextCtrl(panel);
  m_bzList =
      std::make_shared<wxRichTextCtrl>(m_notebookList, wxID_ANY, wxEmptyString,
                                       wxDefaultPosition, wxSize(350, -1));

  viewSizer->Add(m_textBox, 1, wxEXPAND | wxALL, 10);
  viewSizer->Add(outputSizer, 0, wxEXPAND, 10);

  // Tree list for displaying BZ-term mappings
  m_treeList = std::make_shared<wxTreeListCtrl>(
      m_notebookList, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  m_treeList->AppendColumn("reference sign");
  m_treeList->AppendColumn("feature");

  outputSizer->Add(m_notebookList, 2, wxEXPAND | wxALL, 10);
  m_notebookList->AddPage(m_treeList.get(), "overview");
  m_notebookList->AddPage(m_bzList.get(), "reference sign list");

  // Navigation buttons for unnumbered references
  m_buttonBackwardNoNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, "<", wxDefaultPosition, wxSize(50, -1));
  m_buttonForwardNoNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, ">", wxDefaultPosition, wxSize(50, -1));

  // Navigation buttons for wrong number errors
  m_buttonBackwardWrongNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, "<", wxDefaultPosition, wxSize(50, -1));
  m_buttonForwardWrongNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, ">", wxDefaultPosition, wxSize(50, -1));

  // Navigation buttons for split number errors
  m_buttonBackwardSplitNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, "<", wxDefaultPosition, wxSize(50, -1));
  m_buttonForwardSplitNumber = std::make_shared<wxButton>(
      panel, wxID_ANY, ">", wxDefaultPosition, wxSize(50, -1));

  // Navigation buttons for wrong article errors
  m_buttonBackwardWrongArticle = std::make_shared<wxButton>(
      panel, wxID_ANY, "<", wxDefaultPosition, wxSize(50, -1));
  m_buttonForwardWrongArticle = std::make_shared<wxButton>(
      panel, wxID_ANY, ">", wxDefaultPosition, wxSize(50, -1));

  // Layout for unnumbered references row
  noNumberSizer->Add(m_buttonBackwardNoNumber.get());
  noNumberSizer->Add(m_buttonForwardNoNumber.get());
  auto noNumberDescription =
      new wxStaticText(panel, wxID_ANY, "unnumbered", wxDefaultPosition,
                       wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
  m_noNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
  noNumberSizer->Add(m_noNumberLabel.get(), 0, wxLEFT, 10);
  noNumberSizer->Add(noNumberDescription, 0, wxLEFT, 0);

  // Layout for wrong number row
  wrongNumberSizer->Add(m_buttonBackwardWrongNumber.get());
  wrongNumberSizer->Add(m_buttonForwardWrongNumber.get());
  auto wrongNumberDescription =
      new wxStaticText(panel, wxID_ANY, "inconsistent terms", wxDefaultPosition,
                       wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
  m_wrongNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
  wrongNumberSizer->Add(m_wrongNumberLabel.get(), 0, wxLEFT, 10);
  wrongNumberSizer->Add(wrongNumberDescription, 0, wxLEFT, 0);

  // Layout for split number row
  splitNumberSizer->Add(m_buttonBackwardSplitNumber.get());
  splitNumberSizer->Add(m_buttonForwardSplitNumber.get());
  auto splitNumberDescription = new wxStaticText(
      panel, wxID_ANY, "inconsistent reference signs", wxDefaultPosition,
      wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
  m_splitNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
  splitNumberSizer->Add(m_splitNumberLabel.get(), 0, wxLEFT, 10);
  splitNumberSizer->Add(splitNumberDescription, 0, wxLEFT, 0);

  // Layout for wrong article row
  wrongArticleSizer->Add(m_buttonBackwardWrongArticle.get());
  wrongArticleSizer->Add(m_buttonForwardWrongArticle.get());
  auto wrongArticleDescription = new wxStaticText(
      panel, wxID_ANY, "inconsistent article", wxDefaultPosition,
      wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
  m_wrongArticleLabel =
      std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
  wrongArticleSizer->Add(m_wrongArticleLabel.get(), 0, wxLEFT, 10);
  wrongArticleSizer->Add(wrongArticleDescription, 0, wxLEFT, 0);

  numberSizer->Add(noNumberSizer, wxLEFT);
  numberSizer->Add(wrongNumberSizer, wxLEFT);
  numberSizer->Add(splitNumberSizer, wxLEFT);
  numberSizer->Add(wrongArticleSizer, wxLEFT);

  outputSizer->Add(numberSizer, 0, wxALL, 10);

  // Set up text styles
  m_neutralStyle.SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
  m_warningStyle.SetBackgroundColour(*wxYELLOW);
  m_articleWarningStyle.SetBackgroundColour(*wxCYAN);
}

void MainWindow::setupBindings() {
  m_textBox->Bind(wxEVT_TEXT, &MainWindow::debounceFunc, this);
  m_debounceTimer.Bind(wxEVT_TIMER, &MainWindow::scanText, this);

  m_buttonBackwardNoNumber->Bind(wxEVT_BUTTON,
                                 &MainWindow::selectPreviousNoNumber, this);
  m_buttonForwardNoNumber->Bind(wxEVT_BUTTON, &MainWindow::selectNextNoNumber,
                                this);
  m_buttonBackwardWrongNumber->Bind(
      wxEVT_BUTTON, &MainWindow::selectPreviousWrongNumber, this);
  m_buttonForwardWrongNumber->Bind(wxEVT_BUTTON,
                                   &MainWindow::selectNextWrongNumber, this);
  m_buttonBackwardSplitNumber->Bind(
      wxEVT_BUTTON, &MainWindow::selectPreviousSplitNumber, this);
  m_buttonForwardSplitNumber->Bind(wxEVT_BUTTON,
                                   &MainWindow::selectNextSplitNumber, this);
  m_buttonBackwardWrongArticle->Bind(
      wxEVT_BUTTON, &MainWindow::selectPreviousWrongArticle, this);
  m_buttonForwardWrongArticle->Bind(wxEVT_BUTTON,
                                    &MainWindow::selectNextWrongArticle, this);

  m_treeList->Bind(wxEVT_TREELIST_ITEM_CONTEXT_MENU,
                   &MainWindow::onTreeListContextMenu, this);
  m_treeList->Bind(wxEVT_TREELIST_ITEM_ACTIVATED,
                   &MainWindow::onTreeListItemActivated, this);
}

void MainWindow::fillBzList() {
  m_bzList->SetValue("");

  auto treeItem = m_treeList->GetFirstItem();
  while (treeItem.IsOk()) {
    std::wstring bz = m_treeList->GetItemText(treeItem, 0).ToStdWstring();

    if (m_bzToPositions.count(bz) && m_bzToPositions[bz].size() >= 1) {
      size_t start = m_bzToPositions[bz][0].first;
      size_t len = m_bzToPositions[bz][0].second;

      // Extract the term without the BZ number
      size_t termLen = len > bz.size() + 1 ? len - bz.size() - 1 : 0;
      std::wstring termText = m_fullText.substr(start, termLen);

      m_bzList->AppendText(bz + L"\t" + termText + L"\n");
    }

    treeItem = m_treeList->GetNextItem(treeItem);
  }
}

void MainWindow::onTreeListContextMenu(wxTreeListEvent &event) {
  wxTreeListItem item = event.GetItem();
  if (!item.IsOk()) {
    return;
  }

  wxString bzText = m_treeList->GetItemText(item, 0);
  std::wstring bz = bzText.ToStdWstring();

  // Get the stems for this BZ to determine the base word
  if (m_bzToStems.count(bz) && !m_bzToStems[bz].empty()) {
    // Get the first stem vector
    const StemVector &firstStem = *m_bzToStems[bz].begin();

    if (firstStem.empty()) {
      return;
    }

    // The base stem is the last element (for single-word: the only element,
    // for multi-word: the second word like "lager")
    std::wstring baseStem = firstStem.back();

    // Create context menu
    wxMenu menu;
    bool isMultiWord = m_multiWordBaseStems.count(baseStem) > 0;
    menu.Append(1, isMultiWord ? "Disable multi-word mode"
                               : "Enable multi-word mode");

    // Check if this BZ actually has an error (ignoring cleared status)
    const auto &stems = m_bzToStems[bz];
    bool hasError = false;

    // Check if multiple different stems are assigned to this BZ
    if (stems.size() > 1) {
      hasError = true;
    }

    // Check if the stem is also used with other BZs
    if (!hasError) {
      for (const auto &stem : stems) {
        if (m_stemToBz.at(stem).size() > 1) {
          hasError = true;
          break;
        }
      }
    }

    // Only show clear/restore error option if there's an actual error
    if (hasError) {
      bool isCleared = m_clearedErrors.count(bz) > 0;
      menu.Append(2, isCleared ? "Restore error" : "Clear error");
    }

    int selection = GetPopupMenuSelectionFromUser(menu);
    if (selection == 1) {
      toggleMultiWordTerm(baseStem);
    } else if (selection == 2) {
      clearError(bz);
    }
  }
}

void MainWindow::toggleMultiWordTerm(const std::wstring &baseStem) {
  if (m_multiWordBaseStems.count(baseStem)) {
    m_multiWordBaseStems.erase(baseStem);
  } else {
    m_multiWordBaseStems.insert(baseStem);
  }

  // Trigger rescan
  m_debounceTimer.Start(1, true);
}

void MainWindow::clearError(const std::wstring &bz) {
  if (m_clearedErrors.count(bz)) {
    // Restore error - remove from cleared set
    m_clearedErrors.erase(bz);
  } else {
    // Clear error - add to cleared set
    m_clearedErrors.insert(bz);
  }

  // Trigger rescan to update highlighting and icons
  m_debounceTimer.Start(1, true);
}

void MainWindow::onTreeListItemActivated(wxTreeListEvent &event) {
  wxTreeListItem item = event.GetItem();
  if (!item.IsOk()) {
    return;
  }

  wxString bzText = m_treeList->GetItemText(item, 0);
  std::wstring bz = bzText.ToStdWstring();

  // Check if this BZ has any positions
  if (m_bzToPositions.count(bz) && !m_bzToPositions[bz].empty()) {
    const auto &positions = m_bzToPositions[bz];

    // Get current occurrence index for this BZ (or initialize based on cursor position)
    if (!m_bzCurrentOccurrence.count(bz)) {
      // Get current cursor position in the text
      long cursorPos = m_textBox->GetInsertionPoint();

      // Find the occurrence closest to or after the cursor position
      int closestIdx = 0;

      // If cursor is past all occurrences, start from the beginning
      if (cursorPos > static_cast<long>(positions[positions.size() - 1].second)) {
        closestIdx = 0;
      }
      // If cursor is before all occurrences, start from the first one
      else if (cursorPos < static_cast<long>(positions[0].first)) {
        closestIdx = 0;
      }
      // Otherwise, find the next occurrence at or after cursor
      else {
        for (size_t i = 0; i < positions.size(); ++i) {
          if (static_cast<long>(positions[i].first) >= cursorPos) {
            closestIdx = i;
            break;
          }
        }
      }

      m_bzCurrentOccurrence[bz] = closestIdx;
    }

    int &currentIdx = m_bzCurrentOccurrence[bz];

    // Get the current occurrence
    size_t start = positions[currentIdx].first;
    size_t len = positions[currentIdx].second;

    // Select and show the text
    m_textBox->SetSelection(start, start + len);
    m_textBox->ShowPosition(start);
    m_textBox->SetFocus();

    // Move to next occurrence for next double-click
    currentIdx = (currentIdx + 1) % positions.size();
  }
}
