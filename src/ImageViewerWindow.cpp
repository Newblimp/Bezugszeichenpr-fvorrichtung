#include "ImageViewerWindow.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

ImageViewerWindow::ImageViewerWindow(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "Image Viewer",
              wxDefaultPosition, wxSize(1024, 768)) {

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

    // Placeholder panel for future reference signs (right pane)
    // Initially hidden, will be populated in Phase 3
    m_referencePanelPlaceholder = new wxPanel(splitter, wxID_ANY);
    m_referencePanelPlaceholder->SetBackgroundColour(wxColour(240, 240, 240));

    // For now, only show the canvas (unsplit)
    splitter->Initialize(m_canvas);
    // Later in Phase 3: splitter->SplitVertically(m_canvas, m_referencePanelPlaceholder);

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

    m_toolbar->Realize();
}

void ImageViewerWindow::setupStatusBar() {
    m_statusBar = CreateStatusBar(3);
    int widths[3] = {-1, 120, 80};  // Proportional, fixed, fixed
    m_statusBar->SetStatusWidths(3, widths);
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

    // Canvas paint event to update status bar
    m_canvas->Bind(wxEVT_PAINT, &ImageViewerWindow::onCanvasPaint, this);
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

    const wxImage& img = m_document.getPage(m_currentPage);
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

    const wxImage& img = m_document.getPage(m_currentPage);
    m_canvas->setImage(img);
}

void ImageViewerWindow::onOpenFile(wxCommandEvent& event) {
    wxFileDialog openDialog(
        this,
        "Open Image File(s)",
        "", "",
        "Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif)|"
        "*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif|"
        "All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE
    );

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
