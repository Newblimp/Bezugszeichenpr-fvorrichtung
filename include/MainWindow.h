#pragma once
#include "german_stem.h"
#include "utils.h"
#include "wx/notebook.h"
#include "wx/richtext/richtextctrl.h"
#include "wx/textctrl.h"
#include "wx/timer.h"
#include "wx/treelist.h"
#include <map>
#include <memory>
#include <regex>
#include <wx/dataview.h>
#include <wx/listctrl.h>
#include <wx/wx.h>

class MainWindow : public wxFrame {
public:
    MainWindow();

private:
    // Setup methods
    void setupUi();
    void setupBindings();
    void loadIcons();

    // Core scanning logic
    void scanText(wxTimerEvent& event);
    void debounceFunc(wxCommandEvent& event);
    void setupAndClear();

    // Term processing
    void stemWord(std::wstring& word);
    StemVector createStemVector(const std::wstring& word);
    StemVector createMultiWordStemVector(const std::wstring& firstWord, const std::wstring& secondWord);
    bool isMultiWordBase(const std::wstring& word);

    // Display methods
    void fillListTree();
    void fillBzList();
    bool isUniquelyAssigned(const std::wstring& bz);

    // Error detection
    void findUnnumberedWords();
    void checkArticleUsage();
    void highlightConflicts();

    // Navigation methods
    void selectNextNoNumber(wxCommandEvent& event);
    void selectPreviousNoNumber(wxCommandEvent& event);
    void selectNextWrongNumber(wxCommandEvent& event);
    void selectPreviousWrongNumber(wxCommandEvent& event);
    void selectNextSplitNumber(wxCommandEvent& event);
    void selectPreviousSplitNumber(wxCommandEvent& event);
    void selectNextWrongArticle(wxCommandEvent& event);
    void selectPreviousWrongArticle(wxCommandEvent& event);

    // Context menu handling
    void onTreeListContextMenu(wxTreeListEvent& event);
    void onTreeListItemActivated(wxTreeListEvent &event);
    void toggleMultiWordTerm(const std::wstring& baseStem);

    // Regex patterns
    // Single word + number: captures (word)(number)
    std::wregex m_singleWordRegex{
        L"(\\b[[:alpha:]äöüÄÖÜß]+\\b)[[:space:]]+(\\b[[:digit:]]+[a-zA-Z']*\\b)",
        std::regex_constants::ECMAScript | std::regex_constants::optimize |
            std::regex_constants::icase};

    // Two words + number: captures (word1)(word2)(number)
    std::wregex m_twoWordRegex{
        L"(\\b[[:alpha:]äöüÄÖÜß]+\\b)[[:space:]]+(\\b[[:alpha:]äöüÄÖÜß]+\\b)[[:space:]]+(\\b[[:digit:]]+[a-zA-Z']*\\b)",
        std::regex_constants::ECMAScript | std::regex_constants::optimize |
            std::regex_constants::icase};

    // Single word (for finding unnumbered references)
    std::wregex m_wordRegex{
        L"\\b[[:alpha:]äöüÄÖÜß]+\\b",
        std::regex_constants::ECMAScript | std::regex_constants::optimize |
            std::regex_constants::icase};

    // Single word NOT followed by a number (for finding unnumbered multi-word terms)
    std::wregex m_twoWordNoNumberRegex{
        L"(\\b[[:alpha:]äöüÄÖÜß]+\\b)(?![[:space:]]+[[:digit:]])",
        std::regex_constants::ECMAScript | std::regex_constants::optimize |
            std::regex_constants::icase};

    stemming::german_stem<> m_germanStemmer;
    std::wstring m_fullText;

    // Text styles
    wxTextAttr m_neutralStyle;
    wxTextAttr m_warningStyle;
    wxTextAttr m_articleWarningStyle;

    // Debounce timer for text changes
    wxTimer m_debounceTimer;

    // Main data structure: BZ -> set of StemVectors
    // Example: "10" -> {{"lager"}, {"zweit", "lager"}}
    std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>, BZComparatorForMap>
        m_bzToStems;

    // Reverse mapping: StemVector -> set of BZs
    // Example: {"zweit", "lager"} -> {"12"}
    std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash>
        m_stemToBz;

    // Original (unstemmed) words for display
    // BZ -> set of original word strings
    std::unordered_map<std::wstring, std::unordered_set<std::wstring>>
        m_bzToOriginalWords;

    // Position tracking for highlighting and navigation
    // BZ -> list of (start, length) pairs
    std::unordered_map<std::wstring, std::vector<std::pair<size_t,size_t>>> m_bzToPositions;
    
    // StemVector -> list of (start, length) pairs
    std::unordered_map<StemVector, std::vector<std::pair<size_t,size_t>>, StemVectorHash> m_stemToPositions;

    //keeping track of the position of the cursor when browsing occurences
    std::unordered_map<std::wstring, int> m_bzCurrentOccurrence;

    // Set of base word STEMS that should trigger multi-word matching
    // Example: if "lager" is in this set, "erstes Lager 10" becomes {"erst", "lager"}
    std::unordered_set<std::wstring> m_multiWordBaseStems;

    // All unique stems encountered
    std::unordered_set<StemVector, StemVectorHash> m_allStems;

    // UI components
    wxNotebook* m_notebookList;
    wxRichTextCtrl* m_textBox;
    std::shared_ptr<wxRichTextCtrl> m_bzList;
    std::shared_ptr<wxImageList> m_imageList;
    std::shared_ptr<wxTreeListCtrl> m_treeList;
    
    // Navigation buttons
    std::shared_ptr<wxButton> m_buttonForwardNoNumber;
    std::shared_ptr<wxButton> m_buttonBackwardNoNumber;
    std::shared_ptr<wxButton> m_buttonForwardWrongNumber;
    std::shared_ptr<wxButton> m_buttonBackwardWrongNumber;
    std::shared_ptr<wxButton> m_buttonForwardSplitNumber;
    std::shared_ptr<wxButton> m_buttonBackwardSplitNumber;
    std::shared_ptr<wxButton> m_buttonForwardWrongArticle;
    std::shared_ptr<wxButton> m_buttonBackwardWrongArticle;

    // Error position lists: stores (start, end) position pairs
    std::vector<std::pair<int, int>> m_noNumberPositions;
    int m_noNumberSelected{-1};
    std::shared_ptr<wxStaticText> m_noNumberLabel;

    std::vector<std::pair<int, int>> m_wrongNumberPositions;
    int m_wrongNumberSelected{-1};
    std::shared_ptr<wxStaticText> m_wrongNumberLabel;

    std::vector<std::pair<int, int>> m_splitNumberPositions;
    int m_splitNumberSelected{-1};
    std::shared_ptr<wxStaticText> m_splitNumberLabel;

    std::vector<std::pair<int, int>> m_wrongArticlePositions;
    int m_wrongArticleSelected{-1};
    std::shared_ptr<wxStaticText> m_wrongArticleLabel;
};
