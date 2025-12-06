#include "UIBuilder.h"

UIBuilder::UIComponents UIBuilder::buildUI(wxFrame* parent) {
    UIComponents components;

    // Create menu bar
    createMenuBar(parent);

    // Create main panel
    wxPanel *panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(mainSizer);

    // Create splitter window
    components.splitter = new wxSplitterWindow(panel, wxID_ANY);

    // Left panel (existing text editor and controls)
    wxPanel *leftPanel = new wxPanel(components.splitter, wxID_ANY);
    wxBoxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);
    leftPanel->SetSizer(leftSizer);

    wxBoxSizer *viewSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *outputSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *numberSizer = new wxBoxSizer(wxVERTICAL);

    components.notebookList = new wxNotebook(leftPanel, wxID_ANY);

    // Language selector
    wxArrayString languages;
    languages.Add("German");
    languages.Add("English");
    components.languageSelector = new wxRadioBox(leftPanel, wxID_ANY, "Language",
                                                wxDefaultPosition, wxDefaultSize,
                                                languages, 1, wxRA_SPECIFY_COLS);
    components.languageSelector->SetSelection(0); // Default to German

    // Main text editor
    components.textBox = new wxRichTextCtrl(leftPanel);
    components.bzList = std::make_shared<wxRichTextCtrl>(
        components.notebookList, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(350, -1));

    leftSizer->Add(viewSizer, 1, wxEXPAND);

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
    createNavigationRow(leftPanel, allErrorsSizer, "all errors",
                       components.buttonBackwardAllErrors,
                       components.buttonForwardAllErrors,
                       components.allErrorsLabel, 45);

    wxBoxSizer *noNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(leftPanel, noNumberSizer, "unnumbered",
                       components.buttonBackwardNoNumber,
                       components.buttonForwardNoNumber,
                       components.noNumberLabel);

    wxBoxSizer *wrongNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(leftPanel, wrongNumberSizer, "inconsistent terms",
                       components.buttonBackwardWrongNumber,
                       components.buttonForwardWrongNumber,
                       components.wrongNumberLabel);

    wxBoxSizer *splitNumberSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(leftPanel, splitNumberSizer, "inconsistent reference signs",
                       components.buttonBackwardSplitNumber,
                       components.buttonForwardSplitNumber,
                       components.splitNumberLabel);

    wxBoxSizer *wrongArticleSizer = new wxBoxSizer(wxHORIZONTAL);
    createNavigationRow(leftPanel, wrongArticleSizer, "inconsistent article",
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

    // Create image viewer panel (right side)
    components.imagePanel = std::make_shared<wxScrolledWindow>(
        components.splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxVSCROLL | wxHSCROLL);
    components.imagePanel->SetScrollRate(10, 10);
    components.imagePanel->SetBackgroundColour(*wxLIGHT_GREY);

    wxBoxSizer *imageSizer = new wxBoxSizer(wxVERTICAL);
    components.imagePanel->SetSizer(imageSizer);

    // Add informational text (shown when no image is loaded)
    components.imageInfoText = std::make_shared<wxStaticText>(
        components.imagePanel.get(), wxID_ANY,
        "Image Viewer\n\nOpen an image file\nto display it here",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    wxFont font = components.imageInfoText->GetFont();
    font.SetPointSize(12);
    components.imageInfoText->SetFont(font);
    components.imageInfoText->SetForegroundColour(wxColour(100, 100, 100));
    imageSizer->Add(components.imageInfoText.get(), 1, wxALL | wxALIGN_CENTER | wxEXPAND, 20);

    // Create image viewer (initially hidden)
    components.imageViewer = std::make_shared<wxStaticBitmap>(
        components.imagePanel.get(), wxID_ANY, wxNullBitmap);
    components.imageViewer->Hide();
    imageSizer->Add(components.imageViewer.get(), 1, wxALL | wxALIGN_CENTER, 10);

    // Create image navigation controls (initially hidden)
    wxBoxSizer *imageNavSizer = new wxBoxSizer(wxHORIZONTAL);

    components.buttonPreviousImage = std::make_shared<wxButton>(
        components.imagePanel.get(), wxID_ANY, "<", wxDefaultPosition, wxSize(40, 30));
    components.buttonPreviousImage->Hide();
    imageNavSizer->Add(components.buttonPreviousImage.get(), 0, wxALL | wxALIGN_CENTER, 5);

    components.imageNavigationLabel = std::make_shared<wxStaticText>(
        components.imagePanel.get(), wxID_ANY, "0/0", wxDefaultPosition, wxSize(60, -1), wxALIGN_CENTER);
    components.imageNavigationLabel->Hide();
    imageNavSizer->Add(components.imageNavigationLabel.get(), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    components.buttonNextImage = std::make_shared<wxButton>(
        components.imagePanel.get(), wxID_ANY, ">", wxDefaultPosition, wxSize(40, 30));
    components.buttonNextImage->Hide();
    imageNavSizer->Add(components.buttonNextImage.get(), 0, wxALL | wxALIGN_CENTER, 5);

    imageSizer->Add(imageNavSizer, 0, wxALL | wxALIGN_CENTER, 5);

    // Initialize splitter with both panels visible
    components.splitter->SplitVertically(leftPanel, components.imagePanel.get(), -300);
    components.splitter->SetMinimumPaneSize(200);

    // Add splitter to main panel
    mainSizer->Add(components.splitter, 1, wxEXPAND);

    return components;
}

void UIBuilder::createMenuBar(wxFrame* parent) {
    wxMenuBar *menuBar = new wxMenuBar();

    // File menu
    wxMenu *fileMenu = new wxMenu();
    fileMenu->Append(wxID_OPEN, "&Open Image\tCtrl+O", "Open an image file");

    // Tools menu
    wxMenu *toolsMenu = new wxMenu();
    toolsMenu->Append(wxID_HIGHEST + 20, "Restore all errors");
    toolsMenu->Append(wxID_HIGHEST + 21, "Restore cleared textbox errors");
    toolsMenu->Append(wxID_HIGHEST + 22, "Restore cleared overview errors");

    menuBar->Append(fileMenu, "&File");
    menuBar->Append(toolsMenu, "&Tools");
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
