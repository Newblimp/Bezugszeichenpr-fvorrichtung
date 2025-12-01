#pragma once

#include <wx/wx.h>
#include <wx/dnd.h>
#include <wx/scrolwin.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>

// Forward declarations
namespace tesseract {
    class TessBaseAPI;
}

struct fz_context;
struct fz_document;

// Structure to hold information about a detected reference number
struct DetectedReference {
    std::wstring number;      // The reference number (e.g., "10", "10a")
    int page;                 // Page number (0-indexed)
    int x, y, width, height;  // Bounding box
    bool existsInText;        // Whether this number exists in the scanned text
};

// Custom file drop target for PDF drag-and-drop
class PDFDropTarget : public wxFileDropTarget {
public:
    PDFDropTarget(class PDFPanel* panel) : m_panel(panel) {}
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;

private:
    PDFPanel* m_panel;
};

// Main PDF panel for displaying and annotating patent figures
class PDFPanel : public wxScrolledWindow {
public:
    PDFPanel(wxWindow* parent);
    ~PDFPanel();

    // Load and process a PDF file
    bool loadPDF(const wxString& filename);

    // Set the reference numbers from the text analysis
    void setTextReferences(const std::unordered_set<std::wstring>& refs);

    // Get all detected reference numbers from the PDF
    const std::vector<DetectedReference>& getDetectedReferences() const {
        return m_detectedReferences;
    }

    // Get reference numbers that are in PDF but not in text
    std::vector<std::wstring> getMissingInText() const;

    // Get reference numbers that are in text but not in PDF
    std::vector<std::wstring> getMissingInPDF() const;

private:
    // Rendering methods
    void renderPDFPages();
    void renderPage(int pageNum);

    // OCR methods
    void performOCR();
    void ocrPage(int pageNum, const wxImage& pageImage);

    // Drawing methods
    void annotatePages();
    void drawRectangle(wxImage& image, int x, int y, int w, int h,
                      const wxColour& color, int thickness = 2);

    // Event handlers
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);

    // Helper methods
    void clear();
    bool isReferenceNumber(const std::wstring& text) const;

    // MuPDF context and document
    fz_context* m_fzContext;
    fz_document* m_fzDocument;

    // Tesseract OCR engine
    std::unique_ptr<tesseract::TessBaseAPI> m_tesseract;

    // PDF data
    wxString m_pdfFilename;
    int m_pageCount;
    std::vector<wxBitmap> m_renderedPages;     // Rendered and annotated pages
    std::vector<wxImage> m_originalPages;      // Original page images (before annotation)

    // Reference number data
    std::unordered_set<std::wstring> m_textReferences;
    std::vector<DetectedReference> m_detectedReferences;

    // UI state
    int m_spacing;  // Spacing between pages

    wxDECLARE_EVENT_TABLE();
};
