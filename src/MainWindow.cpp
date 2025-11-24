#include "MainWindow.h"
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include "utils.h"
#include "wx/event.h"
#include "wx/gdicmn.h"
#include "wx/timer.h"
#include <algorithm>
#include <locale>
#include <regex>
#include <string>
#include <wx/bitmap.h>

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, "Bezugszeichenprüfvorrichtung",
              wxDefaultPosition, wxSize(1200, 800))
{
    setupUi();
    loadIcons();
    setupBindings();
}

void MainWindow::debounceFunc(wxCommandEvent& event)
{
    m_debounceTimer.Start(500, true);
}

void MainWindow::stemWord(std::wstring& word)
{
    if (word.empty()) return;
    word[0] = std::tolower(word[0]);
    m_germanStemmer(word);
}

StemVector MainWindow::createStemVector(const std::wstring& word)
{
    std::wstring stem = word;
    stemWord(stem);
    return {stem};
}

StemVector MainWindow::createMultiWordStemVector(const std::wstring& firstWord, const std::wstring& secondWord)
{
    std::wstring stem1 = firstWord;
    std::wstring stem2 = secondWord;
    stemWord(stem1);
    stemWord(stem2);
    return {stem1, stem2};
}

bool MainWindow::isMultiWordBase(const std::wstring& word)
{
    std::wstring stem = word;
    stemWord(stem);
    return m_multiWordBaseStems.count(stem) > 0;
}

namespace {
    // Check if a word is an indefinite article (ein, eine, eines, einen, einer, einem)
    bool isIndefiniteArticle(const std::wstring& word)
    {
        std::wstring lower = word;
        for (auto& c : lower) {
            c = std::tolower(c);
        }
        return lower == L"ein" || lower == L"eine" || lower == L"eines" ||
               lower == L"einen" || lower == L"einer" || lower == L"einem";
    }

    // Check if a word is a definite article (der, die, das, den, dem, des)
    bool isDefiniteArticle(const std::wstring& word)
    {
        std::wstring lower = word;
        for (auto& c : lower) {
            c = std::tolower(c);
        }
        return lower == L"der" || lower == L"die" || lower == L"das" ||
               lower == L"den" || lower == L"dem" || lower == L"des";
    }

    // Find the word immediately preceding a given position in the text
    // Returns the word and its start position, or empty string if none found
    std::pair<std::wstring, size_t> findPrecedingWord(const std::wstring& text, size_t pos)
    {
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
}

void MainWindow::scanText(wxTimerEvent& event)
{
    setupAndClear();

    // Track matched positions to avoid duplicate processing
    std::vector<std::pair<size_t, size_t>> matchedRanges;

    auto overlapsExisting = [&matchedRanges](size_t start, size_t end) {
        for (const auto& range : matchedRanges) {
            if (!(end <= range.first || start >= range.second)) {
                return true;
            }
        }
        return false;
    };

    // First pass: scan for two-word patterns
    // Match "[word1] [word2] [number]" and check if word2's stem is in multi-word set
    {
        std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(), m_twoWordRegex);
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
                    m_bzToPositions[bz].push_back(pos);
                    m_bzToPositions[bz].push_back(len);
                    m_stemToPositions[stemVec].push_back(pos);
                    m_stemToPositions[stemVec].push_back(len);
                }
            }
            ++iter;
        }
    }

    // Second pass: scan for single-word patterns (excluding already matched positions)
    {
        std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(), m_singleWordRegex);
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
                m_bzToPositions[bz].push_back(pos);
                m_bzToPositions[bz].push_back(len);
                m_stemToPositions[stemVec].push_back(pos);
                m_stemToPositions[stemVec].push_back(len);
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
    m_noNumberLabel->SetLabel(
        L"0/" + std::to_wstring(m_noNumberPositions.size() / 2) + L"\t");
    m_wrongNumberLabel->SetLabel(
        L"0/" + std::to_wstring(m_wrongNumberPositions.size() / 2) + L"\t");
    m_splitNumberLabel->SetLabel(
        L"0/" + std::to_wstring(m_splitNumberPositions.size() / 2) + L"\t");
    m_wrongArticleLabel->SetLabel(
        L"0/" + std::to_wstring(m_wrongArticlePositions.size() / 2) + L"\t");

    fillBzList();
}

void MainWindow::fillListTree()
{
    for (const auto& [bz, stems] : m_bzToStems) {
        wxTreeListItem item;
        
        if (isUniquelyAssigned(bz)) {
            item = m_treeList->AppendItem(m_treeList->GetRootItem(), bz, 0, 0);
        } else {
            item = m_treeList->AppendItem(m_treeList->GetRootItem(), bz, 1, 1);
        }

        // Display original words for this BZ
        m_treeList->SetItemText(item, 1, 
            stemsToDisplayString(stems, m_bzToOriginalWords[bz]));
    }
}

bool MainWindow::isUniquelyAssigned(const std::wstring& bz)
{
    const auto& stems = m_bzToStems.at(bz);

    // Check if multiple different stems are assigned to this BZ
    if (stems.size() > 1) {
        // Highlight all occurrences of this BZ
        const auto& positions = m_bzToPositions[bz];
        for (size_t i = 0; i < positions.size(); i += 2) {
            size_t start = positions[i];
            size_t len = positions[i + 1];
            m_wrongNumberPositions.push_back(start);
            m_wrongNumberPositions.push_back(start + len);
            m_textBox->SetStyle(start, start + len, m_warningStyle);
        }
        return false;
    }

    // Check if the stem is also used with other BZs
    for (const auto& stem : stems) {
        if (m_stemToBz.at(stem).size() > 1) {
            // This stem maps to multiple BZs - highlight occurrences
            const auto& positions = m_stemToPositions[stem];
            for (size_t i = 0; i < positions.size(); i += 2) {
                size_t start = positions[i];
                size_t len = positions[i + 1];

                // Avoid duplicates in the split number list
                if (std::find(m_splitNumberPositions.begin(), 
                              m_splitNumberPositions.end(), 
                              start) == m_splitNumberPositions.end()) {
                    m_splitNumberPositions.push_back(start);
                    m_splitNumberPositions.push_back(start + len);
                    m_textBox->SetStyle(start, start + len, m_warningStyle);
                }
            }
            return false;
        }
    }

    return true;
}

void MainWindow::loadIcons()
{
    m_imageList = std::make_shared<wxImageList>(16, 16, false, 0);
    wxBitmap check(check_16_xpm);
    wxBitmap warning(warning_16_xpm);

    m_imageList->Add(check);
    m_imageList->Add(warning);
    m_treeList->SetImageList(m_imageList.get());
}

void MainWindow::findUnnumberedWords()
{
    // Collect start positions of all valid references
    std::unordered_set<size_t> validStarts;
    for (const auto& [stem, positions] : m_stemToPositions) {
        for (size_t i = 0; i < positions.size(); i += 2) {
            validStarts.insert(positions[i]);
        }
    }

    // Check for two-word patterns without numbers (if they match known multi-word stems)
    {
        // Pattern: [word1] [word2] NOT followed by a number
        std::wregex twoWordNoNumber{
            L"(\\b[[:alpha:]äöüÄÖÜß]+\\b)[[:space:]]+(\\b[[:alpha:]äöüÄÖÜß]+\\b)(?![[:space:]]+[[:digit:]])",
            std::regex_constants::ECMAScript | std::regex_constants::optimize |
                std::regex_constants::icase};

        std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(), twoWordNoNumber);
        std::wsregex_iterator end;

        while (iter != end) {
            size_t pos = iter->position();

            // Skip if already part of a valid reference
            if (validStarts.count(pos)) {
                ++iter;
                continue;
            }

            std::wstring word1 = (*iter)[1].str();
            std::wstring word2 = (*iter)[2].str();

            // Only flag if this is a known multi-word combination
            if (isMultiWordBase(word2)) {
                StemVector stemVec = createMultiWordStemVector(word1, word2);

                if (m_stemToBz.count(stemVec)) {
                    size_t len = iter->length();
                    m_noNumberPositions.push_back(pos);
                    m_noNumberPositions.push_back(pos + len);
                    m_textBox->SetStyle(pos, pos + len, m_warningStyle);
                }
            }

            ++iter;
        }
    }

    // Check for single words without numbers
    {
        std::wsregex_iterator iter(m_fullText.begin(), m_fullText.end(), m_wordRegex);
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
                m_textBox->SetStyle(pos, pos + len, m_warningStyle);
            }

            ++iter;
        }
    }
}

void MainWindow::checkArticleUsage()
{
    // We need to check each stem's occurrences in order of position
    // First occurrence should have indefinite article, subsequent should have definite
    
    // Build a map of stem -> sorted positions (by position in text)
    struct OccurrenceInfo {
        size_t position;
        size_t length;
        StemVector stem;
    };
    
    std::vector<OccurrenceInfo> allOccurrences;
    
    for (const auto& [stem, positions] : m_stemToPositions) {
        for (size_t i = 0; i < positions.size(); i += 2) {
            allOccurrences.push_back({positions[i], positions[i + 1], stem});
        }
    }
    
    // Sort by position
    std::sort(allOccurrences.begin(), allOccurrences.end(),
              [](const OccurrenceInfo& a, const OccurrenceInfo& b) {
                  return a.position < b.position;
              });
    
    // Track which stems we've seen (first occurrence needs indefinite article)
    std::unordered_set<StemVector, StemVectorHash> seenStems;
    
    for (const auto& occ : allOccurrences) {
        auto [precedingWord, precedingPos] = findPrecedingWord(m_fullText, occ.position);
        
        if (precedingWord.empty()) {
            // No preceding word found - might be at start of text
            // Mark the stem as seen but we can't highlight an article
            seenStems.insert(occ.stem);
            continue;
        }
        
        bool isFirstOccurrence = (seenStems.count(occ.stem) == 0);
        size_t articleEnd = precedingPos + precedingWord.length();
        
        if (isFirstOccurrence) {
            // First occurrence: should have indefinite article
            if (!isIndefiniteArticle(precedingWord)) {
                m_wrongArticlePositions.push_back(precedingPos);
                m_wrongArticlePositions.push_back(articleEnd);
                m_textBox->SetStyle(precedingPos, articleEnd, m_articleWarningStyle);
            }
            seenStems.insert(occ.stem);
        } else {
            // Subsequent occurrence: should have definite article
            if (!isDefiniteArticle(precedingWord)) {
                m_wrongArticlePositions.push_back(precedingPos);
                m_wrongArticlePositions.push_back(articleEnd);
                m_textBox->SetStyle(precedingPos, articleEnd, m_articleWarningStyle);
            }
        }
    }
}

void MainWindow::setupAndClear()
{
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

    // Reset text highlighting
    m_textBox->SetStyle(0, m_textBox->GetValue().length(), m_neutralStyle);
}

void MainWindow::selectNextNoNumber(wxCommandEvent& event)
{
    m_noNumberSelected += 2;
    if (m_noNumberSelected >= static_cast<int>(m_noNumberPositions.size()) || 
        m_noNumberSelected < 0) {
        m_noNumberSelected = 0;
    }

    if (!m_noNumberPositions.empty()) {
        m_textBox->SetSelection(m_noNumberPositions[m_noNumberSelected],
                                m_noNumberPositions[m_noNumberSelected + 1]);
        m_textBox->ShowPosition(m_noNumberPositions[m_noNumberSelected]);
        m_noNumberLabel->SetLabel(
            std::to_wstring(m_noNumberSelected / 2 + 1) + L"/" +
            std::to_wstring(m_noNumberPositions.size() / 2) + L"\t");
    }
}

void MainWindow::selectPreviousNoNumber(wxCommandEvent& event)
{
    m_noNumberSelected -= 2;
    if (m_noNumberSelected >= static_cast<int>(m_noNumberPositions.size()) || 
        m_noNumberSelected < 0) {
        m_noNumberSelected = m_noNumberPositions.size() - 2;
    }

    if (!m_noNumberPositions.empty()) {
        m_textBox->SetSelection(m_noNumberPositions[m_noNumberSelected],
                                m_noNumberPositions[m_noNumberSelected + 1]);
        m_textBox->ShowPosition(m_noNumberPositions[m_noNumberSelected]);
        m_noNumberLabel->SetLabel(
            std::to_wstring(m_noNumberSelected / 2 + 1) + L"/" +
            std::to_wstring(m_noNumberPositions.size() / 2) + L"\t");
    }
}

void MainWindow::selectNextWrongNumber(wxCommandEvent& event)
{
    m_wrongNumberSelected += 2;
    if (m_wrongNumberSelected >= static_cast<int>(m_wrongNumberPositions.size()) || 
        m_wrongNumberSelected < 0) {
        m_wrongNumberSelected = 0;
    }

    if (!m_wrongNumberPositions.empty()) {
        m_textBox->SetSelection(m_wrongNumberPositions[m_wrongNumberSelected],
                                m_wrongNumberPositions[m_wrongNumberSelected + 1]);
        m_textBox->ShowPosition(m_wrongNumberPositions[m_wrongNumberSelected]);
        m_wrongNumberLabel->SetLabel(
            std::to_wstring(m_wrongNumberSelected / 2 + 1) + L"/" +
            std::to_wstring(m_wrongNumberPositions.size() / 2) + L"\t");
    }
}

void MainWindow::selectPreviousWrongNumber(wxCommandEvent& event)
{
    m_wrongNumberSelected -= 2;
    if (m_wrongNumberSelected >= static_cast<int>(m_wrongNumberPositions.size()) || 
        m_wrongNumberSelected < 0) {
        m_wrongNumberSelected = m_wrongNumberPositions.size() - 2;
    }

    if (!m_wrongNumberPositions.empty()) {
        m_textBox->SetSelection(m_wrongNumberPositions[m_wrongNumberSelected],
                                m_wrongNumberPositions[m_wrongNumberSelected + 1]);
        m_textBox->ShowPosition(m_wrongNumberPositions[m_wrongNumberSelected]);
        m_wrongNumberLabel->SetLabel(
            std::to_wstring(m_wrongNumberSelected / 2 + 1) + L"/" +
            std::to_wstring(m_wrongNumberPositions.size() / 2) + L"\t");
    }
}

void MainWindow::selectNextSplitNumber(wxCommandEvent& event)
{
    m_splitNumberSelected += 2;
    if (m_splitNumberSelected >= static_cast<int>(m_splitNumberPositions.size()) || 
        m_splitNumberSelected < 0) {
        m_splitNumberSelected = 0;
    }

    if (!m_splitNumberPositions.empty()) {
        m_textBox->SetSelection(m_splitNumberPositions[m_splitNumberSelected],
                                m_splitNumberPositions[m_splitNumberSelected + 1]);
        m_textBox->ShowPosition(m_splitNumberPositions[m_splitNumberSelected]);
        m_splitNumberLabel->SetLabel(
            std::to_wstring(m_splitNumberSelected / 2 + 1) + L"/" +
            std::to_wstring(m_splitNumberPositions.size() / 2) + L"\t");
    }
}

void MainWindow::selectPreviousSplitNumber(wxCommandEvent& event)
{
    m_splitNumberSelected -= 2;
    if (m_splitNumberSelected >= static_cast<int>(m_splitNumberPositions.size()) || 
        m_splitNumberSelected < 0) {
        m_splitNumberSelected = m_splitNumberPositions.size() - 2;
    }

    if (!m_splitNumberPositions.empty()) {
        m_textBox->SetSelection(m_splitNumberPositions[m_splitNumberSelected],
                                m_splitNumberPositions[m_splitNumberSelected + 1]);
        m_textBox->ShowPosition(m_splitNumberPositions[m_splitNumberSelected]);
        m_splitNumberLabel->SetLabel(
            std::to_wstring(m_splitNumberSelected / 2 + 1) + L"/" +
            std::to_wstring(m_splitNumberPositions.size() / 2) + L"\t");
    }
}

void MainWindow::selectNextWrongArticle(wxCommandEvent& event)
{
    m_wrongArticleSelected += 2;
    if (m_wrongArticleSelected >= static_cast<int>(m_wrongArticlePositions.size()) || 
        m_wrongArticleSelected < 0) {
        m_wrongArticleSelected = 0;
    }

    if (!m_wrongArticlePositions.empty()) {
        m_textBox->SetSelection(m_wrongArticlePositions[m_wrongArticleSelected],
                                m_wrongArticlePositions[m_wrongArticleSelected + 1]);
        m_textBox->ShowPosition(m_wrongArticlePositions[m_wrongArticleSelected]);
        m_wrongArticleLabel->SetLabel(
            std::to_wstring(m_wrongArticleSelected / 2 + 1) + L"/" +
            std::to_wstring(m_wrongArticlePositions.size() / 2) + L"\t");
    }
}

void MainWindow::selectPreviousWrongArticle(wxCommandEvent& event)
{
    m_wrongArticleSelected -= 2;
    if (m_wrongArticleSelected >= static_cast<int>(m_wrongArticlePositions.size()) || 
        m_wrongArticleSelected < 0) {
        m_wrongArticleSelected = m_wrongArticlePositions.size() - 2;
    }

    if (!m_wrongArticlePositions.empty()) {
        m_textBox->SetSelection(m_wrongArticlePositions[m_wrongArticleSelected],
                                m_wrongArticlePositions[m_wrongArticleSelected + 1]);
        m_textBox->ShowPosition(m_wrongArticlePositions[m_wrongArticleSelected]);
        m_wrongArticleLabel->SetLabel(
            std::to_wstring(m_wrongArticleSelected / 2 + 1) + L"/" +
            std::to_wstring(m_wrongArticlePositions.size() / 2) + L"\t");
    }
}

void MainWindow::setupUi()
{
    wxPanel* panel = new wxPanel(this, wxID_ANY);

    wxBoxSizer* viewSizer = new wxBoxSizer(wxHORIZONTAL);
    panel->SetSizer(viewSizer);

    wxBoxSizer* outputSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* numberSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* noNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* wrongNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* splitNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* wrongArticleSizer = new wxBoxSizer(wxHORIZONTAL);

    m_notebookList = new wxNotebook(panel, wxID_ANY);

    // Main text editor
    m_textBox = new wxRichTextCtrl(panel);
    m_bzList = std::make_shared<wxRichTextCtrl>(
        m_notebookList, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(250, -1));

    viewSizer->Add(m_textBox, 1, wxEXPAND | wxALL, 10);
    viewSizer->Add(outputSizer, 0, wxEXPAND, 10);

    // Tree list for displaying BZ-term mappings
    m_treeList = std::make_shared<wxTreeListCtrl>(
        m_notebookList, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    m_treeList->AppendColumn("BZeichen");
    m_treeList->AppendColumn("Merkmal");

    outputSizer->Add(m_notebookList, 2, wxEXPAND | wxALL, 10);
    m_notebookList->AddPage(m_treeList.get(), "List");
    m_notebookList->AddPage(m_bzList.get(), "Text Edit");

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
    auto noNumberDescription = new wxStaticText(
        panel, wxID_ANY, "Unnumbered", wxDefaultPosition,
        wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
    m_noNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
    noNumberSizer->Add(m_noNumberLabel.get(), 0, wxLEFT, 10);
    noNumberSizer->Add(noNumberDescription, 0, wxLEFT, 0);

    // Layout for wrong number row
    wrongNumberSizer->Add(m_buttonBackwardWrongNumber.get());
    wrongNumberSizer->Add(m_buttonForwardWrongNumber.get());
    auto wrongNumberDescription = new wxStaticText(
        panel, wxID_ANY, "Multiple Words per Number", wxDefaultPosition,
        wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
    m_wrongNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
    wrongNumberSizer->Add(m_wrongNumberLabel.get(), 0, wxLEFT, 10);
    wrongNumberSizer->Add(wrongNumberDescription, 0, wxLEFT, 0);

    // Layout for split number row
    splitNumberSizer->Add(m_buttonBackwardSplitNumber.get());
    splitNumberSizer->Add(m_buttonForwardSplitNumber.get());
    auto splitNumberDescription = new wxStaticText(
        panel, wxID_ANY, "Multiple Numbers per Word", wxDefaultPosition,
        wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
    m_splitNumberLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
    splitNumberSizer->Add(m_splitNumberLabel.get(), 0, wxLEFT, 10);
    splitNumberSizer->Add(splitNumberDescription, 0, wxLEFT, 0);

    // Layout for wrong article row
    wrongArticleSizer->Add(m_buttonBackwardWrongArticle.get());
    wrongArticleSizer->Add(m_buttonForwardWrongArticle.get());
    auto wrongArticleDescription = new wxStaticText(
        panel, wxID_ANY, "Wrong Article", wxDefaultPosition,
        wxSize(150, -1), wxST_ELLIPSIZE_END | wxALIGN_LEFT);
    m_wrongArticleLabel = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t");
    wrongArticleSizer->Add(m_wrongArticleLabel.get(), 0, wxLEFT, 10);
    wrongArticleSizer->Add(wrongArticleDescription, 0, wxLEFT, 0);

    numberSizer->Add(noNumberSizer, wxLEFT);
    numberSizer->Add(wrongNumberSizer, wxLEFT);
    numberSizer->Add(splitNumberSizer, wxLEFT);
    numberSizer->Add(wrongArticleSizer, wxLEFT);

    outputSizer->Add(numberSizer, 0, wxALL, 10);

    // Set up text styles
    m_neutralStyle.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    m_warningStyle.SetBackgroundColour(*wxYELLOW);
    m_articleWarningStyle.SetBackgroundColour(*wxCYAN);
}

void MainWindow::setupBindings()
{
    m_textBox->Bind(wxEVT_TEXT, &MainWindow::debounceFunc, this);
    m_debounceTimer.Bind(wxEVT_TIMER, &MainWindow::scanText, this);

    m_buttonBackwardNoNumber->Bind(wxEVT_BUTTON, &MainWindow::selectPreviousNoNumber, this);
    m_buttonForwardNoNumber->Bind(wxEVT_BUTTON, &MainWindow::selectNextNoNumber, this);
    m_buttonBackwardWrongNumber->Bind(wxEVT_BUTTON, &MainWindow::selectPreviousWrongNumber, this);
    m_buttonForwardWrongNumber->Bind(wxEVT_BUTTON, &MainWindow::selectNextWrongNumber, this);
    m_buttonBackwardSplitNumber->Bind(wxEVT_BUTTON, &MainWindow::selectPreviousSplitNumber, this);
    m_buttonForwardSplitNumber->Bind(wxEVT_BUTTON, &MainWindow::selectNextSplitNumber, this);
    m_buttonBackwardWrongArticle->Bind(wxEVT_BUTTON, &MainWindow::selectPreviousWrongArticle, this);
    m_buttonForwardWrongArticle->Bind(wxEVT_BUTTON, &MainWindow::selectNextWrongArticle, this);

    m_treeList->Bind(wxEVT_TREELIST_ITEM_CONTEXT_MENU, &MainWindow::onTreeListContextMenu, this);
}

void MainWindow::fillBzList()
{
    m_bzList->SetValue("");

    auto treeItem = m_treeList->GetFirstItem();
    while (treeItem.IsOk()) {
        std::wstring bz = m_treeList->GetItemText(treeItem, 0).ToStdWstring();
        
        if (m_bzToPositions.count(bz) && m_bzToPositions[bz].size() >= 2) {
            size_t start = m_bzToPositions[bz][0];
            size_t len = m_bzToPositions[bz][1];
            
            // Extract the term without the BZ number
            size_t termLen = len > bz.size() + 1 ? len - bz.size() - 1 : 0;
            std::wstring termText = m_fullText.substr(start, termLen);

            m_bzList->AppendText(bz + L"\t" + termText + L"\n");
        }

        treeItem = m_treeList->GetNextItem(treeItem);
    }
}

void MainWindow::onTreeListContextMenu(wxTreeListEvent& event)
{
    wxTreeListItem item = event.GetItem();
    if (!item.IsOk()) {
        return;
    }

    wxString bzText = m_treeList->GetItemText(item, 0);
    std::wstring bz = bzText.ToStdWstring();

    // Get the stems for this BZ to determine the base word
    if (m_bzToStems.count(bz) && !m_bzToStems[bz].empty()) {
        // Get the first stem vector
        const StemVector& firstStem = *m_bzToStems[bz].begin();
        
        if (firstStem.empty()) {
            return;
        }

        // The base stem is the last element (for single-word: the only element,
        // for multi-word: the second word like "lager")
        std::wstring baseStem = firstStem.back();

        // Create context menu
        wxMenu menu;
        bool isMultiWord = m_multiWordBaseStems.count(baseStem) > 0;
        menu.Append(1, isMultiWord ? "Disable multi-word mode" : "Enable multi-word mode");

        int selection = GetPopupMenuSelectionFromUser(menu);
        if (selection == 1) {
            toggleMultiWordTerm(baseStem);
        }
    }
}

void MainWindow::toggleMultiWordTerm(const std::wstring& baseStem)
{
    if (m_multiWordBaseStems.count(baseStem)) {
        m_multiWordBaseStems.erase(baseStem);
    } else {
        m_multiWordBaseStems.insert(baseStem);
    }

    // Trigger rescan
    m_debounceTimer.Start(1, true);
}