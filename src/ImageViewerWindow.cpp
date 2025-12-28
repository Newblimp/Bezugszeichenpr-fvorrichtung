#include "ImageViewerWindow.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

#ifdef HAVE_OCR_SUPPORT
#include "NcnnOcrEngine.h"
#endif

ImageViewerWindow::ImageViewerWindow(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "Image Viewer",
              wxDefaultPosition, wxSize(1024, 768)) {

#ifdef HAVE_OCR_SUPPORT
    // Initialize OCR components
    m_ocrEngine = std::make_unique<NcnnOcrEngine>();
    m_analyzer = std::make_unique<DrawingAnalyzer>();
#endif

    setupUI();
    setupMenuBar();
    setupToolbar();
    setupStatusBar();
    setupBindings();
    updateToolbarState();
    updateStatusBar();
}

void ImageViewerWindow::setupUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create splitter for future reference panel
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetSashGravity(0.7);  // 70% to image, 30% to panel
    splitter->SetMinimumPaneSize(100);

    // Image canvas (left/main pane)
    m_canvas = new ImageCanvas(splitter);

#ifdef HAVE_OCR_SUPPORT
    // Reference panel for OCR results (right pane)
    m_referencePanel = new ReferencePanel(splitter);

    // For now, only show the canvas (unsplit)
    splitter->Initialize(m_canvas);
    // Will split when OCR runs: splitter->SplitVertically(m_canvas, m_referencePanel);
#else
    // Placeholder panel for future reference signs (right pane)
    // Initially hidden, will be populated in Phase 3
    m_referencePanelPlaceholder = new wxPanel(splitter, wxID_ANY);
    m_referencePanelPlaceholder->SetBackgroundColour(wxColour(240, 240, 240));

    // For now, only show the canvas (unsplit)
    splitter->Initialize(m_canvas);
    // Later in Phase 3: splitter->SplitVertically(m_canvas, m_referencePanelPlaceholder);
#endif

    mainSizer->Add(splitter, 1, wxEXPAND);
    SetSizer(mainSizer);
}

void ImageViewerWindow::setupMenuBar() {
    wxMenuBar* menuBar = new wxMenuBar();

    // File menu
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_OPEN, "&Open Image...\tCtrl+O");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_CLOSE, "&Close\tCtrl+W");
    menuBar->Append(fileMenu, "&File");

    // View menu
    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(ID_ZOOM_IN, "Zoom &In\tCtrl++");
    viewMenu->Append(ID_ZOOM_OUT, "Zoom &Out\tCtrl+-");
    viewMenu->Append(ID_ZOOM_FIT, "Zoom to &Fit\tCtrl+0");
    viewMenu->Append(ID_ZOOM_ACTUAL, "&Actual Size\tCtrl+1");
    menuBar->Append(viewMenu, "&View");

    // Navigate menu
    wxMenu* navMenu = new wxMenu();
    navMenu->Append(ID_NEXT_PAGE, "&Next Page\tPage Down");
    navMenu->Append(ID_PREV_PAGE, "&Previous Page\tPage Up");
    menuBar->Append(navMenu, "&Navigate");

    SetMenuBar(menuBar);
}

void ImageViewerWindow::setupToolbar() {
    m_toolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT);

    m_toolbar->AddTool(ID_PREV_PAGE, wxString::FromUTF8("◄"), wxNullBitmap, "Previous Page");
    m_pageLabel = new wxStaticText(m_toolbar, wxID_ANY, " Page 0/0 ",
                                    wxDefaultPosition, wxSize(80, -1), wxALIGN_CENTER);
    m_toolbar->AddControl(m_pageLabel);
    m_toolbar->AddTool(ID_NEXT_PAGE, wxString::FromUTF8("►"), wxNullBitmap, "Next Page");

    m_toolbar->AddSeparator();

    m_toolbar->AddTool(ID_ZOOM_OUT, "-", wxNullBitmap, "Zoom Out");
    m_toolbar->AddTool(ID_ZOOM_IN, "+", wxNullBitmap, "Zoom In");
    m_toolbar->AddTool(ID_ZOOM_FIT, "Fit", wxNullBitmap, "Fit to Window");
    m_toolbar->AddTool(ID_ZOOM_ACTUAL, "100%", wxNullBitmap, "Actual Size");

    m_toolbar->AddSeparator();
    m_toolbar->AddTool(ID_ROTATE_CCW, wxString::FromUTF8("↺"), wxNullBitmap, "Rotate Counter-Clockwise");
    m_toolbar->AddTool(ID_ROTATE_CW, wxString::FromUTF8("↻"), wxNullBitmap, "Rotate Clockwise");

#ifdef HAVE_OCR_SUPPORT
    m_toolbar->AddSeparator();
    m_toolbar->AddTool(ID_RUN_OCR, "Run OCR", wxNullBitmap, "Detect reference numbers in drawing");
#endif

    // Add flexible spacer to push loading indicator to right
    m_toolbar->AddStretchableSpace();

    // Add loading indicator (hidden by default)
    m_loadingIndicator = new wxActivityIndicator(m_toolbar, wxID_ANY,
                                                 wxDefaultPosition, wxSize(24, 24));
    m_loadingIndicator->Hide();
    m_toolbar->AddControl(m_loadingIndicator);

    m_toolbar->Realize();
}

void ImageViewerWindow::setupStatusBar() {
#ifdef HAVE_OCR_SUPPORT
    // Add 4th pane for OCR status
    m_statusBar = CreateStatusBar(4);
    int widths[4] = {-1, 120, 80, 100};  // Proportional, fixed, fixed, fixed
    m_statusBar->SetStatusWidths(4, widths);
#else
    m_statusBar = CreateStatusBar(3);
    int widths[3] = {-1, 120, 80};  // Proportional, fixed, fixed
    m_statusBar->SetStatusWidths(3, widths);
#endif
}

void ImageViewerWindow::setupBindings() {
    // File menu
    Bind(wxEVT_MENU, &ImageViewerWindow::onOpenFile, this, wxID_OPEN);
    Bind(wxEVT_MENU, &ImageViewerWindow::onClose, this, wxID_CLOSE);

    // View menu
    Bind(wxEVT_MENU, &ImageViewerWindow::onZoomIn, this, ID_ZOOM_IN);
    Bind(wxEVT_MENU, &ImageViewerWindow::onZoomOut, this, ID_ZOOM_OUT);
    Bind(wxEVT_MENU, &ImageViewerWindow::onZoomFit, this, ID_ZOOM_FIT);
    Bind(wxEVT_MENU, &ImageViewerWindow::onZoomActual, this, ID_ZOOM_ACTUAL);

    // Navigate menu
    Bind(wxEVT_MENU, &ImageViewerWindow::onNextPage, this, ID_NEXT_PAGE);
    Bind(wxEVT_MENU, &ImageViewerWindow::onPreviousPage, this, ID_PREV_PAGE);

    // Rotation buttons
    Bind(wxEVT_MENU, &ImageViewerWindow::onRotateCCW, this, ID_ROTATE_CCW);
    Bind(wxEVT_MENU, &ImageViewerWindow::onRotateCW, this, ID_ROTATE_CW);

    // Canvas paint event to update status bar
    m_canvas->Bind(wxEVT_PAINT, &ImageViewerWindow::onCanvasPaint, this);

#ifdef HAVE_OCR_SUPPORT
    // OCR toolbar button
    Bind(wxEVT_MENU, &ImageViewerWindow::onRunOcr, this, ID_RUN_OCR);
    // Reference selection event
    Bind(EVT_REFERENCE_SELECTED, &ImageViewerWindow::onReferenceSelected, this);
#endif
}

bool ImageViewerWindow::openFile(const wxString& path) {
    std::string stdPath = path.ToStdString();
    if (!m_document.loadFromFile(stdPath)) {
        wxMessageBox("Failed to load image: " + path,
                     "Error", wxOK | wxICON_ERROR, this);
        return false;
    }

    m_currentPage = 0;
    updatePageDisplay();
    updateToolbarState();
    updateStatusBar();
    return true;
}

bool ImageViewerWindow::openFiles(const wxArrayString& paths) {
    std::vector<std::string> stdPaths;
    for (const auto& path : paths) {
        stdPaths.push_back(path.ToStdString());
    }

    if (!m_document.loadFromFiles(stdPaths)) {
        wxMessageBox("Failed to load any images",
                     "Error", wxOK | wxICON_ERROR, this);
        return false;
    }

    m_currentPage = 0;
    updatePageDisplay();
    updateToolbarState();
    updateStatusBar();
    return true;
}

void ImageViewerWindow::closeDocument() {
    m_document.clear();
    m_canvas->clearImage();
    m_currentPage = 0;
    updatePageDisplay();
    updateToolbarState();
    updateStatusBar();
}

void ImageViewerWindow::goToPage(size_t pageIndex) {
    if (!m_document.isValidPageIndex(pageIndex)) {
        return;
    }

    m_currentPage = pageIndex;
    updatePageDisplay();
    updateToolbarState();
    updateStatusBar();

#ifdef HAVE_OCR_SUPPORT
    // Update OCR display for new page (canvas only - panel shows all pages)
    if (pageIndex < m_ocrResults.size() && m_ocrResults[pageIndex].success) {
        // Show OCR results for this page on canvas
        m_canvas->setOcrResults(m_ocrResults[pageIndex].references);
        m_statusBar->SetStatusText(
            wxString::Format("OCR: %zu refs", m_ocrResults[pageIndex].references.size()), 3
        );
    } else {
        // Clear OCR display (no results for this page yet)
        m_canvas->clearOcrResults();
        m_statusBar->SetStatusText("", 3);
    }
#endif
}

void ImageViewerWindow::nextPage() {
    if (m_currentPage + 1 < m_document.getPageCount()) {
        goToPage(m_currentPage + 1);
    }
}

void ImageViewerWindow::previousPage() {
    if (m_currentPage > 0) {
        goToPage(m_currentPage - 1);
    }
}

size_t ImageViewerWindow::getCurrentPage() const {
    return m_currentPage;
}

size_t ImageViewerWindow::getPageCount() const {
    return m_document.getPageCount();
}

void ImageViewerWindow::updateStatusBar() {
    if (!m_document.hasPages()) {
        m_statusBar->SetStatusText("No image loaded", 0);
        m_statusBar->SetStatusText("", 1);
        m_statusBar->SetStatusText("", 2);
        return;
    }

    const wxImage img = m_document.getRotatedPage(m_currentPage);
    m_statusBar->SetStatusText(
        wxString::FromUTF8(m_document.getPagePath(m_currentPage)), 0);
    m_statusBar->SetStatusText(
        wxString::Format("%dx%d", img.GetWidth(), img.GetHeight()), 1);
    m_statusBar->SetStatusText(
        wxString::Format("%.0f%%", m_canvas->getZoom() * 100), 2);
}

void ImageViewerWindow::updateToolbarState() {
    bool hasPages = m_document.hasPages();
    bool hasMultiplePages = m_document.getPageCount() > 1;
    bool canGoPrev = hasPages && m_currentPage > 0;
    bool canGoNext = hasPages && m_currentPage + 1 < m_document.getPageCount();

    m_toolbar->EnableTool(ID_PREV_PAGE, canGoPrev);
    m_toolbar->EnableTool(ID_NEXT_PAGE, canGoNext);
    m_toolbar->EnableTool(ID_ZOOM_IN, hasPages);
    m_toolbar->EnableTool(ID_ZOOM_OUT, hasPages);
    m_toolbar->EnableTool(ID_ZOOM_FIT, hasPages);
    m_toolbar->EnableTool(ID_ZOOM_ACTUAL, hasPages);
    m_toolbar->EnableTool(ID_ROTATE_CCW, hasPages);
    m_toolbar->EnableTool(ID_ROTATE_CW, hasPages);
}

void ImageViewerWindow::updatePageDisplay() {
    if (!m_document.hasPages()) {
        m_pageLabel->SetLabel(" Page 0/0 ");
        m_canvas->clearImage();
        return;
    }

    m_pageLabel->SetLabel(wxString::Format(" Page %zu/%zu ",
                                            m_currentPage + 1,
                                            m_document.getPageCount()));

    const wxImage img = m_document.getRotatedPage(m_currentPage);
    m_canvas->setImage(img);
}

void ImageViewerWindow::onOpenFile(wxCommandEvent& event) {
#ifdef HAVE_PDF_SUPPORT
    wxFileDialog openDialog(
        this,
        "Open Image/PDF File(s)",
        "", "",
        "All supported files (*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif;*.pdf)|"
        "*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif;*.pdf|"
        "Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif)|"
        "*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif|"
        "PDF files (*.pdf)|*.pdf|"
        "All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE
    );
#else
    wxFileDialog openDialog(
        this,
        "Open Image File(s)",
        "", "",
        "Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif)|"
        "*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif|"
        "All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE
    );
#endif

    if (openDialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    wxArrayString paths;
    openDialog.GetPaths(paths);

    if (paths.GetCount() == 1) {
        openFile(paths[0]);
    } else if (paths.GetCount() > 1) {
        openFiles(paths);
    }
}

void ImageViewerWindow::onClose(wxCommandEvent& event) {
    Close();
}

void ImageViewerWindow::onZoomIn(wxCommandEvent& event) {
    m_canvas->zoomIn();
    updateStatusBar();
}

void ImageViewerWindow::onZoomOut(wxCommandEvent& event) {
    m_canvas->zoomOut();
    updateStatusBar();
}

void ImageViewerWindow::onZoomFit(wxCommandEvent& event) {
    m_canvas->zoomToFit();
    updateStatusBar();
}

void ImageViewerWindow::onZoomActual(wxCommandEvent& event) {
    m_canvas->zoomToActual();
    updateStatusBar();
}

void ImageViewerWindow::onNextPage(wxCommandEvent& event) {
    nextPage();
}

void ImageViewerWindow::onPreviousPage(wxCommandEvent& event) {
    previousPage();
}

void ImageViewerWindow::onCanvasPaint(wxPaintEvent& event) {
    event.Skip();  // Let the canvas handle painting
    // Update status bar after paint to show current zoom
    CallAfter([this]() {
        updateStatusBar();
    });
}

void ImageViewerWindow::onRotateCCW(wxCommandEvent& event) {
    if (!m_document.hasPages()) return;

#ifdef HAVE_OCR_SUPPORT
    // Check if current page has OCR results
    bool hasOcrResults = m_currentPage < m_ocrResults.size() &&
                         m_ocrResults[m_currentPage].success &&
                         !m_ocrResults[m_currentPage].references.empty();

    if (hasOcrResults) {
        wxMessageDialog dialog(
            this,
            "Rotating this page will invalidate OCR results.\n"
            "Please re-run OCR after rotation for accurate detection.",
            "OCR Results Will Be Cleared",
            wxOK | wxCANCEL | wxICON_WARNING
        );

        if (dialog.ShowModal() == wxID_CANCEL) {
            return;
        }

        // Clear OCR results for this page
        m_ocrResults[m_currentPage].references.clear();
        m_ocrResults[m_currentPage].success = false;
        m_canvas->clearOcrResults();
    }
#endif

    // Rotate the page counter-clockwise
    m_document.rotatePage(m_currentPage, false);
    updatePageDisplay();
    updateStatusBar();
}

void ImageViewerWindow::onRotateCW(wxCommandEvent& event) {
    if (!m_document.hasPages()) return;

#ifdef HAVE_OCR_SUPPORT
    // Check if current page has OCR results
    bool hasOcrResults = m_currentPage < m_ocrResults.size() &&
                         m_ocrResults[m_currentPage].success &&
                         !m_ocrResults[m_currentPage].references.empty();

    if (hasOcrResults) {
        wxMessageDialog dialog(
            this,
            "Rotating this page will invalidate OCR results.\n"
            "Please re-run OCR after rotation for accurate detection.",
            "OCR Results Will Be Cleared",
            wxOK | wxCANCEL | wxICON_WARNING
        );

        if (dialog.ShowModal() == wxID_CANCEL) {
            return;
        }

        // Clear OCR results for this page
        m_ocrResults[m_currentPage].references.clear();
        m_ocrResults[m_currentPage].success = false;
        m_canvas->clearOcrResults();
    }
#endif

    // Rotate the page clockwise
    m_document.rotatePage(m_currentPage, true);
    updatePageDisplay();
    updateStatusBar();
}

#ifdef HAVE_OCR_SUPPORT
void ImageViewerWindow::setReferenceDatabase(const ReferenceDatabase* db) {
    m_analyzer->setReferenceDatabase(db);
}

void ImageViewerWindow::onRunOcr(wxCommandEvent& event) {
    if (!m_document.hasPages()) {
        wxMessageBox("No image loaded", "OCR Error", wxICON_WARNING);
        return;
    }

    size_t pageCount = m_document.getPageCount();

    // Clear previous OCR results
    m_ocrResults.clear();
    m_ocrResults.resize(pageCount);

    // Show and start loading indicator
    m_loadingIndicator->Show();
    m_loadingIndicator->Start();
    m_toolbar->Realize();

    wxBeginBusyCursor();

    // Process all pages
    size_t successCount = 0;
    size_t totalRefs = 0;

    for (size_t i = 0; i < pageCount; i++) {
        // Update status bar with progress
        m_statusBar->SetStatusText(
            wxString::Format("Running OCR... page %zu/%zu", i + 1, pageCount), 3
        );
        wxYield();  // Process events to update UI

        // CRITICAL: Use rotated image for OCR processing
        const wxImage img = m_document.getRotatedPage(i);
        m_ocrResults[i] = m_ocrEngine->processImage(
            img, i, m_document.getPagePath(i)
        );

        if (m_ocrResults[i].success) {
            // Validate results
            m_analyzer->validateResults(m_ocrResults[i]);
            successCount++;
            totalRefs += m_ocrResults[i].references.size();
        }
    }

    wxEndBusyCursor();

    // Stop and hide loading indicator
    m_loadingIndicator->Stop();
    m_loadingIndicator->Hide();
    m_toolbar->Realize();

    // Show reference panel (split if not already)
    wxSplitterWindow* splitter = wxDynamicCast(m_canvas->GetParent(), wxSplitterWindow);
    if (splitter && !splitter->IsSplit()) {
        splitter->SplitVertically(m_canvas, m_referencePanel);
        splitter->SetSashPosition(700);  // 700px for image, rest for panel
    }

    // Update reference panel with ALL results from ALL pages
    m_referencePanel->setResults(m_ocrResults);

    // Update display for current page and show completion in status bar
    if (m_currentPage < m_ocrResults.size() && m_ocrResults[m_currentPage].success) {
        m_canvas->setOcrResults(m_ocrResults[m_currentPage].references);
    }

    // Update status bar with completion message (non-blocking)
    m_statusBar->SetStatusText(
        wxString::Format("OCR complete: %zu pages, %zu refs found",
                        successCount, totalRefs),
        3  // OCR status pane
    );

    // Refresh canvas to show bounding boxes
    m_canvas->Refresh();
}

void ImageViewerWindow::onReferenceSelected(wxCommandEvent& event) {
    // Get selected reference from unified list (which includes page index)
    size_t pageIndex;
    DetectedReference ref;
    if (m_referencePanel->getSelectedReference(pageIndex, ref)) {
        // Navigate to the page if not already there
        if (pageIndex != m_currentPage) {
            goToPage(pageIndex);
        }
        // Highlight the reference
        highlightReference(ref);
    }
}

void ImageViewerWindow::highlightReference(const DetectedReference& ref) {
    m_canvas->highlightBoundingBox(ref.x, ref.y, ref.width, ref.height);
}
#endif // HAVE_OCR_SUPPORT
