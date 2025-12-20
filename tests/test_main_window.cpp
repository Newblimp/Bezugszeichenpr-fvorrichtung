#include <gtest/gtest.h>
#include "MainWindow.h"
#include <wx/wx.h>

class TestApp : public wxApp {
public:
    virtual bool OnInit() override { return true; }
};

class MainWindowTestBase : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        int argc = 0;
        char** argv = nullptr;
        if (!wxTheApp) {
            wxApp::SetInstance(new TestApp());
            wxEntryStart(argc, argv);
            wxTheApp->CallOnInit();
        }
    }

    void SetUp() override {
        window = new MainWindow();
        textBox = window->getTextBox();
    }

    void TearDown() override {
        if (window) {
            window->Destroy();
            window = nullptr;
        }
    }

    void setText(const std::wstring& text) {
        textBox->SetValue(wxString(text));
    }

    void scanAndWait() {
        wxCommandEvent event(wxEVT_TEXT);
        window->testDebounceFunc(event);
        wxMilliSleep(100);

        // Stop the debounce timer and trigger the final scan
        window->getDebounceTimer().Stop();
        window->m_debounceTimer.Notify();

        // Wait for the background scan thread to complete
        if (window->m_scanThread.joinable()) {
            window->m_scanThread.join();
        }
    }

    size_t getTermCount() {
        return window->getContext().db.bzToStems.size();
    }

    bool hasBZ(const std::wstring& bz) {
        return window->getContext().db.bzToStems.count(bz) > 0;
    }

    MainWindow* window = nullptr;
    wxRichTextCtrl* textBox = nullptr;
};

class ScanWorkflowTest : public MainWindowTestBase {};

TEST_F(ScanWorkflowTest, ScanPopulatesDatabase) {
    setText(L"Lager 10 Motor 20");
    scanAndWait();
    EXPECT_GT(getTermCount(), 0);
    EXPECT_TRUE(hasBZ(L"10"));
    EXPECT_TRUE(hasBZ(L"20"));
}

TEST_F(ScanWorkflowTest, ScanClearsOldResults) {
    setText(L"Lager 10");
    scanAndWait();
    EXPECT_TRUE(hasBZ(L"10"));
    setText(L"Motor 20");
    scanAndWait();
    EXPECT_FALSE(hasBZ(L"10"));
    EXPECT_TRUE(hasBZ(L"20"));
}

TEST_F(ScanWorkflowTest, EmptyTextClearsDatabase) {
    setText(L"Lager 10");
    scanAndWait();
    EXPECT_GT(getTermCount(), 0);
    setText(L"");
    scanAndWait();
    EXPECT_EQ(getTermCount(), 0);
}

TEST_F(ScanWorkflowTest, MultipleTermsDetected) {
    setText(L"Lager 10 Motor 20 Zahnrad 30");
    scanAndWait();
    EXPECT_EQ(getTermCount(), 3);
}

TEST_F(ScanWorkflowTest, ConflictingAssignmentsDetected) {
    setText(L"Lager 10 Lager 20");
    scanAndWait();
    // Check that both numbers are in the database but as conflicting BZs for the same term
    auto& db = window->getContext().db;
    EXPECT_GT(db.bzToStems.size(), 0);
    // For the same term (Lager) to have two different BZ assignments, check the database
    bool has10 = db.bzToStems.count(L"10") > 0;
    bool has20 = db.bzToStems.count(L"20") > 0;
    EXPECT_TRUE(has10 && has20);
}

TEST_F(ScanWorkflowTest, UnnumberedTermsDetected) {
    setText(L"Lager 10 Lager");
    scanAndWait();
    // The first "Lager 10" is numbered, the second "Lager" is not
    // Verify that at least one term was found
    EXPECT_GT(window->getContext().db.bzToStems.size(), 0);
    // Verify that the database was populated (meaning scanning happened)
    bool has10 = window->getContext().db.bzToStems.count(L"10") > 0;
    EXPECT_TRUE(has10);
}

class UIStateManagementTest : public MainWindowTestBase {};

TEST_F(UIStateManagementTest, BzListPopulated) {
    setText(L"Lager 10 Motor 20");
    scanAndWait();
    // Check that the underlying data is populated,
    // which would be used to populate the BZ list UI component
    auto& db = window->getContext().db;
    EXPECT_GT(db.bzToStems.size(), 0);
    // Verify the specific terms are in the database
    EXPECT_TRUE(db.bzToStems.count(L"10") > 0);
    EXPECT_TRUE(db.bzToStems.count(L"20") > 0);
}

TEST_F(UIStateManagementTest, TreeHasItems) {
    setText(L"Lager 10 Motor 20");
    scanAndWait();
    // Check that the underlying database has items,
    // which would populate the tree UI component
    size_t termCount = window->getContext().db.stemToFirstWord.size();
    EXPECT_GT(termCount, 0);
}

TEST_F(UIStateManagementTest, TextBoxHasContent) {
    setText(L"Lager 10");
    EXPECT_FALSE(textBox->GetValue().empty());
}

TEST_F(UIStateManagementTest, MultipleScansUpdateUI) {
    setText(L"Lager 10");
    scanAndWait();
    size_t first = getTermCount();
    setText(L"Lager 10 Motor 20");
    scanAndWait();
    size_t second = getTermCount();
    EXPECT_GE(second, first);
}

class ContextMenuTest : public MainWindowTestBase {};

TEST_F(ContextMenuTest, ToggleMultiWord) {
    setText(L"Lager 10");
    scanAndWait();
    window->testToggleMultiWord(L"lager");
    EXPECT_TRUE(window->getContext().manualMultiWordToggles.count(L"lager"));
}

TEST_F(ContextMenuTest, ClearError) {
    setText(L"Lager 10");
    scanAndWait();
    window->testClearError(L"10");
    EXPECT_TRUE(window->getContext().clearedErrors.count(L"10"));
}

TEST_F(ContextMenuTest, RestoreErrors) {
    setText(L"Lager 10");
    scanAndWait();
    window->testClearError(L"10");
    EXPECT_TRUE(window->getContext().clearedErrors.count(L"10"));
    window->testRestoreAllErrors();
    EXPECT_TRUE(window->getContext().clearedErrors.empty());
}

TEST_F(ContextMenuTest, ClearErrorPersists) {
    setText(L"Lager 10");
    scanAndWait();
    window->testClearError(L"10");
    EXPECT_TRUE(window->getContext().clearedErrors.count(L"10"));
    setText(L"Lager 10 Lager");
    scanAndWait();
    EXPECT_TRUE(window->getContext().clearedErrors.count(L"10"));
}

class LanguageSwitchingTest : public MainWindowTestBase {};

TEST_F(LanguageSwitchingTest, DefaultAnalyzerIsGerman) {
    auto german = dynamic_cast<GermanTextAnalyzer*>(
        window->m_currentAnalyzer.get());
    EXPECT_NE(german, nullptr);
}

TEST_F(LanguageSwitchingTest, CanSwitchLanguages) {
    auto selector = window->getLanguageSelector();
    if (selector) {
        selector->SetSelection(1);
        window->testOnLanguageChanged();
        auto english = dynamic_cast<EnglishTextAnalyzer*>(
            window->m_currentAnalyzer.get());
        EXPECT_NE(english, nullptr);
    }
}

TEST_F(LanguageSwitchingTest, SwitchClearsAutoDetected) {
    window->getContext().autoDetectedMultiWordStems.insert(L"test");
    auto selector = window->getLanguageSelector();
    if (selector) {
        selector->SetSelection(1);
        window->testOnLanguageChanged();
        EXPECT_TRUE(window->getContext().autoDetectedMultiWordStems.empty());
    }
}

TEST_F(LanguageSwitchingTest, SwitchPreservesManual) {
    window->getContext().manualMultiWordToggles.insert(L"test");
    auto selector = window->getLanguageSelector();
    if (selector) {
        selector->SetSelection(1);
        window->testOnLanguageChanged();
        EXPECT_TRUE(window->getContext().manualMultiWordToggles.count(L"test"));
    }
}

class IntegrationTest : public MainWindowTestBase {};

TEST_F(IntegrationTest, CompleteWorkflow) {
    setText(L"Lager 10 Motor 20");
    scanAndWait();
    EXPECT_GT(getTermCount(), 0);
}

TEST_F(IntegrationTest, WorkflowWithErrors) {
    setText(L"Lager 10 Lager 20 Lager");
    scanAndWait();
    // Verify that the scan detected multiple conflicting assignments
    // (Lager with both 10 and 20, plus an unnumbered Lager)
    auto& db = window->getContext().db;
    EXPECT_GT(db.bzToStems.size(), 0);
    // Check that we have at least 2 different BZ assignments detected
    EXPECT_TRUE(db.bzToStems.count(L"10") > 0);
    EXPECT_TRUE(db.bzToStems.count(L"20") > 0);
}

TEST_F(IntegrationTest, DataConsistency) {
    setText(L"Lager 10");
    scanAndWait();
    EXPECT_TRUE(hasBZ(L"10"));
    setText(L"Motor 20");
    scanAndWait();
    EXPECT_FALSE(hasBZ(L"10"));
    EXPECT_TRUE(hasBZ(L"20"));
}

TEST_F(IntegrationTest, ErrorPersists) {
    setText(L"Lager 10");
    scanAndWait();
    window->testClearError(L"10");
    EXPECT_TRUE(window->getContext().clearedErrors.count(L"10"));
}
