#include "UIBuilder.h"
#include "MainWindow.h"

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

    // Create right panel container (will contain horizontal splitter for image + OCR)
    auto* rightPanelContainer = new wxPanel(components.splitter, wxID_ANY);
    auto* rightContainerSizer = new wxBoxSizer(wxVERTICAL);
    rightPanelContainer->SetSizer(rightContainerSizer);
    rightPanelContainer->Hide();  // Initially hidden until images are loaded

    // Create horizontal splitter for side-by-side layout (image | OCR)
    components.rightNotebook = new wxSplitterWindow(rightPanelContainer, wxID_ANY);
    rightContainerSizer->Add(components.rightNotebook, 1, wxEXPAND);

    // Create container panel for image viewer section (left side of horizontal splitter)
    auto* imageContainerPanel = new wxPanel(components.rightNotebook, wxID_ANY);
    auto* imageContainerSizer = new wxBoxSizer(wxVERTICAL);
    imageContainerPanel->SetSizer(imageContainerSizer);

    // Create zoom controls at top (initially hidden)
    wxBoxSizer *zoomSizer = new wxBoxSizer(wxHORIZONTAL);

    components.buttonZoomOut = std::make_shared<wxButton>(
        imageContainerPanel, wxID_ANY, "−", wxDefaultPosition, wxSize(40, 30));
    components.buttonZoomOut->Hide();
    zoomSizer->Add(components.buttonZoomOut.get(), 0, wxALL, 2);

    components.zoomLabel = std::make_shared<wxStaticText>(
        imageContainerPanel, wxID_ANY, "100%", wxDefaultPosition, wxSize(50, -1), wxALIGN_CENTER);
    components.zoomLabel->Hide();
    zoomSizer->Add(components.zoomLabel.get(), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

    components.buttonZoomIn = std::make_shared<wxButton>(
        imageContainerPanel, wxID_ANY, "+", wxDefaultPosition, wxSize(40, 30));
    components.buttonZoomIn->Hide();
    zoomSizer->Add(components.buttonZoomIn.get(), 0, wxALL, 2);

    components.buttonZoomReset = std::make_shared<wxButton>(
        imageContainerPanel, wxID_ANY, "Reset", wxDefaultPosition, wxSize(60, 30));
    components.buttonZoomReset->Hide();
    zoomSizer->Add(components.buttonZoomReset.get(), 0, wxALL, 2);

    imageContainerSizer->Add(zoomSizer, 0, wxALIGN_CENTER | wxALL, 5);

    // Create image viewer panel (handles scrolling itself)
    components.imageViewer = std::make_shared<ImageViewerPanel>(imageContainerPanel);
    components.imageViewer->Hide();
    components.imageViewer->SetMinSize(wxSize(100, 100));
    components.imageViewer->SetBackgroundColour(*wxLIGHT_GREY);

    // Add informational text overlay (shown when no image is loaded)
    components.imageInfoText = std::make_shared<wxStaticText>(
        imageContainerPanel, wxID_ANY,
        "Image Viewer\n\nOpen an image file\nto display it here",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    wxFont font = components.imageInfoText->GetFont();
    font.SetPointSize(12);
    components.imageInfoText->SetFont(font);
    components.imageInfoText->SetForegroundColour(wxColour(100, 100, 100));

    // Add image viewer to container (expandable)
    imageContainerSizer->Add(components.imageViewer.get(), 1, wxEXPAND);

    // Keep imagePanel reference for compatibility (but it's actually the viewer now)
    components.imagePanel = std::static_pointer_cast<wxScrolledWindow>(components.imageViewer);

    // Create image navigation controls at bottom (initially hidden)
    wxBoxSizer *imageNavSizer = new wxBoxSizer(wxHORIZONTAL);

    components.buttonPreviousImage = std::make_shared<wxButton>(
        imageContainerPanel, wxID_ANY, "<", wxDefaultPosition, wxSize(40, 30));
    components.buttonPreviousImage->Hide();
    imageNavSizer->Add(components.buttonPreviousImage.get(), 0, wxALL | wxALIGN_CENTER, 5);

    components.imageNavigationLabel = std::make_shared<wxStaticText>(
        imageContainerPanel, wxID_ANY, "0/0", wxDefaultPosition, wxSize(60, -1), wxALIGN_CENTER);
    components.imageNavigationLabel->Hide();
    imageNavSizer->Add(components.imageNavigationLabel.get(), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    components.buttonNextImage = std::make_shared<wxButton>(
        imageContainerPanel, wxID_ANY, ">", wxDefaultPosition, wxSize(40, 30));
    components.buttonNextImage->Hide();
    imageNavSizer->Add(components.buttonNextImage.get(), 0, wxALL | wxALIGN_CENTER, 5);

#ifdef HAVE_OPENCV
    // Add spacer between navigation arrows and scan button
    imageNavSizer->AddSpacer(20);

    // Add "Scan for Numbers" button on same line (initially hidden)
    components.buttonScanOCR = std::make_shared<wxButton>(
        imageContainerPanel, wxID_ANY, "Scan for Numbers",
        wxDefaultPosition, wxSize(140, 30));
    components.buttonScanOCR->Hide();
    imageNavSizer->Add(components.buttonScanOCR.get(), 0, wxALL | wxALIGN_CENTER, 5);
#endif

    imageContainerSizer->Add(imageNavSizer, 0, wxALL | wxALIGN_CENTER, 5);

#ifdef HAVE_OPENCV
    // Create OCR results panel (right side of horizontal splitter)
    components.ocrPanel = std::make_shared<wxPanel>(components.rightNotebook, wxID_ANY);
    wxBoxSizer *ocrSizer = new wxBoxSizer(wxVERTICAL);
    components.ocrPanel->SetSizer(ocrSizer);

    // OCR status label
    components.ocrStatusLabel = std::make_shared<wxStaticText>(
        components.ocrPanel.get(), wxID_ANY,
        "No OCR results yet.\nLoad an image and click 'Scan for Numbers'.",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    wxFont ocrFont = components.ocrStatusLabel->GetFont();
    ocrFont.SetPointSize(11);
    components.ocrStatusLabel->SetFont(ocrFont);
    components.ocrStatusLabel->SetForegroundColour(wxColour(100, 100, 100));
    ocrSizer->Add(components.ocrStatusLabel.get(), 0, wxALL | wxALIGN_CENTER | wxEXPAND, 20);

    // OCR results list
    components.ocrResultsList = std::make_shared<wxListCtrl>(
        components.ocrPanel.get(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL);
    components.ocrResultsList->AppendColumn("Reference Number", wxLIST_FORMAT_LEFT, 150);
    components.ocrResultsList->AppendColumn("Confidence", wxLIST_FORMAT_LEFT, 80);
    components.ocrResultsList->AppendColumn("Status", wxLIST_FORMAT_LEFT, 120);
    components.ocrResultsList->Hide();
    ocrSizer->Add(components.ocrResultsList.get(), 1, wxALL | wxEXPAND, 10);

    // Initialize horizontal splitter: image container on left, OCR on right
    components.rightNotebook->SplitVertically(imageContainerPanel, components.ocrPanel.get(), -250);
    components.rightNotebook->SetMinimumPaneSize(150);
#else
    // No OCR: just show image container
    components.rightNotebook->Initialize(imageContainerPanel);
#endif

    // Initialize main splitter with both panels visible
    components.splitter->SplitVertically(leftPanel, rightPanelContainer, -300);
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
