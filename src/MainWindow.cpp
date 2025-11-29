#include "MainWindow.h"
#include "utils.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Menu_Item.H>
#include <algorithm>
#include <locale>
#include <regex>
#include <string>
#include <codecvt>

// Helper to convert wstring to UTF-8 string
static std::string wstring_to_utf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// Helper to convert UTF-8 string to wstring
static std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

MainWindow::MainWindow()
    : Fl_Double_Window(1200, 800, "Bezugszeichenprüfvorrichtung"),
      m_styleTable(nullptr),
      m_styleTableSize(3),
      m_timerScheduled(false),
      m_debounceDelay(0.5) {

    setupUi();
    setupBindings();
    loadIcons();
    end();
}

MainWindow::~MainWindow() {
    if (m_styleTable) {
        delete[] m_styleTable;
    }
    delete m_textBuffer;
    delete m_styleBuffer;
    delete m_bzListBuffer;
}

void MainWindow::debounceFunc() {
    if (m_timerScheduled) {
        Fl::remove_timeout(scan_timer_callback, this);
    }
    Fl::add_timeout(m_debounceDelay, scan_timer_callback, this);
    m_timerScheduled = true;
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
// Check if a word is an indefinite article
bool isIndefiniteArticle(const std::wstring &word) {
    std::wstring lower = word;
    for (auto &c : lower) {
        c = std::tolower(c);
    }
    return lower == L"ein" || lower == L"eine" || lower == L"eines" ||
           lower == L"einen" || lower == L"einer" || lower == L"einem";
}

// Check if a word is a definite article
bool isDefiniteArticle(const std::wstring &word) {
    std::wstring lower = word;
    for (auto &c : lower) {
        c = std::tolower(c);
    }
    return lower == L"der" || lower == L"die" || lower == L"das" ||
           lower == L"den" || lower == L"dem" || lower == L"des";
}

// Find the word immediately preceding a given position in the text
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

    // Second pass: scan for single-word patterns
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
    std::string label;
    label = "0/" + std::to_string(m_noNumberPositions.size() / 2) + "\t";
    m_noNumberLabel->copy_label(label.c_str());

    label = "0/" + std::to_string(m_wrongNumberPositions.size() / 2) + "\t";
    m_wrongNumberLabel->copy_label(label.c_str());

    label = "0/" + std::to_string(m_splitNumberPositions.size() / 2) + "\t";
    m_splitNumberLabel->copy_label(label.c_str());

    label = "0/" + std::to_string(m_wrongArticlePositions.size() / 2) + "\t";
    m_wrongArticleLabel->copy_label(label.c_str());

    fillBzList();
}

void MainWindow::fillListTree() {
    m_treeList->clear();

    for (const auto &[bz, stems] : m_bzToStems) {
        std::string bzStr = wstring_to_utf8(bz);
        std::string displayStr = wstring_to_utf8(stemsToDisplayString(stems, m_bzToOriginalWords[bz]));

        // Add item: "BZ / display string"
        std::string itemLabel = bzStr + " / " + displayStr;

        if (isUniquelyAssigned(bz)) {
            m_treeList->add(itemLabel.c_str());
        } else {
            m_treeList->add(itemLabel.c_str());
            // Mark with different icon/color if needed
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
            // Set style to warning (yellow background)
            for (size_t j = start; j < start + len && j < m_styleBuffer->length(); ++j) {
                m_styleBuffer->replace(j, j + 1, "B");  // Warning style
            }
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
                    // Set style to warning
                    for (size_t j = start; j < start + len && j < m_styleBuffer->length(); ++j) {
                        m_styleBuffer->replace(j, j + 1, "B");
                    }
                }
            }
            return false;
        }
    }

    return true;
}

void MainWindow::loadIcons() {
    // FLTK doesn't require icon loading for tree items in the same way
    // We can skip this or implement custom drawing if needed
}

void MainWindow::findUnnumberedWords() {
    // Collect start positions of all valid references
    std::unordered_set<size_t> validStarts;
    for (const auto &[stem, positions] : m_stemToPositions) {
        for (const auto [start, len] : positions) {
            validStarts.insert(start);
        }
    }

    // Check for two-word patterns without numbers
    {
        std::wregex twoWordNoNumber{
            L"(\\b[[:alpha:]äöüÄÖÜß]+\\b)(?![[:space:]]+[[:digit:]])",
            std::regex_constants::ECMAScript | std::regex_constants::optimize |
                std::regex_constants::icase};

        std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(),
                                   twoWordNoNumber);
        std::wsregex_iterator end;

        if (iter == end)
            return;

        std::wsmatch prev = *iter;
        ++iter;
        while (iter != end) {
            size_t pos1 = prev.position();
            size_t pos2 = iter->position();

            if (validStarts.count(pos1)) {
                prev = *iter;
                ++iter;
                continue;
            }

            std::wstring word1 = prev.str();
            std::wstring word2 = iter->str();
            stemWord(word2);

            if (isMultiWordBase(word2)) {
                StemVector stemVec = createMultiWordStemVector(word1, word2);

                if (m_stemToBz.count(stemVec)) {
                    size_t len2 = iter->length();
                    m_noNumberPositions.push_back(pos1);
                    m_noNumberPositions.push_back(pos2 + len2);
                    // Set style to warning
                    for (size_t j = pos1; j < pos2 + len2 && j < m_styleBuffer->length(); ++j) {
                        m_styleBuffer->replace(j, j + 1, "B");
                    }
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

            if (validStarts.count(pos)) {
                ++iter;
                continue;
            }

            std::wstring word = iter->str();
            StemVector stemVec = createStemVector(word);

            if (m_stemToBz.count(stemVec)) {
                size_t len = iter->length();
                m_noNumberPositions.push_back(pos);
                m_noNumberPositions.push_back(pos + len);
                // Set style to warning
                for (size_t j = pos; j < pos + len && j < m_styleBuffer->length(); ++j) {
                    m_styleBuffer->replace(j, j + 1, "B");
                }
            }

            ++iter;
        }
    }
}

void MainWindow::checkArticleUsage() {
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

    std::unordered_set<StemVector, StemVectorHash> seenStems;

    for (const auto &occ : allOccurrences) {
        auto [precedingWord, precedingPos] =
            findPrecedingWord(m_fullText, occ.position);

        if (precedingWord.empty()) {
            seenStems.insert(occ.stem);
            continue;
        }

        bool isFirstOccurrence = (seenStems.count(occ.stem) == 0);
        size_t articleEnd = precedingPos + precedingWord.length();

        if (isFirstOccurrence) {
            if (isDefiniteArticle(precedingWord)) {
                m_wrongArticlePositions.push_back(precedingPos);
                m_wrongArticlePositions.push_back(articleEnd);
                // Set style to article warning (cyan)
                for (size_t j = precedingPos; j < articleEnd && j < m_styleBuffer->length(); ++j) {
                    m_styleBuffer->replace(j, j + 1, "C");
                }
            }
            seenStems.insert(occ.stem);
        } else {
            if (isIndefiniteArticle(precedingWord)) {
                m_wrongArticlePositions.push_back(precedingPos);
                m_wrongArticlePositions.push_back(articleEnd);
                // Set style to article warning
                for (size_t j = precedingPos; j < articleEnd && j < m_styleBuffer->length(); ++j) {
                    m_styleBuffer->replace(j, j + 1, "C");
                }
            }
        }
    }
}

void MainWindow::setupAndClear() {
    const char* text = m_textBuffer->text();
    m_fullText = utf8_to_wstring(text);
    free((void*)text);

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

    // Reset text highlighting - set all to neutral style
    int len = m_textBuffer->length();
    m_styleBuffer->remove(0, m_styleBuffer->length());
    std::string neutral(len, 'A');  // 'A' = neutral style
    m_styleBuffer->append(neutral.c_str());
}

void MainWindow::selectNextNoNumber() {
    m_noNumberSelected += 2;
    if (m_noNumberSelected >= static_cast<int>(m_noNumberPositions.size()) ||
        m_noNumberSelected < 0) {
        m_noNumberSelected = 0;
    }

    if (!m_noNumberPositions.empty()) {
        m_textBuffer->select(m_noNumberPositions[m_noNumberSelected],
                            m_noNumberPositions[m_noNumberSelected + 1]);
        m_textBox->insert_position(m_noNumberPositions[m_noNumberSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_noNumberSelected / 2 + 1) + "/" +
                          std::to_string(m_noNumberPositions.size() / 2) + "\t";
        m_noNumberLabel->copy_label(label.c_str());
    }
}

void MainWindow::selectPreviousNoNumber() {
    m_noNumberSelected -= 2;
    if (m_noNumberSelected >= static_cast<int>(m_noNumberPositions.size()) ||
        m_noNumberSelected < 0) {
        m_noNumberSelected = m_noNumberPositions.size() - 2;
    }

    if (!m_noNumberPositions.empty()) {
        m_textBuffer->select(m_noNumberPositions[m_noNumberSelected],
                            m_noNumberPositions[m_noNumberSelected + 1]);
        m_textBox->insert_position(m_noNumberPositions[m_noNumberSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_noNumberSelected / 2 + 1) + "/" +
                          std::to_string(m_noNumberPositions.size() / 2) + "\t";
        m_noNumberLabel->copy_label(label.c_str());
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
        m_textBuffer->select(m_wrongNumberPositions[m_wrongNumberSelected],
                            m_wrongNumberPositions[m_wrongNumberSelected + 1]);
        m_textBox->insert_position(m_wrongNumberPositions[m_wrongNumberSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_wrongNumberSelected / 2 + 1) + "/" +
                          std::to_string(m_wrongNumberPositions.size() / 2) + "\t";
        m_wrongNumberLabel->copy_label(label.c_str());
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
        m_textBuffer->select(m_wrongNumberPositions[m_wrongNumberSelected],
                            m_wrongNumberPositions[m_wrongNumberSelected + 1]);
        m_textBox->insert_position(m_wrongNumberPositions[m_wrongNumberSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_wrongNumberSelected / 2 + 1) + "/" +
                          std::to_string(m_wrongNumberPositions.size() / 2) + "\t";
        m_wrongNumberLabel->copy_label(label.c_str());
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
        m_textBuffer->select(m_splitNumberPositions[m_splitNumberSelected],
                            m_splitNumberPositions[m_splitNumberSelected + 1]);
        m_textBox->insert_position(m_splitNumberPositions[m_splitNumberSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_splitNumberSelected / 2 + 1) + "/" +
                          std::to_string(m_splitNumberPositions.size() / 2) + "\t";
        m_splitNumberLabel->copy_label(label.c_str());
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
        m_textBuffer->select(m_splitNumberPositions[m_splitNumberSelected],
                            m_splitNumberPositions[m_splitNumberSelected + 1]);
        m_textBox->insert_position(m_splitNumberPositions[m_splitNumberSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_splitNumberSelected / 2 + 1) + "/" +
                          std::to_string(m_splitNumberPositions.size() / 2) + "\t";
        m_splitNumberLabel->copy_label(label.c_str());
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
        m_textBuffer->select(
            m_wrongArticlePositions[m_wrongArticleSelected],
            m_wrongArticlePositions[m_wrongArticleSelected + 1]);
        m_textBox->insert_position(m_wrongArticlePositions[m_wrongArticleSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_wrongArticleSelected / 2 + 1) + "/" +
                          std::to_string(m_wrongArticlePositions.size() / 2) + "\t";
        m_wrongArticleLabel->copy_label(label.c_str());
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
        m_textBuffer->select(
            m_wrongArticlePositions[m_wrongArticleSelected],
            m_wrongArticlePositions[m_wrongArticleSelected + 1]);
        m_textBox->insert_position(m_wrongArticlePositions[m_wrongArticleSelected]);
        m_textBox->show_insert_position();

        std::string label = std::to_string(m_wrongArticleSelected / 2 + 1) + "/" +
                          std::to_string(m_wrongArticlePositions.size() / 2) + "\t";
        m_wrongArticleLabel->copy_label(label.c_str());
    }
}

void MainWindow::setupUi() {
    begin();

    // Create main horizontal group (like wxHORIZONTAL sizer)
    Fl_Group *mainGroup = new Fl_Group(0, 0, w(), h());
    mainGroup->begin();

    // Left side: text editor
    m_textBuffer = new Fl_Text_Buffer();
    m_styleBuffer = new Fl_Text_Buffer();

    // Setup style table: A=neutral, B=warning(yellow), C=article warning(cyan)
    m_styleTable = new Fl_Text_Display::Style_Table_Entry[3];
    m_styleTable[0].color = FL_WHITE;           // Neutral background
    m_styleTable[0].font  = FL_HELVETICA;
    m_styleTable[0].size  = 14;
    m_styleTable[1].color = FL_YELLOW;          // Warning background
    m_styleTable[1].font  = FL_HELVETICA;
    m_styleTable[1].size  = 14;
    m_styleTable[2].color = FL_CYAN;            // Article warning background
    m_styleTable[2].font  = FL_HELVETICA;
    m_styleTable[2].size  = 14;

    m_textBox = new Fl_Text_Editor(10, 10, 750, 780);
    m_textBox->buffer(m_textBuffer);
    m_textBox->highlight_data(m_styleBuffer, m_styleTable, 3, 'A', nullptr, nullptr);

    // Right side: output area with notebook/tabs
    Fl_Group *outputGroup = new Fl_Group(770, 10, 420, 780);
    outputGroup->begin();

    // Create tabs (notebook)
    m_notebookList = new Fl_Tabs(770, 10, 420, 520);
    m_notebookList->begin();

    // Tree list tab
    Fl_Group *treeTab = new Fl_Group(770, 35, 420, 495, "overview");
    treeTab->begin();
    m_treeList = new Fl_Tree(770, 35, 420, 495);
    m_treeList->showroot(0);
    treeTab->end();

    // BZ list tab
    Fl_Group *bzTab = new Fl_Group(770, 35, 420, 495, "reference sign list");
    bzTab->begin();
    m_bzListBuffer = new Fl_Text_Buffer();
    m_bzList = new Fl_Text_Display(770, 35, 420, 495);
    m_bzList->buffer(m_bzListBuffer);
    bzTab->end();

    m_notebookList->end();

    // Navigation buttons area
    int btnY = 540;
    int btnX = 770;
    int btnSpacing = 25;

    // Row 1: unnumbered
    m_buttonBackwardNoNumber = new Fl_Button(btnX, btnY, 50, 25, "<");
    m_buttonForwardNoNumber = new Fl_Button(btnX + 55, btnY, 50, 25, ">");
    m_noNumberLabel = new Fl_Box(btnX + 110, btnY, 60, 25, "0/0\t");
    m_noNumberLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Box *noNumberDesc = new Fl_Box(btnX + 175, btnY, 150, 25, "unnumbered");
    noNumberDesc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    btnY += btnSpacing;

    // Row 2: wrong number
    m_buttonBackwardWrongNumber = new Fl_Button(btnX, btnY, 50, 25, "<");
    m_buttonForwardWrongNumber = new Fl_Button(btnX + 55, btnY, 50, 25, ">");
    m_wrongNumberLabel = new Fl_Box(btnX + 110, btnY, 60, 25, "0/0\t");
    m_wrongNumberLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Box *wrongNumberDesc = new Fl_Box(btnX + 175, btnY, 150, 25, "inconsistent terms");
    wrongNumberDesc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    btnY += btnSpacing;

    // Row 3: split number
    m_buttonBackwardSplitNumber = new Fl_Button(btnX, btnY, 50, 25, "<");
    m_buttonForwardSplitNumber = new Fl_Button(btnX + 55, btnY, 50, 25, ">");
    m_splitNumberLabel = new Fl_Box(btnX + 110, btnY, 60, 25, "0/0\t");
    m_splitNumberLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Box *splitNumberDesc = new Fl_Box(btnX + 175, btnY, 200, 25, "inconsistent reference signs");
    splitNumberDesc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    btnY += btnSpacing;

    // Row 4: wrong article
    m_buttonBackwardWrongArticle = new Fl_Button(btnX, btnY, 50, 25, "<");
    m_buttonForwardWrongArticle = new Fl_Button(btnX + 55, btnY, 50, 25, ">");
    m_wrongArticleLabel = new Fl_Box(btnX + 110, btnY, 60, 25, "0/0\t");
    m_wrongArticleLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Box *wrongArticleDesc = new Fl_Box(btnX + 175, btnY, 150, 25, "inconsistent article");
    wrongArticleDesc->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    outputGroup->end();
    mainGroup->end();

    resizable(mainGroup);
}

void MainWindow::setupBindings() {
    // Text change callback
    m_textBuffer->add_modify_callback(text_changed_callback, this);

    // Button callbacks - store button ID in user_data
    m_buttonBackwardNoNumber->callback(button_callback, (void*)0);
    m_buttonForwardNoNumber->callback(button_callback, (void*)1);
    m_buttonBackwardWrongNumber->callback(button_callback, (void*)2);
    m_buttonForwardWrongNumber->callback(button_callback, (void*)3);
    m_buttonBackwardSplitNumber->callback(button_callback, (void*)4);
    m_buttonForwardSplitNumber->callback(button_callback, (void*)5);
    m_buttonBackwardWrongArticle->callback(button_callback, (void*)6);
    m_buttonForwardWrongArticle->callback(button_callback, (void*)7);

    // Tree callback
    m_treeList->callback(tree_callback, this);
}

void MainWindow::fillBzList() {
    m_bzListBuffer->text("");

    // Iterate through tree items
    for (const auto &[bz, positions] : m_bzToPositions) {
        if (positions.size() >= 1) {
            size_t start = positions[0].first;
            size_t len = positions[0].second;

            // Extract the term without the BZ number
            size_t bzLen = bz.size();
            size_t termLen = len > bzLen + 1 ? len - bzLen - 1 : 0;
            std::wstring termText = m_fullText.substr(start, termLen);

            std::string output = wstring_to_utf8(bz) + "\t" + wstring_to_utf8(termText) + "\n";
            m_bzListBuffer->append(output.c_str());
        }
    }
}

void MainWindow::onTreeListContextMenu() {
    // Get selected item from tree
    Fl_Tree_Item *item = m_treeList->first_selected_item();
    if (!item) return;

    // Parse the item label to extract BZ
    const char *label = item->label();
    if (!label) return;

    std::string labelStr(label);
    size_t slashPos = labelStr.find('/');
    if (slashPos == std::string::npos) return;

    std::string bzStr = labelStr.substr(0, slashPos);
    // Trim whitespace
    bzStr.erase(0, bzStr.find_first_not_of(" \t"));
    bzStr.erase(bzStr.find_last_not_of(" \t") + 1);

    std::wstring bz = utf8_to_wstring(bzStr);

    // Get the stems for this BZ
    if (m_bzToStems.count(bz) && !m_bzToStems[bz].empty()) {
        const StemVector &firstStem = *m_bzToStems[bz].begin();
        if (firstStem.empty()) return;

        std::wstring baseStem = firstStem.back();

        // Show context menu
        bool isMultiWord = m_multiWordBaseStems.count(baseStem) > 0;
        const char *menuLabel = isMultiWord ? "Disable multi-word mode" : "Enable multi-word mode";

        int result = fl_choice("Toggle multi-word mode?", "Cancel", menuLabel, nullptr);
        if (result == 1) {
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

    // Trigger rescan immediately
    if (m_timerScheduled) {
        Fl::remove_timeout(scan_timer_callback, this);
    }
    Fl::add_timeout(0.001, scan_timer_callback, this);
    m_timerScheduled = true;
}

void MainWindow::onTreeListItemActivated() {
    Fl_Tree_Item *item = m_treeList->first_selected_item();
    if (!item) return;

    const char *label = item->label();
    if (!label) return;

    std::string labelStr(label);
    size_t slashPos = labelStr.find('/');
    if (slashPos == std::string::npos) return;

    std::string bzStr = labelStr.substr(0, slashPos);
    bzStr.erase(0, bzStr.find_first_not_of(" \t"));
    bzStr.erase(bzStr.find_last_not_of(" \t") + 1);

    std::wstring bz = utf8_to_wstring(bzStr);

    // Check if this BZ has any positions
    if (m_bzToPositions.count(bz) && !m_bzToPositions[bz].empty()) {
        if (!m_bzCurrentOccurrence.count(bz)) {
            m_bzCurrentOccurrence[bz] = 0;
        }

        int &currentIdx = m_bzCurrentOccurrence[bz];
        const auto &positions = m_bzToPositions[bz];

        size_t start = positions[currentIdx].first;
        size_t len = positions[currentIdx].second;

        // Select and show the text
        m_textBuffer->select(start, start + len);
        m_textBox->insert_position(start);
        m_textBox->show_insert_position();
        m_textBox->take_focus();

        // Move to next occurrence
        currentIdx = (currentIdx + 1) % positions.size();
    }
}

// Static callback implementations
void MainWindow::debounce_callback(void* data) {
    // Not used - we use scan_timer_callback directly
}

void MainWindow::scan_timer_callback(void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    win->m_timerScheduled = false;
    win->scanText();
}

void MainWindow::button_callback(Fl_Widget* w, void* data) {
    MainWindow* win = nullptr;

    // Find the MainWindow by going up the widget tree
    Fl_Widget* parent = w->parent();
    while (parent) {
        win = dynamic_cast<MainWindow*>(parent);
        if (win) break;
        parent = parent->parent();
    }

    if (!win) return;

    // Button ID stored in user_data
    intptr_t buttonId = reinterpret_cast<intptr_t>(data);

    switch (buttonId) {
        case 0: win->selectPreviousNoNumber(); break;
        case 1: win->selectNextNoNumber(); break;
        case 2: win->selectPreviousWrongNumber(); break;
        case 3: win->selectNextWrongNumber(); break;
        case 4: win->selectPreviousSplitNumber(); break;
        case 5: win->selectNextSplitNumber(); break;
        case 6: win->selectPreviousWrongArticle(); break;
        case 7: win->selectNextWrongArticle(); break;
    }
}

void MainWindow::text_changed_callback(int pos, int nInserted, int nDeleted,
                                       int nRestyled, const char* deletedText, void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    win->debounceFunc();
}

void MainWindow::tree_callback(Fl_Widget* w, void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    Fl_Tree* tree = static_cast<Fl_Tree*>(w);

    // Handle double-click or Enter key
    if (Fl::event_clicks() > 0 || Fl::event_key() == FL_Enter) {
        win->onTreeListItemActivated();
    }

    // Handle right-click for context menu
    if (Fl::event_button() == FL_RIGHT_MOUSE) {
        win->onTreeListContextMenu();
    }
}
