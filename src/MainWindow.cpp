#include "MainWindow.h"
#include "RegexPatterns.h"
#include "EnglishTextAnalyzer.h"
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include "ErrorNavigator.h"
#include "TextScanner.h"
#include "ErrorDetectorHelper.h"
#include "UIBuilder.h"
#include "utils.h"
#include "wx/event.h"
#include "wx/gdicmn.h"
#include "wx/timer.h"
#include "wx/wupdlock.h"
#include <algorithm>
#include <locale>
#include <string>
#include <wx/bitmap.h>
#include "TimerHelper.h"

// Static member definitions - analyzers for different languages
GermanTextAnalyzer MainWindow::s_germanAnalyzer;
EnglishTextAnalyzer MainWindow::s_englishAnalyzer;
bool MainWindow::s_useGerman = true; // Default to German

// Helper method implementations - dispatch to correct analyzer based on language
StemVector MainWindow::createCurrentStemVector(std::wstring word) {
  if (s_useGerman) {
    return s_germanAnalyzer.createStemVector(std::move(word));
  } else {
    return s_englishAnalyzer.createStemVector(std::move(word));
  }
}

StemVector MainWindow::createCurrentMultiWordStemVector(std::wstring firstWord, std::wstring secondWord) {
  if (s_useGerman) {
    return s_germanAnalyzer.createMultiWordStemVector(std::move(firstWord), std::move(secondWord));
  } else {
    return s_englishAnalyzer.createMultiWordStemVector(std::move(firstWord), std::move(secondWord));
  }
}

bool MainWindow::isCurrentMultiWordBase(std::wstring word, const std::unordered_set<std::wstring>& multiWordBaseStems) {
  if (s_useGerman) {
    return s_germanAnalyzer.isMultiWordBase(std::move(word), multiWordBaseStems);
  } else {
    return s_englishAnalyzer.isMultiWordBase(std::move(word), multiWordBaseStems);
  }
}

bool MainWindow::isCurrentIndefiniteArticle(const std::wstring& word) {
  if (s_useGerman) {
    return GermanTextAnalyzer::isIndefiniteArticle(word);
  } else {
    return EnglishTextAnalyzer::isIndefiniteArticle(word);
  }
}

bool MainWindow::isCurrentDefiniteArticle(const std::wstring& word) {
  if (s_useGerman) {
    return GermanTextAnalyzer::isDefiniteArticle(word);
  } else {
    return EnglishTextAnalyzer::isDefiniteArticle(word);
  }
}

std::pair<std::wstring, size_t> MainWindow::findCurrentPrecedingWord(const std::wstring& text, size_t pos) {
  if (s_useGerman) {
    return GermanTextAnalyzer::findPrecedingWord(text, pos);
  } else {
    return EnglishTextAnalyzer::findPrecedingWord(text, pos);
  }
}

bool MainWindow::isCurrentIgnoredWord(const std::wstring& word) {
  if (s_useGerman) {
    return GermanTextAnalyzer::isIgnoredWord(word);
  } else {
    return EnglishTextAnalyzer::isIgnoredWord(word);
  }
}



MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY,
              wxString::FromUTF8("Bezugszeichenprüfvorrichtung"),
              wxDefaultPosition, wxSize(1200, 800)),
      // Initialize RE2 patterns using shared constants
      m_singleWordRegex(RegexPatterns::SINGLE_WORD_PATTERN),
      m_twoWordRegex(RegexPatterns::TWO_WORD_PATTERN),
      m_wordRegex(RegexPatterns::WORD_PATTERN) {
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

MainWindow::~MainWindow() {
#ifdef HAVE_OPENCV
  // Wait for OCR thread to complete if it's running
  if (m_ocrThread && m_ocrThread->joinable()) {
    std::cout << "Waiting for OCR thread to complete..." << std::endl;
    m_ocrThread->join();
  }
#endif
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
  // Clear all data structures
  m_bzToStems.clear();
  m_stemToBz.clear();
  m_bzToOriginalWords.clear();
  m_bzToPositions.clear();
  m_stemToPositions.clear();

  m_allErrorsPositions.clear();
  m_noNumberPositions.clear();
  m_wrongNumberPositions.clear();
  m_splitNumberPositions.clear();
  m_wrongArticlePositions.clear();

  std::cout << "Time for setup and clearing: " << t_setup.elapsed() << " milliseconds\n";

  if (m_cancelScan) {
    return;
  }

  // Scan text for patterns using TextScanner
  Timer t_scan;
  TextScanner::scanText(m_fullText, m_singleWordRegex, m_twoWordRegex,
                        m_multiWordBaseStems, m_clearedTextPositions,
                        m_bzToStems, m_stemToBz, m_bzToOriginalWords,
                        m_bzToPositions, m_stemToPositions);
  std::cout << "Time for TextScanner::scanText: " << t_scan.elapsed() << " milliseconds\n";

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
  m_wrongNumberLabel->SetLabel(
      L"0/" + std::to_wstring(m_wrongNumberPositions.size()) + L"\t");
  m_splitNumberLabel->SetLabel(
      L"0/" + std::to_wstring(m_splitNumberPositions.size()) + L"\t");
  m_wrongArticleLabel->SetLabel(
      L"0/" + std::to_wstring(m_wrongArticlePositions.size()) + L"\t");

  // Refresh layout to accommodate label size changes
  Layout();

  Timer t_fillBzList;
  fillBzList();
  std::cout << "Time for fillBzList: " << t_fillBzList.elapsed() << " milliseconds\n\n";

  m_textBox->EndSuppressUndo();
  // wxWindowUpdateLocker automatically "thaws" the window when it goes out of scope
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
  return ErrorDetectorHelper::isUniquelyAssigned(
      bz, m_bzToStems, m_stemToBz, m_bzToPositions, m_stemToPositions,
      m_clearedErrors, m_clearedTextPositions, m_textBox, m_warningStyle,
      m_wrongNumberPositions, m_splitNumberPositions, m_allErrorsPositions);
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
  ErrorDetectorHelper::findUnnumberedWords(
      m_fullText, m_wordRegex, m_multiWordBaseStems, m_stemToPositions,
      m_stemToBz, m_clearedTextPositions, m_textBox, m_warningStyle,
      m_noNumberPositions, m_allErrorsPositions);
}

void MainWindow::checkArticleUsage() {
  ErrorDetectorHelper::checkArticleUsage(
      m_fullText, m_stemToPositions, m_clearedTextPositions, m_textBox,
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

void MainWindow::selectNextWrongNumber(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_wrongNumberPositions, m_wrongNumberSelected,
                             m_textBox, m_wrongNumberLabel.get());
}

void MainWindow::selectPreviousWrongNumber(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_wrongNumberPositions, m_wrongNumberSelected,
                                 m_textBox, m_wrongNumberLabel.get());
}

void MainWindow::selectNextSplitNumber(wxCommandEvent &event) {
  ErrorNavigator::selectNext(m_splitNumberPositions, m_splitNumberSelected,
                             m_textBox, m_splitNumberLabel.get());
}

void MainWindow::selectPreviousSplitNumber(wxCommandEvent &event) {
  ErrorNavigator::selectPrevious(m_splitNumberPositions, m_splitNumberSelected,
                                 m_textBox, m_splitNumberLabel.get());
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
  m_treeList = components.treeList;

  m_buttonForwardAllErrors = components.buttonForwardAllErrors;
  m_buttonBackwardAllErrors = components.buttonBackwardAllErrors;
  m_allErrorsLabel = components.allErrorsLabel;

  m_buttonForwardNoNumber = components.buttonForwardNoNumber;
  m_buttonBackwardNoNumber = components.buttonBackwardNoNumber;
  m_noNumberLabel = components.noNumberLabel;

  m_buttonForwardWrongNumber = components.buttonForwardWrongNumber;
  m_buttonBackwardWrongNumber = components.buttonBackwardWrongNumber;
  m_wrongNumberLabel = components.wrongNumberLabel;

  m_buttonForwardSplitNumber = components.buttonForwardSplitNumber;
  m_buttonBackwardSplitNumber = components.buttonBackwardSplitNumber;
  m_splitNumberLabel = components.splitNumberLabel;

  m_buttonForwardWrongArticle = components.buttonForwardWrongArticle;
  m_buttonBackwardWrongArticle = components.buttonBackwardWrongArticle;
  m_wrongArticleLabel = components.wrongArticleLabel;

  // Image viewer components
  m_splitter = components.splitter;
  m_imagePanel = components.imagePanel;
  m_imageViewer = components.imageViewer;
  m_imageInfoText = components.imageInfoText;

  // Image navigation components
  m_buttonPreviousImage = components.buttonPreviousImage;
  m_buttonNextImage = components.buttonNextImage;
  m_imageNavigationLabel = components.imageNavigationLabel;

  // Right panel notebook
  m_rightNotebook = components.rightNotebook;

#ifdef HAVE_OPENCV
  // OCR components
  m_ocrPanel = components.ocrPanel;
  m_ocrResultsList = components.ocrResultsList;
  m_buttonScanOCR = components.buttonScanOCR;
  m_ocrStatusLabel = components.ocrStatusLabel;
#endif

  // Set up text styles
  m_neutralStyle.SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
  m_warningStyle.SetBackgroundColour(*wxYELLOW);
  m_articleWarningStyle.SetBackgroundColour(*wxCYAN);
}

void MainWindow::setupBindings() {
  m_textBox->Bind(wxEVT_TEXT, &MainWindow::debounceFunc, this);
  m_debounceTimer.Bind(wxEVT_TIMER, &MainWindow::scanText, this);

  // Image navigation buttons
  m_buttonPreviousImage->Bind(wxEVT_BUTTON, &MainWindow::onPreviousImage, this);
  m_buttonNextImage->Bind(wxEVT_BUTTON, &MainWindow::onNextImage, this);

  // Image panel resize event
  m_imagePanel->Bind(wxEVT_SIZE, &MainWindow::onImagePanelResize, this);

#ifdef HAVE_OPENCV
  // OCR scan button
  m_buttonScanOCR->Bind(wxEVT_BUTTON, &MainWindow::onScanOCR, this);
  // OCR thread completion event
  Bind(wxEVT_THREAD, &MainWindow::onOCRThreadComplete, this);
  // OCR loading animation timer
  m_ocrLoadingTimer.Bind(wxEVT_TIMER, &MainWindow::onOCRLoadingAnimation, this);
#endif

  m_buttonBackwardAllErrors->Bind(wxEVT_BUTTON,
                                  &MainWindow::selectPreviousAllError, this);
  m_buttonForwardAllErrors->Bind(wxEVT_BUTTON, &MainWindow::selectNextAllError,
                                 this);
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
  
  // Text right-click for clearing errors
  m_textBox->Bind(wxEVT_RIGHT_DOWN, &MainWindow::onTextRightClick, this);
  
  // Menu bar handlers
  Bind(wxEVT_MENU, &MainWindow::onOpenImage, this, wxID_OPEN);
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

  // Lock mutex to safely access shared data
  std::lock_guard<std::mutex> lock(m_dataMutex);

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
    
    // Use proper wxWidgets menu IDs (avoid conflicts with system IDs on Windows)
    const int ID_MULTIWORD = wxID_HIGHEST + 1;
    const int ID_CLEAR_ERROR = wxID_HIGHEST + 2;
    
    bool isMultiWord = m_multiWordBaseStems.count(baseStem) > 0;
    menu.Append(ID_MULTIWORD, isMultiWord ? "Disable multi-word mode"
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

  // Lock mutex to safely access shared data
  std::lock_guard<std::mutex> lock(m_dataMutex);

  // Check if this BZ has any positions
  if (m_bzToPositions.count(bz) && !m_bzToPositions[bz].empty()) {
    const auto &positions = m_bzToPositions[bz];

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
  m_clearedTextPositions.insert({start, end});
  
  // Trigger rescan to update highlighting
  m_debounceTimer.Start(1, true);
}

bool MainWindow::isPositionCleared(size_t start, size_t end) const {
  return ErrorDetectorHelper::isPositionCleared(m_clearedTextPositions, start, end);
}

void MainWindow::onRestoreTextboxErrors(wxCommandEvent &event) {
  m_clearedTextPositions.clear();
  m_debounceTimer.Start(1, true);
}

void MainWindow::onRestoreOverviewErrors(wxCommandEvent &event) {
  m_clearedErrors.clear();
  m_debounceTimer.Start(1, true);
}

void MainWindow::onRestoreAllErrors(wxCommandEvent &event) {
  m_clearedTextPositions.clear();
  m_clearedErrors.clear();
  m_debounceTimer.Start(1, true);
}


void MainWindow::onLanguageChanged(wxCommandEvent &event) {
  // Update language selection
  s_useGerman = (m_languageSelector->GetSelection() == 0);

  // Trigger rescan with new language
  m_debounceTimer.Start(1, true);
}

void MainWindow::onOpenImage(wxCommandEvent &event) {
#ifdef HAVE_MUPDF
  wxFileDialog openFileDialog(
      this, "Open Image or PDF Files", "", "",
      "All Supported (*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.pdf)|*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.pdf|"
      "Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.gif)|*.png;*.jpg;*.jpeg;*.bmp;*.gif|"
      "PDF Files (*.pdf)|*.pdf",
      wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
#else
  wxFileDialog openFileDialog(
      this, "Open image file(s)", "", "",
      "Image files (*.png;*.jpg;*.jpeg;*.bmp;*.gif)|*.png;*.jpg;*.jpeg;*.bmp;*.gif",
      wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
#endif

  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return;

  // Clear previous images
  m_loadedImages.clear();
  m_imagePaths.clear();
  m_currentImageIndex = -1;

  // Load all selected files
  wxArrayString paths;
  openFileDialog.GetPaths(paths);

  for (const auto& path : paths) {
    loadImage(path);
  }

  // Display the first image
  if (!m_loadedImages.empty()) {
    m_currentImageIndex = 0;
    updateImageDisplay();
    updateImageNavigationButtons();
  }
}

void MainWindow::loadImage(const wxString &filePath) {
#ifdef HAVE_MUPDF
  // Check if file is PDF
  if (filePath.Lower().EndsWith(".pdf")) {
    loadPDF(filePath);
    return;
  }
#endif

  // Original image loading code
  wxImage image;
  if (!image.LoadFile(filePath)) {
    wxMessageBox("Failed to load image: " + filePath, "Error",
                 wxICON_ERROR | wxOK);
    return;
  }

  // Add image to collection
  wxBitmap bitmap(image);
  m_loadedImages.push_back(bitmap);
  m_imagePaths.push_back(filePath);
}

#ifdef HAVE_MUPDF
void MainWindow::loadPDF(const wxString &filePath) {
  std::vector<wxBitmap> pdfPages;
  wxString errorMsg;

  // Show busy cursor during loading (300 DPI can take 1-2 sec/page)
  wxBusyCursor wait;

  // Load PDF at 300 DPI
  if (!PDFLoader::loadPDF(filePath, 300, pdfPages, errorMsg)) {
    wxMessageBox("Failed to load PDF: " + errorMsg, "Error",
                 wxICON_ERROR | wxOK);
    return;
  }

  if (pdfPages.empty()) {
    wxMessageBox("PDF contains no pages: " + filePath, "Error",
                 wxICON_ERROR | wxOK);
    return;
  }

  // Add all pages to image collection
  for (size_t i = 0; i < pdfPages.size(); ++i) {
    m_loadedImages.push_back(pdfPages[i]);

    // Store path with page number for display
    wxString pathWithPage = wxString::Format("%s (Page %zu)",
                                              filePath, i + 1);
    m_imagePaths.push_back(pathWithPage);
  }
}
#endif

void MainWindow::updateImageDisplay() {
  if (m_currentImageIndex < 0 || m_currentImageIndex >= static_cast<int>(m_loadedImages.size())) {
    // No valid image to display
    m_imageViewer->Hide();
    m_imageInfoText->Show();
    m_buttonPreviousImage->Hide();
    m_buttonNextImage->Hide();
    m_imageNavigationLabel->Hide();
#ifdef HAVE_OPENCV
    m_buttonScanOCR->Hide();
#endif
    m_imagePanel->Layout();
    return;
  }

  // Hide the info text and show the image viewer
  m_imageInfoText->Hide();

  // Get original bitmap
  const wxBitmap& originalBitmap = m_loadedImages[m_currentImageIndex];

  // Get available space in the image panel (account for navigation buttons and margins)
  wxSize panelSize = m_imagePanel->GetClientSize();

  // Calculate actual navigation controls height
  int navHeight = m_buttonPreviousImage->GetSize().GetHeight() + 20; // button height + margins
  int margin = 40; // Additional safety margin for padding

  int availableHeight = panelSize.GetHeight() - navHeight - margin;
  int availableWidth = panelSize.GetWidth() - 40; // Account for horizontal margins


  // Skip scaling if panel is too small
  if (availableWidth < 50 || availableHeight < 50) {
    m_imageViewer->SetBitmap(originalBitmap);
    m_imageViewer->Show();
    m_imagePanel->Layout();
    return;
  }

  // Calculate scaling to fit while maintaining aspect ratio
  int origWidth = originalBitmap.GetWidth();
  int origHeight = originalBitmap.GetHeight();

  double scaleX = static_cast<double>(availableWidth) / origWidth;
  double scaleY = static_cast<double>(availableHeight) / origHeight;
  double scale = std::min(scaleX, scaleY);

  // Only scale down, not up (to avoid pixelation)
  scale = std::min(scale, 1.0);

  int newWidth = static_cast<int>(origWidth * scale);
  int newHeight = static_cast<int>(origHeight * scale);

  // Scale the bitmap
  wxImage scaledImage = originalBitmap.ConvertToImage();
  scaledImage.Rescale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH);
  wxBitmap scaledBitmap(scaledImage);

  // Display scaled image
  m_imageViewer->SetBitmap(scaledBitmap);
  m_imageViewer->Show();

  // Show navigation controls
  m_buttonPreviousImage->Show();
  m_buttonNextImage->Show();
  m_imageNavigationLabel->Show();
#ifdef HAVE_OPENCV
  m_buttonScanOCR->Show();
#endif

  // Update navigation label
  wxString label = wxString::Format("%d/%zu", m_currentImageIndex + 1, m_loadedImages.size());
  m_imageNavigationLabel->SetLabel(label);

  // Reset virtual size (no scrolling needed since we fit to window)
  m_imagePanel->SetVirtualSize(panelSize);
  m_imagePanel->Layout();
}

void MainWindow::updateImageNavigationButtons() {
  if (m_loadedImages.empty()) {
    m_buttonPreviousImage->Enable(false);
    m_buttonNextImage->Enable(false);
    return;
  }

  // Enable/disable buttons based on current position
  m_buttonPreviousImage->Enable(m_currentImageIndex > 0);
  m_buttonNextImage->Enable(m_currentImageIndex < static_cast<int>(m_loadedImages.size()) - 1);
}

void MainWindow::onPreviousImage(wxCommandEvent &event) {
  if (m_currentImageIndex > 0) {
    m_currentImageIndex--;
    updateImageDisplay();
    updateImageNavigationButtons();
  }
}

void MainWindow::onNextImage(wxCommandEvent &event) {
  if (m_currentImageIndex < static_cast<int>(m_loadedImages.size()) - 1) {
    m_currentImageIndex++;
    updateImageDisplay();
    updateImageNavigationButtons();
  }
}

void MainWindow::onImagePanelResize(wxSizeEvent &event) {
  // Update image display when panel is resized
  if (!m_loadedImages.empty() && m_currentImageIndex >= 0) {
    updateImageDisplay();
  }
  event.Skip(); // Allow default processing
}

#ifdef HAVE_OPENCV
void MainWindow::onScanOCR(wxCommandEvent &event) {
  // Initialize OCR engine lazily on first use
  if (!m_ocrEngine) {
    m_ocrEngine = std::make_unique<OCREngine>();
    if (!m_ocrEngine->initialize()) {
      wxMessageBox("Failed to initialize OCR engine: " + m_ocrEngine->getLastError(),
                   "OCR Error", wxICON_ERROR | wxOK, this);
      m_ocrEngine.reset();
      return;
    }
  }

  // Check if we have an image to scan
  if (m_currentImageIndex < 0 || m_loadedImages.empty()) {
    wxMessageBox("No image loaded to scan", "OCR Error", wxICON_ERROR | wxOK, this);
    return;
  }

  // Disable button during processing
  m_buttonScanOCR->Disable();

  // Show analysis in progress status
  m_ocrStatusLabel->SetLabel("🔍 Analyzing image...");
  m_ocrStatusLabel->Show();
  m_ocrResultsList->Hide();
  m_ocrStatusLabel->GetParent()->Layout();

  // Start loading animation
  m_ocrLoadingFrame = 0;
  m_ocrLoadingTimer.Start(200, wxTIMER_CONTINUOUS);

  // Copy current image and launch processing thread
  const wxBitmap& currentImage = m_loadedImages[m_currentImageIndex];

  // Wait for previous thread to complete if it exists
  if (m_ocrThread && m_ocrThread->joinable()) {
    m_ocrThread->join();
  }

  // Launch OCR processing on background thread
  m_ocrThread = std::make_unique<std::thread>(&MainWindow::ocrProcessingThread, this, currentImage);
}

void MainWindow::updateOCRResults(const std::vector<OCRResult>& results) {
  // Clear previous results
  m_ocrResultsList->DeleteAllItems();

  if (results.empty()) {
    // Show status label with no results message
    m_ocrStatusLabel->SetLabel("No reference numbers found in this image.\nTry scanning a different image.");
    m_ocrStatusLabel->Show();
    m_ocrResultsList->Hide();
  } else {
    // Hide status label and show results list
    m_ocrStatusLabel->Hide();
    m_ocrResultsList->Show();

      // Populate list with results
      // Lock mutex to safely access shared data (m_bzToStems)
      std::lock_guard<std::mutex> lock(m_dataMutex);

      for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        long itemIndex = m_ocrResultsList->InsertItem(i, wxString(result.text));
        wxString confStr = wxString::Format("%.0f%%", result.confidence * 100);
        m_ocrResultsList->SetItem(itemIndex, 1, confStr);

        wxString status;
        if (m_bzToStems.count(result.text)) {
          status = "Found in Text";
        } else {
          status = "New";
        }
        m_ocrResultsList->SetItem(itemIndex, 2, status);
      }
  }

  // Layout the OCR panel
  m_ocrPanel->Layout();
}

void MainWindow::ocrProcessingThread(const wxBitmap& image) {
  std::cout << "OCR processing thread started..." << std::endl;

  try {
    // Process image on background thread
    std::vector<OCRResult> results = m_ocrEngine->processImage(image);
    wxBitmap debugImage = m_ocrEngine->getDebugImage();

    // Store results in thread-safe manner
    {
      std::lock_guard<std::mutex> lock(m_ocrMutex);
      m_ocrThreadResults = results;
      m_ocrThreadDebugImage = debugImage;
    }

    std::cout << "OCR processing completed: " << results.size() << " results" << std::endl;

    // Send completion event to main thread
    wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
    event->SetInt(1); // Success flag
    wxQueueEvent(this, event);

  } catch (const std::exception& e) {
    std::cerr << "Error in OCR processing thread: " << e.what() << std::endl;

    // Send error event to main thread
    wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
    event->SetInt(0); // Error flag
    wxQueueEvent(this, event);
  }
}

void MainWindow::onOCRThreadComplete(wxThreadEvent &event) {
  std::cout << "OCR thread completion event received" << std::endl;

  // Stop loading animation
  m_ocrLoadingTimer.Stop();

  // Enable button
  m_buttonScanOCR->Enable();

  if (event.GetInt() == 1) {
    // Success - get results from thread-safe storage
    std::vector<OCRResult> results;
    wxBitmap debugImage;

    {
      std::lock_guard<std::mutex> lock(m_ocrMutex);
      results = m_ocrThreadResults;
      debugImage = m_ocrThreadDebugImage;
    }

    // Update results
    m_ocrResults = results;

    // Replace displayed image with debug image showing detection boxes
    if (debugImage.IsOk() && m_currentImageIndex >= 0 && m_currentImageIndex < (int)m_loadedImages.size()) {
      m_loadedImages[m_currentImageIndex] = debugImage;
      updateImageDisplay();
    }

    // Update results display
    updateOCRResults(m_ocrResults);

    // Switch to OCR tab to show results
    m_rightNotebook->SetSelection(1);

    std::cout << "OCR results updated in UI" << std::endl;
  } else {
    // Error occurred
    m_ocrStatusLabel->SetLabel("❌ Error during OCR analysis. Check console for details.");
    m_ocrStatusLabel->Show();
    m_ocrResultsList->Hide();
    m_ocrStatusLabel->GetParent()->Layout();
  }
}

void MainWindow::onOCRLoadingAnimation(wxTimerEvent &event) {
  // Spinning wheel animation frames
  const std::string spinnerFrames[] = {
      "🔍 Analyzing image ⠋",
      "🔍 Analyzing image ⠙",
      "🔍 Analyzing image ⠹",
      "🔍 Analyzing image ⠸",
      "🔍 Analyzing image ⠼",
      "🔍 Analyzing image ⠴",
      "🔍 Analyzing image ⠦",
      "🔍 Analyzing image ⠧",
      "🔍 Analyzing image ⠇",
      "🔍 Analyzing image ⠏"
  };

  // Update label with current frame
  const std::string& frame = spinnerFrames[m_ocrLoadingFrame % 10];
  m_ocrStatusLabel->SetLabel(wxString::FromUTF8(frame.c_str()));

  // Move to next frame
  m_ocrLoadingFrame++;
}

#endif
