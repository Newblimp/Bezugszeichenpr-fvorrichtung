#pragma once
#include <wx/frame.h>
#include <wx/toolbar.h>
#include <wx/statusbr.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <memory>
#include "ImageCanvas.h"
#include "ImageDocument.h"

/**
 * @brief Main window for viewing images with zoom, pan, and multi-page support
 *
 * Features:
 * - File menu with Open Image (Ctrl+O)
 * - Toolbar with page navigation and zoom controls
 * - Status bar showing image path, dimensions, and zoom level
 * - Splitter for future reference panel (Phase 3)
 */
class ImageViewerWindow : public wxFrame {
public:
    explicit ImageViewerWindow(wxWindow* parent);

    // Document management
    bool openFile(const wxString& path);
    bool openFiles(const wxArrayString& paths);
    void closeDocument();

    // Page navigation
    void goToPage(size_t pageIndex);
    void nextPage();
    void previousPage();
    size_t getCurrentPage() const;
    size_t getPageCount() const;

private:
    void setupUI();
    void setupMenuBar();
    void setupToolbar();
    void setupStatusBar();
    void setupBindings();
    void updateStatusBar();
    void updateToolbarState();
    void updatePageDisplay();

    // Menu/toolbar handlers
    void onOpenFile(wxCommandEvent& event);
    void onClose(wxCommandEvent& event);
    void onZoomIn(wxCommandEvent& event);
    void onZoomOut(wxCommandEvent& event);
    void onZoomFit(wxCommandEvent& event);
    void onZoomActual(wxCommandEvent& event);
    void onNextPage(wxCommandEvent& event);
    void onPreviousPage(wxCommandEvent& event);

    // Zoom update callback
    void onCanvasPaint(wxPaintEvent& event);

    // UI components
    ImageCanvas* m_canvas;
    wxToolBar* m_toolbar;
    wxStatusBar* m_statusBar;
    wxStaticText* m_pageLabel;  // "Page X / Y" display
    wxPanel* m_referencePanelPlaceholder;  // For Phase 3

    // Document state
    ImageDocument m_document;
    size_t m_currentPage{0};

    // Toolbar button IDs
    enum {
        ID_PREV_PAGE = wxID_HIGHEST + 1,
        ID_NEXT_PAGE,
        ID_ZOOM_IN,
        ID_ZOOM_OUT,
        ID_ZOOM_FIT,
        ID_ZOOM_ACTUAL
    };
};
