#include "MainWindow.h"
#include "utils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMenu>
#include <QTextCursor>
#include <QTextDocument>
#include <QMessageBox>
#include <QIcon>
#include <QPixmap>
#include <QHeaderView>
#include <algorithm>
#include <locale>
#include <regex>
#include <string>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
  setWindowTitle(QString::fromUtf8("Bezugszeichenprüfvorrichtung"));
  resize(1200, 800);

#ifdef _WIN32
  // Set window icon on Windows
  setWindowIcon(QIcon(":/icons/app_icon.ico"));
#endif

  setupUi();
  loadIcons();
  setupBindings();
}

void MainWindow::debounceFunc() {
  m_debounceTimer.start(500);
}

void MainWindow::stemWord(std::wstring &word) {
  if (word.empty())
    return;
  word[0] = std::tolower(word[0]);
  m_germanStemmer(word);
}

StemVector MainWindow::createStemVector(const std::wstring &word) {
  std::wstring stem = word;
  stemWord(stem);
  return {stem};
}

StemVector
MainWindow::createMultiWordStemVector(const std::wstring &firstWord,
                                      const std::wstring &secondWord) {
  std::wstring stem1 = firstWord;
  std::wstring stem2 = secondWord;
  stemWord(stem1);
  stemWord(stem2);
  return {stem1, stem2};
}

bool MainWindow::isMultiWordBase(const std::wstring &word) {
  std::wstring stem = word;
  stemWord(stem);
  return m_multiWordBaseStems.count(stem) > 0;
}

namespace {
// Check if a word is an indefinite article (ein, eine, eines, einen, einer,
// einem)
bool isIndefiniteArticle(const std::wstring &word) {
  std::wstring lower = word;
  for (auto &c : lower) {
    c = std::tolower(c);
  }
  return lower == L"ein" || lower == L"eine" || lower == L"eines" ||
         lower == L"einen" || lower == L"einer" || lower == L"einem";
}

// Check if a word is a definite article (der, die, das, den, dem, des)
bool isDefiniteArticle(const std::wstring &word) {
  std::wstring lower = word;
  for (auto &c : lower) {
    c = std::tolower(c);
  }
  return lower == L"der" || lower == L"die" || lower == L"das" ||
         lower == L"den" || lower == L"dem" || lower == L"des";
}

// Find the word immediately preceding a given position in the text
// Returns the word and its start position, or empty string if none found
std::pair<std::wstring, size_t> findPrecedingWord(const std::wstring &text,
                                                  size_t pos) {
  if (pos == 0) {
    return {L"", 0};
  }

  // Skip whitespace backwards
  size_t end = pos;
  while (end > 0 && std::iswspace(text[end - 1])) {
    --end;
  }

  if (end == 0) {
    return {L"", 0};
  }

  // Find the start of the word
  size_t start = end;
  while (start > 0 && std::iswalpha(text[start - 1])) {
    --start;
  }

  if (start == end) {
    return {L"", 0};
  }

  return {text.substr(start, end - start), start};
}
} // namespace

void MainWindow::scanText() {
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
    std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(),
                               m_twoWordRegex);
    std::wsregex_iterator end;

    while (iter != end) {
      size_t pos = iter->position();
      size_t len = iter->length();
      size_t endPos = pos + len;

      std::wstring word1 = (*iter)[1].str();
      std::wstring word2 = (*iter)[2].str();
      std::wstring bz = (*iter)[3].str();

      // Check if word2's stem is marked for multi-word matching
      if (isMultiWordBase(word2)) {
        if (!overlapsExisting(pos, endPos)) {
          matchedRanges.emplace_back(pos, endPos);

          // Create stem vector with both words stemmed separately
          StemVector stemVec = createMultiWordStemVector(word1, word2);
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
      ++iter;
    }
  }

  // Second pass: scan for single-word patterns (excluding already matched
  // positions)
  {
    std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(),
                               m_singleWordRegex);
    std::wsregex_iterator end;

    while (iter != end) {
      size_t pos = iter->position();
      size_t len = iter->length();
      size_t endPos = pos + len;

      if (!overlapsExisting(pos, endPos)) {
        matchedRanges.emplace_back(pos, endPos);

        std::wstring word = (*iter)[1].str();
        std::wstring bz = (*iter)[2].str();

        // Create single-element stem vector
        StemVector stemVec = createStemVector(word);

        // Store mappings
        m_bzToStems[bz].insert(stemVec);
        m_stemToBz[stemVec].insert(bz);
        m_bzToOriginalWords[bz].insert(word);

        // Track positions
        m_bzToPositions[bz].push_back({pos, len});
        m_stemToPositions[stemVec].push_back({pos, len});
      }
      ++iter;
    }
  }

  // Collect all unique stems
  collectAllStems(m_stemToBz, m_allStems);

  // Update display
  fillListTree();
  findUnnumberedWords();
  checkArticleUsage();

  // Update navigation labels
  m_noNumberLabel->setText(
      QString::fromStdWString(L"0/" + std::to_wstring(m_noNumberPositions.size() / 2) + L"\t"));
  m_wrongNumberLabel->setText(
      QString::fromStdWString(L"0/" + std::to_wstring(m_wrongNumberPositions.size() / 2) + L"\t"));
  m_splitNumberLabel->setText(
      QString::fromStdWString(L"0/" + std::to_wstring(m_splitNumberPositions.size() / 2) + L"\t"));
  m_wrongArticleLabel->setText(
      QString::fromStdWString(L"0/" + std::to_wstring(m_wrongArticlePositions.size() / 2) + L"\t"));

  fillBzList();
}

void MainWindow::fillListTree() {
  for (const auto &[bz, stems] : m_bzToStems) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_treeList.get());

    item->setText(0, QString::fromStdWString(bz));
    item->setText(1, QString::fromStdWString(stemsToDisplayString(stems, m_bzToOriginalWords[bz])));

    if (isUniquelyAssigned(bz)) {
      item->setIcon(0, QIcon(":/icons/check_16.png"));
    } else {
      item->setIcon(0, QIcon(":/icons/warning_16.png"));
    }
  }
}

bool MainWindow::isUniquelyAssigned(const std::wstring &bz) {
  const auto &stems = m_bzToStems.at(bz);

  // Check if multiple different stems are assigned to this BZ
  if (stems.size() > 1) {
    // Highlight all occurrences of this BZ
    const auto &positions = m_bzToPositions[bz];
    for (const auto i : positions) {
      size_t start = i.first;
      size_t len = i.second;
      m_wrongNumberPositions.push_back(start);
      m_wrongNumberPositions.push_back(start + len);

      // Apply highlighting using QTextEdit
      QTextCursor cursor(m_textBox->document());
      cursor.setPosition(start);
      cursor.setPosition(start + len, QTextCursor::KeepAnchor);
      cursor.setCharFormat(m_warningStyle);
    }
    return false;
  }

  // Check if the stem is also used with other BZs
  for (const auto &stem : stems) {
    if (m_stemToBz.at(stem).size() > 1) {
      // This stem maps to multiple BZs - highlight occurrences
      const auto &positions = m_stemToPositions[stem];
      for (const auto i : positions) {
        size_t start = i.first;
        size_t len = i.second;

        // Avoid duplicates in the split number list
        if (std::find(m_splitNumberPositions.begin(),
                      m_splitNumberPositions.end(),
                      start) == m_splitNumberPositions.end()) {
          m_splitNumberPositions.push_back(start);
          m_splitNumberPositions.push_back(start + len);

          QTextCursor cursor(m_textBox->document());
          cursor.setPosition(start);
          cursor.setPosition(start + len, QTextCursor::KeepAnchor);
          cursor.setCharFormat(m_warningStyle);
        }
      }
      return false;
    }
  }

  return true;
}

void MainWindow::loadIcons() {
  // Icons would be loaded from resources
  // For now, we'll skip the actual icon loading
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
    // Pattern: [word1] [word2] NOT followed by a number
    std::wregex twoWordNoNumber{
        L"(\\b[[:alpha:]äöüÄÖÜß]+\\b)(?![[:space:]]+[[:digit:]])",
        std::regex_constants::ECMAScript | std::regex_constants::optimize |
            std::regex_constants::icase};

    std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(),
                               twoWordNoNumber);
    std::wsregex_iterator end;

    if (iter == end)
      return; // No matches at all

    // Store the previous match manually
    std::wsmatch prev = *iter;
    ++iter;
    while (iter != end) {
      size_t pos1 = prev.position();
      size_t pos2 = iter->position();

      // Skip if already part of a valid reference
      if (validStarts.count(pos1)) {
        prev = *iter;
        ++iter;
        continue;
      }

      std::wstring word1 = prev.str();
      std::wstring word2 = iter->str();
      stemWord(word2);

      // Only flag if this is a known multi-word combination
      if (isMultiWordBase(word2)) {
        StemVector stemVec = createMultiWordStemVector(word1, word2);

        if (m_stemToBz.count(stemVec)) {
          size_t len2 = iter->length();
          m_noNumberPositions.push_back(pos1);
          m_noNumberPositions.push_back(pos2 + len2);

          QTextCursor cursor(m_textBox->document());
          cursor.setPosition(pos1);
          cursor.setPosition(pos2 + len2, QTextCursor::KeepAnchor);
          cursor.setCharFormat(m_warningStyle);
        }
      }
      prev = *iter;
      ++iter;
    }
  }

  // Check for single words without numbers
  {
    std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(),
                               m_wordRegex);
    std::wsregex_iterator end;

    while (iter != end) {
      size_t pos = iter->position();

      // Skip if already part of a valid reference
      if (validStarts.count(pos)) {
        ++iter;
        continue;
      }

      std::wstring word = iter->str();
      StemVector stemVec = createStemVector(word);

      // Check if this stem is known from valid references
      if (m_stemToBz.count(stemVec)) {
        size_t len = iter->length();
        m_noNumberPositions.push_back(pos);
        m_noNumberPositions.push_back(pos + len);

        QTextCursor cursor(m_textBox->document());
        cursor.setPosition(pos);
        cursor.setPosition(pos + len, QTextCursor::KeepAnchor);
        cursor.setCharFormat(m_warningStyle);
      }

      ++iter;
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
        findPrecedingWord(m_fullText, occ.position);

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
      if (isDefiniteArticle(precedingWord)) {
        m_wrongArticlePositions.push_back(precedingPos);
        m_wrongArticlePositions.push_back(articleEnd);

        QTextCursor cursor(m_textBox->document());
        cursor.setPosition(precedingPos);
        cursor.setPosition(articleEnd, QTextCursor::KeepAnchor);
        cursor.setCharFormat(m_articleWarningStyle);
      }
      seenStems.insert(occ.stem);
    } else {
      // Subsequent occurrence: should have definite article
      if (isIndefiniteArticle(precedingWord)) {
        m_wrongArticlePositions.push_back(precedingPos);
        m_wrongArticlePositions.push_back(articleEnd);

        QTextCursor cursor(m_textBox->document());
        cursor.setPosition(precedingPos);
        cursor.setPosition(articleEnd, QTextCursor::KeepAnchor);
        cursor.setCharFormat(m_articleWarningStyle);
      }
    }
  }
}

void MainWindow::setupAndClear() {
  m_fullText = m_textBox->toPlainText().toStdWString();

  // Clear all data structures
  m_bzToStems.clear();
  m_stemToBz.clear();
  m_bzToOriginalWords.clear();
  m_bzToPositions.clear();
  m_stemToPositions.clear();
  m_allStems.clear();

  m_treeList->clear();

  m_noNumberPositions.clear();
  m_wrongNumberPositions.clear();
  m_splitNumberPositions.clear();
  m_wrongArticlePositions.clear();
  m_bzCurrentOccurrence.clear();

  // Reset text highlighting
  QTextCursor cursor(m_textBox->document());
  cursor.select(QTextCursor::Document);
  cursor.setCharFormat(m_neutralStyle);
}

void MainWindow::selectNextNoNumber() {
  m_noNumberSelected += 2;
  if (m_noNumberSelected >= static_cast<int>(m_noNumberPositions.size()) ||
      m_noNumberSelected < 0) {
    m_noNumberSelected = 0;
  }

  if (!m_noNumberPositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_noNumberPositions[m_noNumberSelected]);
    cursor.setPosition(m_noNumberPositions[m_noNumberSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_noNumberLabel->setText(
        QString::fromStdWString(std::to_wstring(m_noNumberSelected / 2 + 1) + L"/" +
        std::to_wstring(m_noNumberPositions.size() / 2) + L"\t"));
  }
}

void MainWindow::selectPreviousNoNumber() {
  m_noNumberSelected -= 2;
  if (m_noNumberSelected >= static_cast<int>(m_noNumberPositions.size()) ||
      m_noNumberSelected < 0) {
    m_noNumberSelected = m_noNumberPositions.size() - 2;
  }

  if (!m_noNumberPositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_noNumberPositions[m_noNumberSelected]);
    cursor.setPosition(m_noNumberPositions[m_noNumberSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_noNumberLabel->setText(
        QString::fromStdWString(std::to_wstring(m_noNumberSelected / 2 + 1) + L"/" +
        std::to_wstring(m_noNumberPositions.size() / 2) + L"\t"));
  }
}

void MainWindow::selectNextWrongNumber() {
  m_wrongNumberSelected += 2;
  if (m_wrongNumberSelected >=
          static_cast<int>(m_wrongNumberPositions.size()) ||
      m_wrongNumberSelected < 0) {
    m_wrongNumberSelected = 0;
  }

  if (!m_wrongNumberPositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_wrongNumberPositions[m_wrongNumberSelected]);
    cursor.setPosition(m_wrongNumberPositions[m_wrongNumberSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_wrongNumberLabel->setText(
        QString::fromStdWString(std::to_wstring(m_wrongNumberSelected / 2 + 1) + L"/" +
        std::to_wstring(m_wrongNumberPositions.size() / 2) + L"\t"));
  }
}

void MainWindow::selectPreviousWrongNumber() {
  m_wrongNumberSelected -= 2;
  if (m_wrongNumberSelected >=
          static_cast<int>(m_wrongNumberPositions.size()) ||
      m_wrongNumberSelected < 0) {
    m_wrongNumberSelected = m_wrongNumberPositions.size() - 2;
  }

  if (!m_wrongNumberPositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_wrongNumberPositions[m_wrongNumberSelected]);
    cursor.setPosition(m_wrongNumberPositions[m_wrongNumberSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_wrongNumberLabel->setText(
        QString::fromStdWString(std::to_wstring(m_wrongNumberSelected / 2 + 1) + L"/" +
        std::to_wstring(m_wrongNumberPositions.size() / 2) + L"\t"));
  }
}

void MainWindow::selectNextSplitNumber() {
  m_splitNumberSelected += 2;
  if (m_splitNumberSelected >=
          static_cast<int>(m_splitNumberPositions.size()) ||
      m_splitNumberSelected < 0) {
    m_splitNumberSelected = 0;
  }

  if (!m_splitNumberPositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_splitNumberPositions[m_splitNumberSelected]);
    cursor.setPosition(m_splitNumberPositions[m_splitNumberSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_splitNumberLabel->setText(
        QString::fromStdWString(std::to_wstring(m_splitNumberSelected / 2 + 1) + L"/" +
        std::to_wstring(m_splitNumberPositions.size() / 2) + L"\t"));
  }
}

void MainWindow::selectPreviousSplitNumber() {
  m_splitNumberSelected -= 2;
  if (m_splitNumberSelected >=
          static_cast<int>(m_splitNumberPositions.size()) ||
      m_splitNumberSelected < 0) {
    m_splitNumberSelected = m_splitNumberPositions.size() - 2;
  }

  if (!m_splitNumberPositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_splitNumberPositions[m_splitNumberSelected]);
    cursor.setPosition(m_splitNumberPositions[m_splitNumberSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_splitNumberLabel->setText(
        QString::fromStdWString(std::to_wstring(m_splitNumberSelected / 2 + 1) + L"/" +
        std::to_wstring(m_splitNumberPositions.size() / 2) + L"\t"));
  }
}

void MainWindow::selectNextWrongArticle() {
  m_wrongArticleSelected += 2;
  if (m_wrongArticleSelected >=
          static_cast<int>(m_wrongArticlePositions.size()) ||
      m_wrongArticleSelected < 0) {
    m_wrongArticleSelected = 0;
  }

  if (!m_wrongArticlePositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_wrongArticlePositions[m_wrongArticleSelected]);
    cursor.setPosition(m_wrongArticlePositions[m_wrongArticleSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_wrongArticleLabel->setText(
        QString::fromStdWString(std::to_wstring(m_wrongArticleSelected / 2 + 1) + L"/" +
        std::to_wstring(m_wrongArticlePositions.size() / 2) + L"\t"));
  }
}

void MainWindow::selectPreviousWrongArticle() {
  m_wrongArticleSelected -= 2;
  if (m_wrongArticleSelected >=
          static_cast<int>(m_wrongArticlePositions.size()) ||
      m_wrongArticleSelected < 0) {
    m_wrongArticleSelected = m_wrongArticlePositions.size() - 2;
  }

  if (!m_wrongArticlePositions.empty()) {
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(m_wrongArticlePositions[m_wrongArticleSelected]);
    cursor.setPosition(m_wrongArticlePositions[m_wrongArticleSelected + 1], QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();

    m_wrongArticleLabel->setText(
        QString::fromStdWString(std::to_wstring(m_wrongArticleSelected / 2 + 1) + L"/" +
        std::to_wstring(m_wrongArticlePositions.size() / 2) + L"\t"));
  }
}

void MainWindow::setupUi() {
  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QHBoxLayout *viewLayout = new QHBoxLayout(centralWidget);

  QVBoxLayout *outputLayout = new QVBoxLayout();
  QVBoxLayout *numberLayout = new QVBoxLayout();
  QHBoxLayout *noNumberLayout = new QHBoxLayout();
  QHBoxLayout *wrongNumberLayout = new QHBoxLayout();
  QHBoxLayout *splitNumberLayout = new QHBoxLayout();
  QHBoxLayout *wrongArticleLayout = new QHBoxLayout();

  m_notebookList = new QTabWidget(centralWidget);

  // Main text editor
  m_textBox = new QTextEdit(centralWidget);
  m_bzList = std::make_shared<QTextEdit>(m_notebookList);
  m_bzList->setReadOnly(true);

  viewLayout->addWidget(m_textBox, 1);
  viewLayout->addLayout(outputLayout, 0);

  // Tree widget for displaying BZ-term mappings
  m_treeList = std::make_shared<QTreeWidget>(m_notebookList);
  m_treeList->setColumnCount(2);
  QStringList headers;
  headers << "reference sign" << "feature";
  m_treeList->setHeaderLabels(headers);
  m_treeList->setContextMenuPolicy(Qt::CustomContextMenu);

  outputLayout->addWidget(m_notebookList, 2);
  m_notebookList->addTab(m_treeList.get(), "overview");
  m_notebookList->addTab(m_bzList.get(), "reference sign list");

  // Navigation buttons for unnumbered references
  m_buttonBackwardNoNumber = std::make_shared<QPushButton>("<", centralWidget);
  m_buttonBackwardNoNumber->setFixedWidth(50);
  m_buttonForwardNoNumber = std::make_shared<QPushButton>(">", centralWidget);
  m_buttonForwardNoNumber->setFixedWidth(50);

  // Navigation buttons for wrong number errors
  m_buttonBackwardWrongNumber = std::make_shared<QPushButton>("<", centralWidget);
  m_buttonBackwardWrongNumber->setFixedWidth(50);
  m_buttonForwardWrongNumber = std::make_shared<QPushButton>(">", centralWidget);
  m_buttonForwardWrongNumber->setFixedWidth(50);

  // Navigation buttons for split number errors
  m_buttonBackwardSplitNumber = std::make_shared<QPushButton>("<", centralWidget);
  m_buttonBackwardSplitNumber->setFixedWidth(50);
  m_buttonForwardSplitNumber = std::make_shared<QPushButton>(">", centralWidget);
  m_buttonForwardSplitNumber->setFixedWidth(50);

  // Navigation buttons for wrong article errors
  m_buttonBackwardWrongArticle = std::make_shared<QPushButton>("<", centralWidget);
  m_buttonBackwardWrongArticle->setFixedWidth(50);
  m_buttonForwardWrongArticle = std::make_shared<QPushButton>(">", centralWidget);
  m_buttonForwardWrongArticle->setFixedWidth(50);

  // Layout for unnumbered references row
  noNumberLayout->addWidget(m_buttonBackwardNoNumber.get());
  noNumberLayout->addWidget(m_buttonForwardNoNumber.get());
  QLabel *noNumberDescription = new QLabel("unnumbered", centralWidget);
  noNumberDescription->setFixedWidth(150);
  m_noNumberLabel = std::make_shared<QLabel>("0/0\t", centralWidget);
  noNumberLayout->addWidget(m_noNumberLabel.get());
  noNumberLayout->addWidget(noNumberDescription);

  // Layout for wrong number row
  wrongNumberLayout->addWidget(m_buttonBackwardWrongNumber.get());
  wrongNumberLayout->addWidget(m_buttonForwardWrongNumber.get());
  QLabel *wrongNumberDescription = new QLabel("inconsistent terms", centralWidget);
  wrongNumberDescription->setFixedWidth(150);
  m_wrongNumberLabel = std::make_shared<QLabel>("0/0\t", centralWidget);
  wrongNumberLayout->addWidget(m_wrongNumberLabel.get());
  wrongNumberLayout->addWidget(wrongNumberDescription);

  // Layout for split number row
  splitNumberLayout->addWidget(m_buttonBackwardSplitNumber.get());
  splitNumberLayout->addWidget(m_buttonForwardSplitNumber.get());
  QLabel *splitNumberDescription = new QLabel("inconsistent reference signs", centralWidget);
  splitNumberDescription->setFixedWidth(150);
  m_splitNumberLabel = std::make_shared<QLabel>("0/0\t", centralWidget);
  splitNumberLayout->addWidget(m_splitNumberLabel.get());
  splitNumberLayout->addWidget(splitNumberDescription);

  // Layout for wrong article row
  wrongArticleLayout->addWidget(m_buttonBackwardWrongArticle.get());
  wrongArticleLayout->addWidget(m_buttonForwardWrongArticle.get());
  QLabel *wrongArticleDescription = new QLabel("inconsistent article", centralWidget);
  wrongArticleDescription->setFixedWidth(150);
  m_wrongArticleLabel = std::make_shared<QLabel>("0/0\t", centralWidget);
  wrongArticleLayout->addWidget(m_wrongArticleLabel.get());
  wrongArticleLayout->addWidget(wrongArticleDescription);

  numberLayout->addLayout(noNumberLayout);
  numberLayout->addLayout(wrongNumberLayout);
  numberLayout->addLayout(splitNumberLayout);
  numberLayout->addLayout(wrongArticleLayout);

  outputLayout->addLayout(numberLayout, 0);

  // Set up text styles
  m_neutralStyle.setBackground(Qt::white);
  m_warningStyle.setBackground(Qt::yellow);
  m_articleWarningStyle.setBackground(Qt::cyan);
}

void MainWindow::setupBindings() {
  connect(m_textBox, &QTextEdit::textChanged, this, &MainWindow::debounceFunc);

  m_debounceTimer.setSingleShot(true);
  connect(&m_debounceTimer, &QTimer::timeout, this, &MainWindow::scanText);

  connect(m_buttonBackwardNoNumber.get(), &QPushButton::clicked,
          this, &MainWindow::selectPreviousNoNumber);
  connect(m_buttonForwardNoNumber.get(), &QPushButton::clicked,
          this, &MainWindow::selectNextNoNumber);

  connect(m_buttonBackwardWrongNumber.get(), &QPushButton::clicked,
          this, &MainWindow::selectPreviousWrongNumber);
  connect(m_buttonForwardWrongNumber.get(), &QPushButton::clicked,
          this, &MainWindow::selectNextWrongNumber);

  connect(m_buttonBackwardSplitNumber.get(), &QPushButton::clicked,
          this, &MainWindow::selectPreviousSplitNumber);
  connect(m_buttonForwardSplitNumber.get(), &QPushButton::clicked,
          this, &MainWindow::selectNextSplitNumber);

  connect(m_buttonBackwardWrongArticle.get(), &QPushButton::clicked,
          this, &MainWindow::selectPreviousWrongArticle);
  connect(m_buttonForwardWrongArticle.get(), &QPushButton::clicked,
          this, &MainWindow::selectNextWrongArticle);

  connect(m_treeList.get(), &QTreeWidget::customContextMenuRequested,
          this, &MainWindow::onTreeItemContextMenu);
  connect(m_treeList.get(), &QTreeWidget::itemActivated,
          this, &MainWindow::onTreeItemActivated);
}

void MainWindow::fillBzList() {
  m_bzList->clear();

  for (int i = 0; i < m_treeList->topLevelItemCount(); ++i) {
    QTreeWidgetItem *item = m_treeList->topLevelItem(i);
    std::wstring bz = item->text(0).toStdWString();

    if (m_bzToPositions.count(bz) && m_bzToPositions[bz].size() >= 1) {
      size_t start = m_bzToPositions[bz][0].first;
      size_t len = m_bzToPositions[bz][0].second;

      // Extract the term without the BZ number
      size_t termLen = len > bz.size() + 1 ? len - bz.size() - 1 : 0;
      std::wstring termText = m_fullText.substr(start, termLen);

      m_bzList->append(QString::fromStdWString(bz + L"\t" + termText));
    }
  }
}

void MainWindow::onTreeItemContextMenu(const QPoint &pos) {
  QTreeWidgetItem *item = m_treeList->itemAt(pos);
  if (!item) {
    return;
  }

  std::wstring bz = item->text(0).toStdWString();

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
    QMenu menu(this);
    bool isMultiWord = m_multiWordBaseStems.count(baseStem) > 0;
    QAction *action = menu.addAction(isMultiWord ? "Disable multi-word mode"
                               : "Enable multi-word mode");

    QAction *selected = menu.exec(m_treeList->mapToGlobal(pos));
    if (selected == action) {
      toggleMultiWordTerm(baseStem);
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
  m_debounceTimer.start(1);
}

void MainWindow::onTreeItemActivated(QTreeWidgetItem *item, int column) {
  if (!item) {
    return;
  }

  std::wstring bz = item->text(0).toStdWString();

  // Check if this BZ has any positions
  if (m_bzToPositions.count(bz) && !m_bzToPositions[bz].empty()) {
    // Get current occurrence index for this BZ (or initialize to 0)
    if (!m_bzCurrentOccurrence.count(bz)) {
      m_bzCurrentOccurrence[bz] = 0;
    }

    int &currentIdx = m_bzCurrentOccurrence[bz];
    const auto &positions = m_bzToPositions[bz];

    // Get the current occurrence
    size_t start = positions[currentIdx].first;
    size_t len = positions[currentIdx].second;

    // Select and show the text
    QTextCursor cursor(m_textBox->document());
    cursor.setPosition(start);
    cursor.setPosition(start + len, QTextCursor::KeepAnchor);
    m_textBox->setTextCursor(cursor);
    m_textBox->ensureCursorVisible();
    m_textBox->setFocus();

    // Move to next occurrence for next double-click
    currentIdx = (currentIdx + 1) % positions.size();
  }
}
