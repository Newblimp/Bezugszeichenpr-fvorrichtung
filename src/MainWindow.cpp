#include "MainWindow.h"
#include "RegexPatterns.h"
#include "GermanTextAnalyzer.h"
#include "EnglishTextAnalyzer.h"
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include "ErrorNavigator.h"
#include "TextScanner.h"
#include "ErrorDetectorHelper.h"
#include "UIBuilder.h"
#include "utils.h"
#include "ImageViewerWindow.h"
#include "wx/event.h"
#include "wx/gdicmn.h"
#include "wx/timer.h"
#include "wx/wupdlock.h"
#include <algorithm>
#include <locale>
#include <string>
#include <wx/bitmap.h>
#include "TimerHelper.h"


MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY,
              wxString::FromUTF8("Bezugszeichenprüfvorrichtung"),
              wxDefaultPosition, wxSize(1200, 800)),
      // Initialize RE2 patterns using shared constants
      m_singleWordRegex(RegexPatterns::SINGLE_WORD_PATTERN),
      m_twoWordRegex(RegexPatterns::TWO_WORD_PATTERN),
      m_wordRegex(RegexPatterns::WORD_PATTERN) {
  
  // Initialize default analyzer (German)
  m_currentAnalyzer = std::make_unique<GermanTextAnalyzer>();

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

  setupUi();
  loadIcons();
  setupBindings();
}

void MainWindow::debounceFunc(wxCommandEvent &event) {
  m_debounceTimer.Start(500, true);
}

void MainWindow::scanText(wxTimerEvent &event) {
  // Cancel any running scan
  m_cancelScan = true;

  // Wait for previous thread to finish if it's still running
  if (m_scanThread.joinable()) {
    m_scanThread.join();
  }

  // Reset cancellation flag
  m_cancelScan = false;

  // Get the text to scan (on main thread, as required by wxWidgets)
  m_fullText = m_textBox->GetValue().ToStdWstring();

  // Launch background thread for scanning
  m_scanThread = std::jthread([this](std::stop_token stoken) {
    this->scanTextBackground();
  });
}

void MainWindow::scanTextBackground() {
  // This function runs on the background thread
  // We lock the mutex for the entire operation to ensure data consistency
  std::lock_guard<std::mutex> lock(m_dataMutex);

  Timer t_total;

  // Check for cancellation
  if (m_cancelScan) {
    return;
  }

  Timer t_setup;
  // Clear all results
  m_ctx.clearResults();

  m_allErrorsPositions.clear();
  m_noNumberPositions.clear();
  m_wrongTermBzPositions.clear();
  m_wrongArticlePositions.clear();

  std::cout << "Time for setup and clearing: " << t_setup.elapsed() << " milliseconds\n";

  if (m_cancelScan) {
    return;
  }

  // AUTO-DETECT ordinal patterns for multi-word terms BEFORE scanning
  Timer t_ordinalDetect;
  bool useGerman = (dynamic_cast<GermanTextAnalyzer*>(m_currentAnalyzer.get()) != nullptr);
  std::unordered_set<std::wstring> newAutoDetected =
      OrdinalDetector::detectOrdinalPatterns(m_fullText, m_twoWordRegex, useGerman, *m_currentAnalyzer);
  std::cout << "Time for OrdinalDetector: " << t_ordinalDetect.elapsed() << " milliseconds\n";

  // Rebuild combined multi-word set: manual + auto - disabled
  m_ctx.multiWordBaseStems = m_ctx.manualMultiWordToggles;  // Start with manual enables
  for (const auto& stem : newAutoDetected) {
    if (m_ctx.manuallyDisabledMultiWord.count(stem) == 0) {
      m_ctx.multiWordBaseStems.insert(stem);  // Add auto-detected if not manually disabled
    }
  }
  m_ctx.autoDetectedMultiWordStems = newAutoDetected;  // Remember what was auto-detected

  if (m_cancelScan) {
    return;
  }

  // Scan text for patterns using TextScanner
  Timer t_scan;
  TextScanner::scanText(m_fullText, *m_currentAnalyzer, m_singleWordRegex, m_twoWordRegex, m_ctx);
  std::cout << "Time for TextScanner::scanText: " << t_scan.elapsed() << " milliseconds\n";

  // Cache first occurrence words for display
  m_ctx.db.stemToFirstWord.clear();
  for (const auto& [stem, positions] : m_ctx.db.stemToPositions) {
    if (!positions.empty()) {
      size_t firstStart = positions[0].first;
      size_t firstLen = positions[0].second;
      std::wstring fullMatch = m_fullText.substr(firstStart, firstLen);

      // Extract word before BZ number
      size_t bzStart = fullMatch.find_last_of(L' ');
      if (bzStart != std::wstring::npos) {
        m_ctx.db.stemToFirstWord[stem] = fullMatch.substr(0, bzStart);
      } else {
        m_ctx.db.stemToFirstWord[stem] = fullMatch;
      }
    }
  }

  std::cout << "Total background scan time: " << t_total.elapsed() << " milliseconds\n";

  // Schedule UI update on main thread
  // Note: CallAfter is thread-safe in wxWidgets
  CallAfter(&MainWindow::updateUIAfterScan);
}

void MainWindow::updateUIAfterScan() {
  // This function runs on the main thread
  // Lock mutex while accessing shared data
  std::lock_guard<std::mutex> lock(m_dataMutex);

  // Check if scan was cancelled
  if (m_cancelScan) {
    return;
  }

  // RAII-based window update locker prevents redraws during updates
  wxWindowUpdateLocker updateLocker(m_textBox);
  m_textBox->BeginSuppressUndo();

  // Clear UI elements
  m_treeList->DeleteAllItems();
  m_bzCurrentOccurrence.clear();
  m_stemCurrentOccurrence.clear();

  // Reset text highlighting
  m_textBox->SetStyle(0, m_textBox->GetValue().length(), m_neutralStyle);

  // Update display
  Timer t_fillListTree;
  fillListTree();
  std::cout << "Time for fillListTree: " << t_fillListTree.elapsed() << " milliseconds\n";

  Timer t_findUnnumberedWords;
  findUnnumberedWords();
  std::cout << "Time for finding unnumbered words: " << t_findUnnumberedWords.elapsed() << " milliseconds\n";

  Timer t_checkArticleUsage;
  checkArticleUsage();
  std::cout << "Time for checking articles: " << t_checkArticleUsage.elapsed() << " milliseconds\n";

  // Sort the positions of all the errors and remove any duplicate entries
  Timer t_sortErrors;
  std::sort(m_allErrorsPositions.begin(), m_allErrorsPositions.end());
  auto last = std::unique(m_allErrorsPositions.begin(), m_allErrorsPositions.end());
  m_allErrorsPositions.erase(last, m_allErrorsPositions.end());
  std::cout << "Time for error sort: " << t_sortErrors.elapsed() << " milliseconds\n";

  // Update navigation labels
  m_allErrorsLabel->SetLabel(
      L"0/" + std::to_wstring(m_allErrorsPositions.size()) + L"\t");
  m_noNumberLabel->SetLabel(
      L"0/" + std::to_wstring(m_noNumberPositions.size()) + L"\t");
  m_wrongTermBzLabel->SetLabel(
      L"0/" + std::to_wstring(m_wrongTermBzPositions.size()) + L"\t");
  m_wrongArticleLabel->SetLabel(
      L"0/" + std::to_wstring(m_wrongArticlePositions.size()) + L"\t");

  // Refresh layout to accommodate label size changes
  Layout();

  Timer t_fillBzList;
  fillBzList();
  std::cout << "Time for fillBzList: " << t_fillBzList.elapsed() << " milliseconds\n";

  Timer t_fillTermList;
  fillTermList();
  std::cout << "Time for fillTermList: " << t_fillTermList.elapsed() << " milliseconds\n\n";

  m_textBox->EndSuppressUndo();
  // wxWindowUpdateLocker automatically "thaws" the window when it goes out of scope
}

void MainWindow::fillListTree() {
  for (const auto &[bz, stems] : m_ctx.db.bzToStems) {
    wxTreeListItem item;

    // Check if error has been cleared by user
    bool isCleared = m_ctx.clearedErrors.count(bz) > 0;

    if (isCleared || isUniquelyAssigned(bz)) {
      // Use check icon (0) for cleared errors or no errors
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bz, 0, 0);
    } else {
      // Use warning icon (1) for actual errors
      item = m_treeList->AppendItem(m_treeList->GetRootItem(), bz, 1, 1);
    }

    // Build display string from first occurrence of each stem
    std::wstring displayText;
    for (const auto& stem : stems) {
      std::wstring firstWord = getFirstOccurrenceWord(stem);
      if (!firstWord.empty()) {
        if (!displayText.empty()) {
          displayText += L"; ";
        }
        displayText += firstWord;
      }
    }

    m_treeList->SetItemText(item, 1, displayText);
  }
}

bool MainWindow::isUniquelyAssigned(const std::wstring &bz) {
  return ErrorDetectorHelper::isUniquelyAssigned(
      bz, m_ctx, m_textBox, m_conflictStyle,
      m_wrongTermBzPositions, m_allErrorsPositions);
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
  m_termList->SetImageList(m_imageList.get());
}

void MainWindow::findUnnumberedWords() {
  ErrorDetectorHelper::findUnnumberedWords(
      m_fullText, *m_currentAnalyzer, m_wordRegex, m_ctx, m_textBox, m_warningStyle,
      m_noNumberPositions, m_allErrorsPositions);
}

void MainWindow::checkArticleUsage() {
  ErrorDetectorHelper::checkArticleUsage(
      m_fullText, *m_currentAnalyzer, m_ctx, m_textBox,
      m_articleWarningStyle, m_wrongArticlePositions, m_allErrorsPositions);
}


void MainWindow::selectNextAllError(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_allErrorsPositions, m_allErrorsSelected,
                             m_textBox, m_allErrorsLabel.get());
}

void MainWindow::selectPreviousAllError(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_allErrorsPositions, m_allErrorsSelected,
                                 m_textBox, m_allErrorsLabel.get());
}

void MainWindow::selectNextNoNumber(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_noNumberPositions, m_noNumberSelected, m_textBox,
                             m_noNumberLabel.get());
}

void MainWindow::selectPreviousNoNumber(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_noNumberPositions, m_noNumberSelected,
                                 m_textBox, m_noNumberLabel.get());
}

void MainWindow::selectNextWrongTermBz(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_wrongTermBzPositions, m_wrongTermBzSelected,
                             m_textBox, m_wrongTermBzLabel.get());
}

void MainWindow::selectPreviousWrongTermBz(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_wrongTermBzPositions, m_wrongTermBzSelected,
                                 m_textBox, m_wrongTermBzLabel.get());
}

void MainWindow::selectNextWrongArticle(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_wrongArticlePositions, m_wrongArticleSelected,
                             m_textBox, m_wrongArticleLabel.get());
}

void MainWindow::selectPreviousWrongArticle(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_wrongArticlePositions,
                                 m_wrongArticleSelected, m_textBox,
                                 m_wrongArticleLabel.get());
}

void MainWindow::setupUi() {
  // Build UI using UIBuilder
  auto components = UIBuilder::buildUI(this);

  // Assign components to member variables
  m_notebookList = components.notebookList;
  m_textBox = components.textBox;
  m_languageSelector = components.languageSelector;
  m_bzList = components.bzList;
  m_termList = components.termList;
  m_treeList = components.treeList;

  m_buttonForwardAllErrors = components.buttonForwardAllErrors;
  m_buttonBackwardAllErrors = components.buttonBackwardAllErrors;
  m_allErrorsLabel = components.allErrorsLabel;

  m_buttonForwardNoNumber = components.buttonForwardNoNumber;
  m_buttonBackwardNoNumber = components.buttonBackwardNoNumber;
  m_noNumberLabel = components.noNumberLabel;

  m_buttonForwardWrongTermBz = components.buttonForwardWrongTermBz;
  m_buttonBackwardWrongTermBz = components.buttonBackwardWrongTermBz;
  m_wrongTermBzLabel = components.wrongTermBzLabel;

  m_buttonForwardWrongArticle = components.buttonForwardWrongArticle;
  m_buttonBackwardWrongArticle = components.buttonBackwardWrongArticle;
  m_wrongArticleLabel = components.wrongArticleLabel;

  // Set up text styles
  m_neutralStyle.SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
  m_warningStyle.SetBackgroundColour(*wxYELLOW);
  m_conflictStyle.SetBackgroundColour(wxColour(255, 165, 0));  // Orange
  m_articleWarningStyle.SetBackgroundColour(*wxCYAN);

  // Create menu bar if it doesn't exist
  wxMenuBar* menuBar = GetMenuBar();
  if (!menuBar) {
      menuBar = new wxMenuBar();
      SetMenuBar(menuBar);
  }

  // Help menu
  wxMenu* helpMenu = new wxMenu();
  helpMenu->Append(wxID_ABOUT, wxT("&About"));
  menuBar->Append(helpMenu, wxT("&Help"));
}

void MainWindow::setupBindings() {
  // Menu bar bindings
  Bind(wxEVT_MENU, &MainWindow::onAbout, this, wxID_ABOUT);

  m_textBox->Bind(wxEVT_TEXT, &MainWindow::debounceFunc, this);
  m_debounceTimer.Bind(wxEVT_TIMER, &MainWindow::scanText, this);

  m_buttonBackwardAllErrors->Bind(wxEVT_BUTTON,
                                  &MainWindow::selectPreviousAllError, this);
  m_buttonForwardAllErrors->Bind(wxEVT_BUTTON, &MainWindow::selectNextAllError,
                                 this);
  m_buttonBackwardNoNumber->Bind(wxEVT_BUTTON,
                                 &MainWindow::selectPreviousNoNumber, this);
  m_buttonForwardNoNumber->Bind(wxEVT_BUTTON, &MainWindow::selectNextNoNumber,
                                this);
  m_buttonBackwardWrongTermBz->Bind(
      wxEVT_BUTTON, &MainWindow::selectPreviousWrongTermBz, this);
  m_buttonForwardWrongTermBz->Bind(wxEVT_BUTTON,
                                   &MainWindow::selectNextWrongTermBz, this);
  m_buttonBackwardWrongArticle->Bind(
      wxEVT_BUTTON, &MainWindow::selectPreviousWrongArticle, this);
  m_buttonForwardWrongArticle->Bind(wxEVT_BUTTON,
                                    &MainWindow::selectNextWrongArticle, this);

  m_treeList->Bind(wxEVT_TREELIST_ITEM_CONTEXT_MENU,
                   &MainWindow::onTreeListContextMenu, this);
  m_treeList->Bind(wxEVT_TREELIST_ITEM_ACTIVATED,
                   &MainWindow::onTreeListItemActivated, this);

  m_termList->Bind(wxEVT_TREELIST_ITEM_CONTEXT_MENU,
                   &MainWindow::onTermListContextMenu, this);
  m_termList->Bind(wxEVT_TREELIST_ITEM_ACTIVATED,
                   &MainWindow::onTermListItemActivated, this);

  // Text right-click for clearing errors
  m_textBox->Bind(wxEVT_RIGHT_DOWN, &MainWindow::onTextRightClick, this);
  
  // Menu bar handlers
  Bind(wxEVT_MENU, &MainWindow::onOpenImage, this, wxID_HIGHEST + 30);
  Bind(wxEVT_MENU, &MainWindow::onRestoreAllErrors, this, wxID_HIGHEST + 20);
  Bind(wxEVT_MENU, &MainWindow::onRestoreTextboxErrors, this, wxID_HIGHEST + 21);
  Bind(wxEVT_MENU, &MainWindow::onRestoreOverviewErrors, this, wxID_HIGHEST + 22);
  
  
  // Language selector
  m_languageSelector->Bind(wxEVT_RADIOBOX, &MainWindow::onLanguageChanged, this);
}

void MainWindow::fillBzList() {
  m_bzList->SetValue("");

  auto treeItem = m_treeList->GetFirstItem();
  while (treeItem.IsOk()) {
    std::wstring bz = m_treeList->GetItemText(treeItem, 0).ToStdWstring();

    if (m_ctx.db.bzToPositions.count(bz) && m_ctx.db.bzToPositions[bz].size() >= 1) {
      size_t start = m_ctx.db.bzToPositions[bz][0].first;
      size_t len = m_ctx.db.bzToPositions[bz][0].second;

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

  // Lock mutex to safely access shared data
  std::lock_guard<std::mutex> lock(m_dataMutex);

  // Get the stems for this BZ to determine the base word
  if (m_ctx.db.bzToStems.count(bz) && !m_ctx.db.bzToStems[bz].empty()) {
    // Get the first stem vector
    const StemVector &firstStem = *m_ctx.db.bzToStems[bz].begin();

    if (firstStem.empty()) {
      return;
    }

    // The base stem is the last element (for single-word: the only element,
    // for multi-word: the second word like "lager")
    std::wstring baseStem = firstStem.back();

    // Create context menu
    wxMenu menu;
    
    // Use proper wxWidgets menu IDs (avoid conflicts with system IDs on Windows)
    const int ID_MULTIWORD = wxID_HIGHEST + 1;
    const int ID_CLEAR_ERROR = wxID_HIGHEST + 2;
    
    bool isMultiWord = m_ctx.multiWordBaseStems.count(baseStem) > 0;
    menu.Append(ID_MULTIWORD, isMultiWord ? "Disable multi-word mode"
                                          : "Enable multi-word mode");

    // Check if this BZ actually has an error (ignoring cleared status)
    const auto &stems = m_ctx.db.bzToStems[bz];
    bool hasError = false;

    // Check if multiple different stems are assigned to this BZ
    if (stems.size() > 1) {
      hasError = true;
    }

    // Check if the stem is also used with other BZs
    if (!hasError) {
      for (const auto &stem : stems) {
        if (m_ctx.db.stemToBz.at(stem).size() > 1) {
          hasError = true;
          break;
        }
      }
    }

    // Only show clear/restore error option if there's an actual error
    if (hasError) {
      bool isCleared = m_ctx.clearedErrors.count(bz) > 0;
      menu.Append(ID_CLEAR_ERROR, isCleared ? "Restore error" : "Clear error");
    }

    int selection = GetPopupMenuSelectionFromUser(menu);
    if (selection == ID_MULTIWORD) {
      toggleMultiWordTerm(baseStem);
    } else if (selection == ID_CLEAR_ERROR) {
      clearError(bz);
    }
  }
}

void MainWindow::onTermListContextMenu(wxTreeListEvent &event) {
  wxTreeListItem item = event.GetItem();
  if (!item.IsOk()) {
    return;
  }

  // Get BZs from the second column
  wxString bzListText = m_termList->GetItemText(item, 1);
  std::wstring bzListStr = bzListText.ToStdWstring();
  
  // Parse BZs
  std::vector<std::wstring> bzs;
  size_t start = 0;
  size_t end = bzListStr.find(L", ");
  while (end != std::wstring::npos) {
    bzs.push_back(bzListStr.substr(start, end - start));
    start = end + 2; // skip ", "
    end = bzListStr.find(L", ", start);
  }
  bzs.push_back(bzListStr.substr(start));

  if (bzs.empty()) return;

  // Create menu
  wxMenu menu;
  
  // Lock mutex to safely access shared data
  std::lock_guard<std::mutex> lock(m_dataMutex);

  int idCounter = 0;
  const int BASE_ID = wxID_HIGHEST + 100;

  for (const auto& bz : bzs) {
    if (bz.empty()) continue;

    bool isCleared = m_ctx.clearedErrors.count(bz) > 0;
    
    // Check if there is an error associated with this BZ
    bool hasError = false;
    if (bzs.size() > 1) {
        hasError = true;
    } else {
        if (!isUniquelyAssigned(bz)) {
            hasError = true;
        }
    }
    
    if (hasError || isCleared) {
        wxString label = isCleared ? wxString::Format("Restore error for '%s'", bz) 
                                   : wxString::Format("Clear error for '%s'", bz);
        menu.Append(BASE_ID + idCounter, label);
    }
    idCounter++;
  }

  if (menu.GetMenuItemCount() > 0) {
      int selection = GetPopupMenuSelectionFromUser(menu);
      if (selection >= BASE_ID && selection < BASE_ID + idCounter) {
          int index = selection - BASE_ID;
          if (index >= 0 && index < static_cast<int>(bzs.size())) {
              clearError(bzs[index]);
          }
      }
  }
}

void MainWindow::toggleMultiWordTerm(const std::wstring &baseStem) {
  bool currentlyActive = m_ctx.multiWordBaseStems.count(baseStem) > 0;

  if (currentlyActive) {
    // User is DISABLING multi-word mode
    m_ctx.manualMultiWordToggles.erase(baseStem);          // Remove from manual enables
    m_ctx.manuallyDisabledMultiWord.insert(baseStem);      // Add to manual disables

    // Also remove from auto-detected (so rebuild logic doesn't re-add it)
    m_ctx.autoDetectedMultiWordStems.erase(baseStem);
  } else {
    // User is ENABLING multi-word mode
    m_ctx.manuallyDisabledMultiWord.erase(baseStem);       // Remove from manual disables
    m_ctx.manualMultiWordToggles.insert(baseStem);         // Add to manual enables
  }

  // Trigger rescan
  m_debounceTimer.Start(1, true);
}

void MainWindow::clearError(const std::wstring &bz) {
  if (m_ctx.clearedErrors.count(bz)) {
    // Restore error - remove from cleared set
    m_ctx.clearedErrors.erase(bz);
  } else {
    // Clear error - add to cleared set
    m_ctx.clearedErrors.insert(bz);
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

  // Lock mutex to safely access shared data
  std::lock_guard<std::mutex> lock(m_dataMutex);

  // Check if this BZ has any positions
  if (m_ctx.db.bzToPositions.count(bz) && !m_ctx.db.bzToPositions[bz].empty()) {
    const auto &positions = m_ctx.db.bzToPositions[bz];

    // Get current occurrence index for this BZ (or initialize based on cursor
    // position)
    if (!m_bzCurrentOccurrence.count(bz)) {
      // Get current cursor position in the text
      long cursorPos = m_textBox->GetInsertionPoint();

      // Find the occurrence closest to or after the cursor position
      int closestIdx = 0;

      // If cursor is past all occurrences, start from the beginning
      if (cursorPos >
          static_cast<long>(positions[positions.size() - 1].second)) {
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

void MainWindow::onTermListItemActivated(wxTreeListEvent &event) {
  wxTreeListItem item = event.GetItem();
  if (!item.IsOk()) {
    return;
  }

  wxString termText = m_termList->GetItemText(item, 0);
  std::wstring termWord = termText.ToStdWstring();

  // Lock mutex to safely access shared data
  std::lock_guard<std::mutex> lock(m_dataMutex);

  // Find the stem for this term word
  StemVector foundStem;
  bool stemFound = false;

  for (const auto& [stem, firstWord] : m_ctx.db.stemToFirstWord) {
    if (firstWord == termWord) {
      foundStem = stem;
      stemFound = true;
      break;
    }
  }

  if (!stemFound) {
    return;
  }

  // Check if this stem has any positions
  if (m_ctx.db.stemToPositions.count(foundStem) && !m_ctx.db.stemToPositions[foundStem].empty()) {
    const auto &positions = m_ctx.db.stemToPositions[foundStem];

    // Get current occurrence index for this stem (or initialize based on cursor position)
    if (!m_stemCurrentOccurrence.count(foundStem)) {
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

      m_stemCurrentOccurrence[foundStem] = closestIdx;
    }

    int &currentIdx = m_stemCurrentOccurrence[foundStem];

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

void MainWindow::onTextRightClick(wxMouseEvent &event) {
  // Convert mouse position to text position
  wxPoint mousePos = event.GetPosition();
  wxTextCoord col, row;
  wxTextCtrlHitTestResult hitTest = m_textBox->HitTest(mousePos, &col, &row);
  
  if (hitTest == wxTE_HT_UNKNOWN || hitTest == wxTE_HT_BEYOND) {
    event.Skip();
    return;
  }
  
  // Convert row/col to character position
  long clickPos = m_textBox->XYToPosition(col, row);
  
  // Check if we're on a highlighted error
  size_t errorStart = 0, errorEnd = 0;
  bool foundError = false;
  
  // Check all error position vectors
  auto checkPositions = [&](const std::vector<std::pair<int, int>>& positions) {
    for (const auto& [start, end] : positions) {
      if (clickPos >= start && clickPos < end) {
        errorStart = start;
        errorEnd = end;
        return true;
      }
    }
    return false;
  };
  
  // all errors should be in m_allErrorsPositions so we don't need to check others separately
  if (checkPositions(m_allErrorsPositions)) {
    foundError = true;
  }
  
  if (foundError) {
    wxMenu menu;
    const int ID_CLEAR_TEXT_ERROR = wxID_HIGHEST + 10;
    menu.Append(ID_CLEAR_TEXT_ERROR, "Clear error");
    
    int selection = GetPopupMenuSelectionFromUser(menu, event.GetPosition());
    if (selection == ID_CLEAR_TEXT_ERROR) {
      clearTextError(errorStart, errorEnd);
    }
  } else {
    event.Skip(); // Let other handlers process the event
  }
}

void MainWindow::clearTextError(size_t start, size_t end) {
  // Add to cleared positions
  m_ctx.clearedTextPositions.insert({start, end});
  
  // Trigger rescan to update highlighting
  m_debounceTimer.Start(1, true);
}

bool MainWindow::isPositionCleared(size_t start, size_t end) const {
  return ErrorDetectorHelper::isPositionCleared(m_ctx.clearedTextPositions, start, end);
}

void MainWindow::onRestoreTextboxErrors(wxCommandEvent &event) {
  m_ctx.clearedTextPositions.clear();
  m_debounceTimer.Start(1, true);
}

void MainWindow::onRestoreOverviewErrors(wxCommandEvent &event) {
  m_ctx.clearedErrors.clear();
  m_debounceTimer.Start(1, true);
}

void MainWindow::onRestoreAllErrors(wxCommandEvent &event) {
  m_ctx.clearedTextPositions.clear();
  m_ctx.clearedErrors.clear();
  m_debounceTimer.Start(1, true);
}


void MainWindow::onLanguageChanged(wxCommandEvent &event) {
  // Update language selection
  if (m_languageSelector->GetSelection() == 0) {
      m_currentAnalyzer = std::make_unique<GermanTextAnalyzer>();
  } else {
      m_currentAnalyzer = std::make_unique<EnglishTextAnalyzer>();
  }

  // Clear auto-detected stems (language-specific)
  m_ctx.autoDetectedMultiWordStems.clear();

  // Rebuild combined set with just manual toggles
  m_ctx.multiWordBaseStems = m_ctx.manualMultiWordToggles;

  // Trigger rescan with new language
  m_debounceTimer.Start(1, true);
}

void MainWindow::onAbout(wxCommandEvent &event) {
  wxString aboutText = wxT(
    "Bezugszeichenprüfvorrichtung\n"
    "Reference Number Verification Tool\n\n"
    "Version 0.5 (Dec 16, 2025)\n\n"
    "A utility for validating reference numbers in patent applications.\n"
    "Automatically checks that technical terms are consistently numbered\n"
    "throughout a document with support for German and English.\n\n"
    "Features:\n"
    "• Bilingual support (German/English)\n"
    "• Smart stemming for plurals and cases\n"
    "• Multi-word term detection\n"
    "• Error highlighting and navigation\n"
    "• Automatic ordinal pattern detection\n\n"
    "Credits & Libraries:\n"
    "• wxWidgets - Cross-platform GUI\n"
    "• Google RE2 - Regular expressions\n"
    "• Oleander Stemming - Language analysis\n"
    "• Google Test - Unit testing\n"
    "• Abseil - C++ utilities\n"
  );

  wxMessageBox(aboutText, wxT("About Bezugszeichenprüfvorrichtung"),
               wxOK | wxICON_INFORMATION);
}

void MainWindow::onOpenImage(wxCommandEvent &event) {
  if (!m_imageViewer) {
    m_imageViewer = new ImageViewerWindow(this);
  }
  m_imageViewer->Show();
  m_imageViewer->Raise();

  // Trigger the file open dialog
  wxCommandEvent openEvent(wxEVT_MENU, wxID_OPEN);
  m_imageViewer->ProcessWindowEvent(openEvent);
}

std::wstring MainWindow::getFirstOccurrenceWord(const StemVector& stem) const {
  if (!m_ctx.db.stemToPositions.count(stem) || m_ctx.db.stemToPositions.at(stem).empty()) {
    return L"";
  }

  const auto& positions = m_ctx.db.stemToPositions.at(stem);
  size_t firstStart = positions[0].first;
  size_t firstLen = positions[0].second;

  std::wstring fullMatch = m_fullText.substr(firstStart, firstLen);

  // Extract word before BZ number
  size_t bzStart = fullMatch.find_last_of(L' ');
  if (bzStart != std::wstring::npos) {
    return fullMatch.substr(0, bzStart);
  }

  return fullMatch;
}

void MainWindow::fillTermList() {
  m_termList->DeleteAllItems();

  // Collect all stems sorted by first occurrence position
  struct StemInfo {
    StemVector stem;
    size_t firstPosition;
    std::wstring firstWord;
    std::unordered_set<std::wstring> bzs;
  };

  std::vector<StemInfo> stemInfos;

  for (const auto& [stem, bzSet] : m_ctx.db.stemToBz) {
    StemInfo info;
    info.stem = stem;
    info.bzs = bzSet;

    // Get first position and word
    if (m_ctx.db.stemToPositions.count(stem) && !m_ctx.db.stemToPositions.at(stem).empty()) {
      info.firstPosition = m_ctx.db.stemToPositions.at(stem)[0].first;

      if (m_ctx.db.stemToFirstWord.count(stem)) {
        info.firstWord = m_ctx.db.stemToFirstWord.at(stem);
      }
    } else {
      info.firstPosition = SIZE_MAX;
    }

    stemInfos.push_back(info);
  }

  // Sort by first occurrence position
  std::sort(stemInfos.begin(), stemInfos.end(),
            [](const StemInfo& a, const StemInfo& b) {
              return a.firstPosition < b.firstPosition;
            });

  // Populate tree control
  for (const auto& info : stemInfos) {
    // Sort BZs numerically
    std::vector<std::wstring> sortedBzs(info.bzs.begin(), info.bzs.end());
    std::sort(sortedBzs.begin(), sortedBzs.end(), BZComparatorForMap());

    std::wstring bzListStr;
    for (size_t i = 0; i < sortedBzs.size(); ++i) {
      if (i > 0) bzListStr += L", ";
      bzListStr += sortedBzs[i];
    }

    // Check if there's a conflict:
    // 1. Term maps to multiple BZs (info.bzs.size() > 1)
    // 2. Any BZ this term is assigned to is also assigned to other terms
    bool hasError = false;
    if (info.bzs.size() > 1) {
      hasError = true;  // One term -> multiple BZs
    } else {
      // Check if the single BZ is assigned to multiple terms
      for (const auto& bz : info.bzs) {
        if (!isUniquelyAssigned(bz)) {
          hasError = true;
          break;
        }
      }
    }

    // Create tree item with appropriate icon
    wxTreeListItem item;
    if (hasError) {
      // Warning icon (1) for conflicts
      item = m_termList->AppendItem(m_termList->GetRootItem(), info.firstWord, 1, 1);
    } else {
      // Check icon (0) for no conflicts
      item = m_termList->AppendItem(m_termList->GetRootItem(), info.firstWord, 0, 0);
    }

    // Set the BZ list in the second column
    m_termList->SetItemText(item, 1, bzListStr);
  }
}
