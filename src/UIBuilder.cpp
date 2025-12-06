#include "UIBuilder.h"

UIBuilder::UIComponents UIBuilder::buildUI(wxFrame* parent) {
    UIComponents components;

    // Create menu bar
    createMenuBar(parent);

    wxPanel *panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer *viewSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *outputSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *numberSizer = new wxBoxSizer(wxVERTICAL);

    components.notebookList = new wxNotebook(panel, wxID_ANY);

    // Language selector
    wxArrayString languages;
    languages.Add("German");
    languages.Add("English");
    components.languageSelector = new wxRadioBox(panel, wxID_ANY, "Language",
                                                wxDefaultPosition, wxDefaultSize,
                                                languages, 1, wxRA_SPECIFY_COLS);
    components.languageSelector->SetSelection(0); // Default to German

    // Main text editor
    components.textBox = new wxRichTextCtrl(panel);
    components.bzList = std::make_shared<wxRichTextCtrl>(
        components.notebookList, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(350, -1));

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(viewSizer, 1, wxEXPAND);
    panel->SetSizer(mainSizer);

    viewSizer->Add(components.textBox, 1, wxEXPAND | wxALL, 10);
    viewSizer->Add(outputSizer, 0, wxEXPAND, 10);

    // Tree list for displaying BZ-term mappings
    components.treeList = std::make_shared<wxTreeListCtrl>(
        components.notebookList, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    components.treeList->AppendColumn("reference sign");
    components.treeList->AppendColumn("feature");

    outputSizer->Add(components.notebookList, 2, wxEXPAND | wxALL, 10);
    components.notebookList->AddPage(components.treeList.get(), "overview");
    components.notebookList->AddPage(components.bzList.get(), "reference sign list");

    // Create navigation rows for each error type
    wxBoxSizer *allErrorsSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(panel, allErrorsSizer, "all errors",
                       components.buttonBackwardAllErrors,
                       components.buttonForwardAllErrors,
                       components.allErrorsLabel, 45);

    wxBoxSizer *noNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(panel, noNumberSizer, "unnumbered",
                       components.buttonBackwardNoNumber,
                       components.buttonForwardNoNumber,
                       components.noNumberLabel);

    wxBoxSizer *wrongNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(panel, wrongNumberSizer, "inconsistent terms",
                       components.buttonBackwardWrongNumber,
                       components.buttonForwardWrongNumber,
                       components.wrongNumberLabel);

    wxBoxSizer *splitNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(panel, splitNumberSizer, "inconsistent reference signs",
                       components.buttonBackwardSplitNumber,
                       components.buttonForwardSplitNumber,
                       components.splitNumberLabel);

    wxBoxSizer *wrongArticleSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(panel, wrongArticleSizer, "inconsistent article",
                       components.buttonBackwardWrongArticle,
                       components.buttonForwardWrongArticle,
                       components.wrongArticleLabel);

    numberSizer->Add(allErrorsSizer, wxLEFT);
    numberSizer->AddSpacer(5);
    numberSizer->Add(noNumberSizer, wxLEFT);
    numberSizer->Add(wrongNumberSizer, wxLEFT);
    numberSizer->Add(splitNumberSizer, wxLEFT);
    numberSizer->Add(wrongArticleSizer, wxLEFT);

    // Create horizontal sizer for error navigation and language selector
    wxBoxSizer *bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    bottomSizer->Add(numberSizer, 0, wxALL, 0);
    bottomSizer->AddStretchSpacer(1);
    bottomSizer->Add(components.languageSelector, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    outputSizer->Add(bottomSizer, 0, wxALL | wxEXPAND, 10);

    return components;
}

void UIBuilder::createMenuBar(wxFrame* parent) {
    wxMenuBar *menuBar = new wxMenuBar();
    wxMenu *toolsMenu = new wxMenu();

    toolsMenu->Append(wxID_HIGHEST + 20, "Restore all errors");
    toolsMenu->Append(wxID_HIGHEST + 21, "Restore cleared textbox errors");
    toolsMenu->Append(wxID_HIGHEST + 22, "Restore cleared overview errors");

    menuBar->Append(toolsMenu, "Tools");
    parent->SetMenuBar(menuBar);
}

void UIBuilder::createNavigationRow(
    wxPanel* panel,
    wxBoxSizer* sizer,
    const wxString& description,
    std::shared_ptr<wxButton>& backButton,
    std::shared_ptr<wxButton>& forwardButton,
    std::shared_ptr<wxStaticText>& label,
    int labelWidth
) {
    backButton = std::make_shared<wxButton>(
        panel, wxID_ANY, "<", wxDefaultPosition, wxSize(35, -1));
    forwardButton = std::make_shared<wxButton>(
        panel, wxID_ANY, ">", wxDefaultPosition, wxSize(35, -1));

    // Adjust button width for "all errors" row
    if (labelWidth == 45) {
        backButton = std::make_shared<wxButton>(
            panel, wxID_ANY, "<", wxDefaultPosition, wxSize(40, -1));
        forwardButton = std::make_shared<wxButton>(
            panel, wxID_ANY, ">", wxDefaultPosition, wxSize(40, -1));
    }

    sizer->Add(backButton.get());
    sizer->Add(forwardButton.get());

    auto descriptionText = new wxStaticText(panel, wxID_ANY, description,
                                           wxDefaultPosition, wxSize(150, -1),
                                           wxST_ELLIPSIZE_END | wxALIGN_LEFT);
    label = std::make_shared<wxStaticText>(panel, wxID_ANY, "0/0\t",
                                          wxDefaultPosition, wxSize(labelWidth, -1));

    sizer->Add(label.get(), 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 10);
    sizer->Add(descriptionText, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 0);
}
