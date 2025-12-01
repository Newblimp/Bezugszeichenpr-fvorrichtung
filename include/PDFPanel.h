#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/dnd.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>

// Forward declarations for MuPDF and Tesseract
// These will be included in the .cpp file to avoid header pollution
struct fz_context;
struct fz_document;
struct fz_page;
struct fz_pixmap;
namespace tesseract {
    class TessBaseAPI;
}

// Structure to hold detected reference sign information
struct DetectedRefSign {
    std::wstring refSign;        // The reference sign text (e.g., "10", "12a")
    wxRect boundingBox;          // Position in the image
    int pageNumber;              // Which page it was found on
    bool existsInText;           // Whether this ref sign exists in the text analysis
};

// File drop target for PDF files
class PDFDropTarget : public wxFileDropTarget {
public:
    PDFDropTarget(wxWindow* parent);
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;

private:
    wxWindow* m_parent;
};

// Main PDF panel class
class PDFPanel : public wxScrolledWindow {
public:
    PDFPanel(wxWindow* parent);
    ~PDFPanel();

    // Load and process a PDF file
    bool LoadPDF(const wxString& filepath);

    // Set the reference signs found in the text for comparison
    void SetTextReferenceSignsForComparison(const std::set<std::wstring>& refSigns);

    // Clear the current PDF
    void Clear();

    // Get detected reference signs
    const std::vector<DetectedRefSign>& GetDetectedRefSigns() const { return m_detectedRefSigns; }

private:
    // PDF rendering
    bool InitializeMuPDF();
    void CleanupMuPDF();
    bool RenderPDFPages();
    wxImage RenderPage(int pageNumber);

    // OCR processing
    bool InitializeTesseract();
    void CleanupTesseract();
    std::vector<DetectedRefSign> ProcessPageOCR(const wxImage& pageImage, int pageNumber);

    // Reference sign extraction from OCR results
    bool IsValidReferenceSign(const std::wstring& text) const;

    // Image annotation
    void AnnotateImages();
    void DrawBoundingBox(wxImage& image, const wxRect& box, const wxColour& color, int thickness = 2);

    // Comparison with text analysis
    void CompareWithTextAnalysis();

    // UI events
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

    // MuPDF context and document
    fz_context* m_fzContext;
    fz_document* m_fzDocument;

    // Tesseract OCR
    std::unique_ptr<tesseract::TessBaseAPI> m_tesseract;

    // Processed data
    std::vector<wxImage> m_pageImages;           // Rendered PDF pages
    std::vector<wxImage> m_annotatedImages;      // Pages with bounding boxes drawn
    std::vector<DetectedRefSign> m_detectedRefSigns;  // All detected ref signs
    std::set<std::wstring> m_textRefSigns;       // Ref signs from text analysis

    // Current PDF path
    wxString m_currentPDFPath;

    // Layout parameters
    int m_pageSpacing;
    int m_margin;

    wxDECLARE_EVENT_TABLE();
};
