#pragma once
#include "german_stem.h"
#include "utils.h"
#include <QMainWindow>
#include <QTextEdit>
#include <QTabWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QTextCharFormat>
#include <map>
#include <memory>
#include <regex>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    // Core scanning logic
    void scanText();
    void debounceFunc();

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
    void onTreeItemContextMenu(const QPoint &pos);
    void onTreeItemActivated(QTreeWidgetItem *item, int column);

private:
    // Setup methods
    void setupUi();
    void setupBindings();
    void loadIcons();
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

    // Context menu handling
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

    stemming::german_stem<> m_germanStemmer;
    std::wstring m_fullText;

    // Text styles
    QTextCharFormat m_neutralStyle;
    QTextCharFormat m_warningStyle;
    QTextCharFormat m_articleWarningStyle;

    // Debounce timer for text changes
    QTimer m_debounceTimer;

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
    QTabWidget* m_notebookList;
    QTextEdit* m_textBox;
    std::shared_ptr<QTextEdit> m_bzList;
    std::shared_ptr<QTreeWidget> m_treeList;

    // Navigation buttons
    std::shared_ptr<QPushButton> m_buttonForwardNoNumber;
    std::shared_ptr<QPushButton> m_buttonBackwardNoNumber;
    std::shared_ptr<QPushButton> m_buttonForwardWrongNumber;
    std::shared_ptr<QPushButton> m_buttonBackwardWrongNumber;
    std::shared_ptr<QPushButton> m_buttonForwardSplitNumber;
    std::shared_ptr<QPushButton> m_buttonBackwardSplitNumber;
    std::shared_ptr<QPushButton> m_buttonForwardWrongArticle;
    std::shared_ptr<QPushButton> m_buttonBackwardWrongArticle;

    // Error position lists: stores alternating (start, end) positions
    std::vector<int> m_noNumberPositions;
    int m_noNumberSelected{-2};
    std::shared_ptr<QLabel> m_noNumberLabel;

    std::vector<int> m_wrongNumberPositions;
    int m_wrongNumberSelected{-2};
    std::shared_ptr<QLabel> m_wrongNumberLabel;

    std::vector<int> m_splitNumberPositions;
    int m_splitNumberSelected{-2};
    std::shared_ptr<QLabel> m_splitNumberLabel;

    std::vector<int> m_wrongArticlePositions;
    int m_wrongArticleSelected{-2};
    std::shared_ptr<QLabel> m_wrongArticleLabel;
};
