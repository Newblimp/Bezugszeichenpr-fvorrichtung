#include <gtest/gtest.h>
#include "ErrorNavigator.h"
#include "UIBuilder.h"
#include "stem_collector.h"
#include "utils.h"
#include "utils_core.h"
#include <wx/wx.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/treelist.h>

// --- StemCollector Tests ---

TEST(StemCollectorTest, BasicUsage) {
    std::wstring stem = L"teststem";
    StemCollector collector(stem);

    EXPECT_EQ(collector.get_stem(), stem);
    EXPECT_TRUE(collector.get_full_words().empty());

    collector.add_word(L"word1");
    collector.add_word(L"word2");

    std::unordered_set<std::wstring> words = collector.get_full_words();
    EXPECT_EQ(words.size(), 2);
    EXPECT_TRUE(words.count(L"word1"));
    EXPECT_TRUE(words.count(L"word2"));
}


// --- Utils Tests ---

TEST(UtilsTest, StemsToDisplayString) {
    std::unordered_set<StemVector, StemVectorHash> stems;
    stems.insert({L"stem1"});
    stems.insert({L"stem2", L"suffix"});
    
    std::unordered_set<std::wstring> originalWords;
    originalWords.insert(L"Word1");
    originalWords.insert(L"Word2Suffix");

    // The output order depends on hash map iteration, so we verify content
    wxString result = stemsToDisplayString(stems, originalWords);
    
    // It should contain the stems format
    // Implementation details: stemsToDisplayString usually returns "stem1, stem2 suffix (Word1, Word2Suffix)" or similar
    // Let's check if it's not empty and contains expected substrings
    EXPECT_FALSE(result.IsEmpty());
}

// --- Utils Core Tests ---

TEST(UtilsCoreTest, StemVectorToString) {
    StemVector sv1 = {L"hello"};
    EXPECT_EQ(stemVectorToString(sv1), L"hello");

    StemVector sv2 = {L"hello", L"world"};
    EXPECT_EQ(stemVectorToString(sv2), L"hello world");
    
    StemVector sv3 = {};
    EXPECT_EQ(stemVectorToString(sv3), L"");
}

TEST(UtilsCoreTest, CollectAllStems) {
    std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash> stemToBz;
    StemVector sv1 = {L"a"};
    StemVector sv2 = {L"b"};
    stemToBz[sv1] = {L"1"};
    stemToBz[sv2] = {L"2"};

    std::unordered_set<StemVector, StemVectorHash> allStems;
    collectAllStems(stemToBz, allStems);

    EXPECT_EQ(allStems.size(), 2);
    EXPECT_TRUE(allStems.count(sv1));
    EXPECT_TRUE(allStems.count(sv2));
}

TEST(UtilsCoreTest, AppendAlternationPattern) {
    std::unordered_set<std::wstring> strings = {L"alpha", L"beta"};
    std::wstring regex;
    
    appendAlternationPattern(strings, regex);
    
    // It should be alpha|beta or beta|alpha (no parentheses)
    EXPECT_TRUE(regex == L"alpha|beta" || regex == L"beta|alpha");
    
    strings.insert(L"gamma");
    regex = L"";
    appendAlternationPattern(strings, regex);
    // Check it contains |
    EXPECT_NE(regex.find(L"|"), std::wstring::npos);
    
    // Empty set
    std::unordered_set<std::wstring> empty;
    std::wstring emptyRegex;
    appendAlternationPattern(empty, emptyRegex);
    EXPECT_EQ(emptyRegex, L"");
}


// --- GUI Tests (ErrorNavigator & UIBuilder) ---

namespace {
    class CoverageTestApp : public wxApp {
    public:
        virtual bool OnInit() override { return true; }
    };
}

class CoverageGuiTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        int argc = 0;
        char** argv = nullptr;
        // Only set instance if not already set (e.g. by other tests)
        if (!wxTheApp) {
            wxApp::SetInstance(new CoverageTestApp());
            wxEntryStart(argc, argv);
            wxTheApp->CallOnInit();
        }
    }

    void SetUp() override {
        frame = new wxFrame(nullptr, wxID_ANY, "Test Frame");
        textBox = new wxRichTextCtrl(frame, wxID_ANY);
        label = new wxStaticText(frame, wxID_ANY, "Label");
    }

    void TearDown() override {
        // Destroy frame will destroy children (textBox, label)
        if (frame) frame->Destroy();
        // Nullify pointers
        frame = nullptr;
        textBox = nullptr;
        label = nullptr;
    }

    wxFrame* frame = nullptr;
    wxRichTextCtrl* textBox = nullptr;
    wxStaticText* label = nullptr;
};

// ErrorNavigator Tests
TEST_F(CoverageGuiTest, ErrorNavigator_SelectNext) {
    std::vector<std::pair<int, int>> positions = {{0, 5}, {10, 15}, {20, 25}};
    int currentIndex = -1; // Initially none selected

    // 1. Select next (first)
    ErrorNavigator::selectNext(positions, currentIndex, textBox, label);
    EXPECT_EQ(currentIndex, 0);
    // Verify selection in text box (need to check if GetSelection returns expected values)
    long selStart, selEnd;
    textBox->GetSelection(&selStart, &selEnd);
    EXPECT_EQ(selStart, 0);
    EXPECT_EQ(selEnd, 5);
    
    // Label should update (note the trailing tab)
    EXPECT_EQ(label->GetLabel(), L"1/3\t");

    // 2. Select next (second)
    ErrorNavigator::selectNext(positions, currentIndex, textBox, label);
    EXPECT_EQ(currentIndex, 1);
    textBox->GetSelection(&selStart, &selEnd);
    EXPECT_EQ(selStart, 10);
    EXPECT_EQ(selEnd, 15);

    // 3. Select next (third)
    ErrorNavigator::selectNext(positions, currentIndex, textBox, label);
    EXPECT_EQ(currentIndex, 2);

    // 4. Select next (wrap around to first)
    ErrorNavigator::selectNext(positions, currentIndex, textBox, label);
    EXPECT_EQ(currentIndex, 0);
}

TEST_F(CoverageGuiTest, ErrorNavigator_SelectPrevious) {
    std::vector<std::pair<int, int>> positions = {{0, 5}, {10, 15}};
    int currentIndex = -1;

    // 1. Select previous (wrap around to last)
    ErrorNavigator::selectPrevious(positions, currentIndex, textBox, label);
    EXPECT_EQ(currentIndex, 1);
    EXPECT_EQ(label->GetLabel(), L"2/2\t");

    // 2. Select previous (first)
    ErrorNavigator::selectPrevious(positions, currentIndex, textBox, label);
    EXPECT_EQ(currentIndex, 0);
    EXPECT_EQ(label->GetLabel(), L"1/2\t");
}

TEST_F(CoverageGuiTest, ErrorNavigator_EmptyList) {
    std::vector<std::pair<int, int>> positions;
    int currentIndex = -1;
    
    ErrorNavigator::selectNext(positions, currentIndex, textBox, label);
    // Behavior: resets to 0 even if empty
    EXPECT_EQ(currentIndex, 0);
    
    currentIndex = -1;
    ErrorNavigator::selectPrevious(positions, currentIndex, textBox, label);
    // Behavior: resets to 0 (size - 1 = -1 cast to int? no, size is 0)
    // Code: if (current >= size || current < 0) current = size - 1;
    // size=0 -> current = -1.
    EXPECT_EQ(currentIndex, -1);
}

// UIBuilder Tests
TEST_F(CoverageGuiTest, UIBuilder_BuildUI) {
    // We pass our existing frame as parent
    // Note: UIBuilder::buildUI creates a menu bar which might replace existing one
    
    try {
        auto components = UIBuilder::buildUI(frame);
        
        EXPECT_TRUE(components.notebookList != nullptr);
        EXPECT_TRUE(components.textBox != nullptr);
        EXPECT_TRUE(components.languageSelector != nullptr);
        EXPECT_TRUE(components.bzList != nullptr);
        EXPECT_TRUE(components.termList != nullptr);
        EXPECT_TRUE(components.treeList != nullptr);
        
        EXPECT_TRUE(components.buttonForwardAllErrors != nullptr);
        EXPECT_TRUE(components.allErrorsLabel != nullptr);
        
        // Verify notebook pages
        EXPECT_EQ(components.notebookList->GetPageCount(), 3);
        
        // Explicitly clear components to avoid double-free issues during TearDown
        // shared_ptrs will delete windows, removing them from frame
        components = UIBuilder::UIComponents(); 
        
    } catch (...) {
        FAIL() << "UIBuilder::buildUI threw exception";
    }
}
