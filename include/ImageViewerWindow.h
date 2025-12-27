#pragma once
#include <wx/frame.h>
#include <wx/toolbar.h>
#include <wx/statusbr.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <memory>
#include "ImageCanvas.h"
#include "ImageDocument.h"

#ifdef HAVE_OCR_SUPPORT
#include "IOcrEngine.h"
#include "DrawingAnalyzer.h"
#include "ReferencePanel.h"
#endif

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

#ifdef HAVE_OCR_SUPPORT
    // OCR support
    void setReferenceDatabase(const ReferenceDatabase* db);
#endif

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

#ifdef HAVE_OCR_SUPPORT
    // OCR handlers
    void onRunOcr(wxCommandEvent& event);
    void onReferenceSelected(wxCommandEvent& event);
    void highlightReference(const DetectedReference& ref);
#endif

    // UI components
    ImageCanvas* m_canvas;
    wxToolBar* m_toolbar;
    wxStatusBar* m_statusBar;
    wxStaticText* m_pageLabel;  // "Page X / Y" display
#ifdef HAVE_OCR_SUPPORT
    ReferencePanel* m_referencePanel;
#else
    wxPanel* m_referencePanelPlaceholder;  // For Phase 3
#endif

    // Document state
    ImageDocument m_document;
    size_t m_currentPage{0};

#ifdef HAVE_OCR_SUPPORT
    // OCR components
    std::unique_ptr<IOcrEngine> m_ocrEngine;
    std::unique_ptr<DrawingAnalyzer> m_analyzer;
    std::vector<OcrResult> m_ocrResults;  // OCR results for all pages
#endif

    // Toolbar button IDs
    enum {
        ID_PREV_PAGE = wxID_HIGHEST + 1,
        ID_NEXT_PAGE,
        ID_ZOOM_IN,
        ID_ZOOM_OUT,
        ID_ZOOM_FIT,
        ID_ZOOM_ACTUAL,
#ifdef HAVE_OCR_SUPPORT
        ID_RUN_OCR
#endif
    };
};
