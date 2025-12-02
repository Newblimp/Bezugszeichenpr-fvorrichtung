#pragma once

#include <wx/wx.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/treelist.h>
#include <wx/notebook.h>
#include <memory>

/**
 * @brief Helper class for building the MainWindow UI
 *
 * This class contains all UI setup code separated from MainWindow
 * to improve readability and maintainability.
 */
class UIBuilder {
public:
    struct UIComponents {
        // Main components
        wxNotebook* notebookList;
        wxRichTextCtrl* textBox;
        wxRadioBox* languageSelector;
        std::shared_ptr<wxRichTextCtrl> bzList;
        std::shared_ptr<wxTreeListCtrl> treeList;

        // Navigation buttons - All errors
        std::shared_ptr<wxButton> buttonForwardAllErrors;
        std::shared_ptr<wxButton> buttonBackwardAllErrors;
        std::shared_ptr<wxStaticText> allErrorsLabel;

        // Navigation buttons - No number
        std::shared_ptr<wxButton> buttonForwardNoNumber;
        std::shared_ptr<wxButton> buttonBackwardNoNumber;
        std::shared_ptr<wxStaticText> noNumberLabel;

        // Navigation buttons - Wrong number
        std::shared_ptr<wxButton> buttonForwardWrongNumber;
        std::shared_ptr<wxButton> buttonBackwardWrongNumber;
        std::shared_ptr<wxStaticText> wrongNumberLabel;

        // Navigation buttons - Split number
        std::shared_ptr<wxButton> buttonForwardSplitNumber;
        std::shared_ptr<wxButton> buttonBackwardSplitNumber;
        std::shared_ptr<wxStaticText> splitNumberLabel;

        // Navigation buttons - Wrong article
        std::shared_ptr<wxButton> buttonForwardWrongArticle;
        std::shared_ptr<wxButton> buttonBackwardWrongArticle;
        std::shared_ptr<wxStaticText> wrongArticleLabel;
    };

    /**
     * @brief Build the complete UI for the main window
     * @param parent The parent frame
     * @return Struct containing all created UI components
     */
    static UIComponents buildUI(wxFrame* parent);

private:
    /**
     * @brief Create the menu bar
     */
    static void createMenuBar(wxFrame* parent);

    /**
     * @brief Create navigation buttons for a specific error type
     */
    static void createNavigationRow(
        wxPanel* panel,
        wxBoxSizer* sizer,
        const wxString& description,
        std::shared_ptr<wxButton>& backButton,
        std::shared_ptr<wxButton>& forwardButton,
        std::shared_ptr<wxStaticText>& label,
        int labelWidth = 55
    );
};
