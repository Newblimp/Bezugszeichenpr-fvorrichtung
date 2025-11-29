#pragma once
#include "german_stem.h"
#include "utils.h"
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Pack.H>
#include <map>
#include <memory>
#include <regex>

class MainWindow : public Fl_Double_Window {
public:
    MainWindow();
    ~MainWindow();

private:
    // Setup methods
    void setupUi();
    void setupBindings();
    void loadIcons();

    // Core scanning logic
    void scanText();
    void debounceFunc();
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
    void selectNextNoNumber();
    void selectPreviousNoNumber();
    void selectNextWrongNumber();
    void selectPreviousWrongNumber();
    void selectNextSplitNumber();
    void selectPreviousSplitNumber();
    void selectNextWrongArticle();
    void selectPreviousWrongArticle();

    // Context menu handling
    void onTreeListContextMenu();
    void onTreeListItemActivated();
    void toggleMultiWordTerm(const std::wstring& baseStem);

    // Static callbacks for FLTK
    static void debounce_callback(void* data);
    static void scan_timer_callback(void* data);
    static void button_callback(Fl_Widget* w, void* data);
    static void text_changed_callback(int pos, int nInserted, int nDeleted,
                                     int nRestyled, const char* deletedText, void* data);
    static void tree_callback(Fl_Widget* w, void* data);

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

    stemming::german_stem<> m_germanStemmer;
    std::wstring m_fullText;

    // Text styles (FLTK uses style tables)
    Fl_Text_Display::Style_Table_Entry* m_styleTable;
    int m_styleTableSize;

    // Debounce timer tracking
    bool m_timerScheduled;
    double m_debounceDelay;

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
    Fl_Tabs* m_notebookList;
    Fl_Text_Editor* m_textBox;
    Fl_Text_Buffer* m_textBuffer;
    Fl_Text_Buffer* m_styleBuffer;
    Fl_Text_Display* m_bzList;
    Fl_Text_Buffer* m_bzListBuffer;
    Fl_Tree* m_treeList;

    // Navigation buttons
    Fl_Button* m_buttonForwardNoNumber;
    Fl_Button* m_buttonBackwardNoNumber;
    Fl_Button* m_buttonForwardWrongNumber;
    Fl_Button* m_buttonBackwardWrongNumber;
    Fl_Button* m_buttonForwardSplitNumber;
    Fl_Button* m_buttonBackwardSplitNumber;
    Fl_Button* m_buttonForwardWrongArticle;
    Fl_Button* m_buttonBackwardWrongArticle;

    // Error position lists: stores alternating (start, end) positions
    std::vector<int> m_noNumberPositions;
    int m_noNumberSelected{-2};
    Fl_Box* m_noNumberLabel;

    std::vector<int> m_wrongNumberPositions;
    int m_wrongNumberSelected{-2};
    Fl_Box* m_wrongNumberLabel;

    std::vector<int> m_splitNumberPositions;
    int m_splitNumberSelected{-2};
    Fl_Box* m_splitNumberLabel;

    std::vector<int> m_wrongArticlePositions;
    int m_wrongArticleSelected{-2};
    Fl_Box* m_wrongArticleLabel;
};
