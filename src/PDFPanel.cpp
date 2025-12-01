#include "PDFPanel.h"
#include <wx/dcbuffer.h>
#include <wx/msgdlg.h>
#include <regex>
#include <algorithm>

// MuPDF headers
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4100 4244 4456 4457 4458)
#endif
#include <mupdf/fitz.h>
#ifdef _WIN32
#pragma warning(pop)
#endif

// Tesseract headers
#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>
#include <leptonica/allheaders.h>

wxBEGIN_EVENT_TABLE(PDFPanel, wxScrolledWindow)
    EVT_PAINT(PDFPanel::OnPaint)
    EVT_SIZE(PDFPanel::OnSize)
wxEND_EVENT_TABLE()

//------------------------------------------------------------------------------
// PDFDropTarget Implementation
//------------------------------------------------------------------------------

PDFDropTarget::PDFDropTarget(wxWindow* parent)
    : m_parent(parent) {
}

bool PDFDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) {
    if (filenames.GetCount() == 0) {
        return false;
    }

    // Take the first file
    wxString filename = filenames[0];

    // Check if it's a PDF
    if (!filename.Lower().EndsWith(".pdf")) {
        wxMessageBox("Please drop a PDF file.", "Invalid File", wxOK | wxICON_WARNING);
        return false;
    }

    // Cast parent to PDFPanel and load the PDF
    PDFPanel* panel = dynamic_cast<PDFPanel*>(m_parent);
    if (panel) {
        return panel->LoadPDF(filename);
    }

    return false;
}

//------------------------------------------------------------------------------
// PDFPanel Implementation
//------------------------------------------------------------------------------

PDFPanel::PDFPanel(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxVSCROLL | wxHSCROLL | wxFULL_REPAINT_ON_RESIZE),
      m_fzContext(nullptr),
      m_fzDocument(nullptr),
      m_pageSpacing(20),
      m_margin(10) {

    SetBackgroundColour(*wxLIGHT_GREY);
    SetScrollRate(10, 10);

    // Set up drag and drop
    SetDropTarget(new PDFDropTarget(this));

    // Initialize MuPDF and Tesseract
    if (!InitializeMuPDF()) {
        wxLogError("Failed to initialize MuPDF");
    }

    if (!InitializeTesseract()) {
        wxLogError("Failed to initialize Tesseract OCR");
    }
}

PDFPanel::~PDFPanel() {
    CleanupMuPDF();
    CleanupTesseract();
}

bool PDFPanel::InitializeMuPDF() {
    // Create MuPDF context
    m_fzContext = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!m_fzContext) {
        wxLogError("Failed to create MuPDF context");
        return false;
    }

    // Register document handlers
    fz_try(m_fzContext) {
        fz_register_document_handlers(m_fzContext);
    }
    fz_catch(m_fzContext) {
        fz_drop_context(m_fzContext);
        m_fzContext = nullptr;
        return false;
    }

    return true;
}

void PDFPanel::CleanupMuPDF() {
    if (m_fzDocument) {
        fz_drop_document(m_fzContext, m_fzDocument);
        m_fzDocument = nullptr;
    }

    if (m_fzContext) {
        fz_drop_context(m_fzContext);
        m_fzContext = nullptr;
    }
}

bool PDFPanel::InitializeTesseract() {
    m_tesseract = std::make_unique<tesseract::TessBaseAPI>();

    // Initialize Tesseract with German language
    // You can change "deu" to "eng" for English or "deu+eng" for both
    if (m_tesseract->Init(nullptr, "deu", tesseract::OEM_LSTM_ONLY) != 0) {
        wxLogError("Failed to initialize Tesseract. Make sure tessdata is available.");
        m_tesseract.reset();
        return false;
    }

    // Set page segmentation mode to sparse text (best for finding numbers)
    m_tesseract->SetPageSegMode(tesseract::PSM_SPARSE_TEXT);

    // Whitelist digits and common reference sign characters
    m_tesseract->SetVariable("tessedit_char_whitelist", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'");

    return true;
}

void PDFPanel::CleanupTesseract() {
    if (m_tesseract) {
        m_tesseract->End();
        m_tesseract.reset();
    }
}

bool PDFPanel::LoadPDF(const wxString& filepath) {
    if (!m_fzContext) {
        wxMessageBox("MuPDF not initialized", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    // Clear previous data
    Clear();

    // Open PDF document
    fz_try(m_fzContext) {
        m_fzDocument = fz_open_document(m_fzContext, filepath.utf8_str().data());
    }
    fz_catch(m_fzContext) {
        wxMessageBox("Failed to open PDF: " + filepath, "Error", wxOK | wxICON_ERROR);
        return false;
    }

    m_currentPDFPath = filepath;

    // Render all pages
    if (!RenderPDFPages()) {
        wxMessageBox("Failed to render PDF pages", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    // Process OCR on all pages
    wxProgressDialog progress("Processing PDF", "Running OCR on PDF pages...",
                              m_pageImages.size(), this,
                              wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH);

    for (size_t i = 0; i < m_pageImages.size(); i++) {
        progress.Update(i, wxString::Format("Processing page %zu of %zu", i + 1, m_pageImages.size()));

        auto detectedSigns = ProcessPageOCR(m_pageImages[i], i);
        m_detectedRefSigns.insert(m_detectedRefSigns.end(),
                                  detectedSigns.begin(), detectedSigns.end());
    }

    // Compare with text analysis and annotate images
    CompareWithTextAnalysis();
    AnnotateImages();

    // Update display
    Refresh();

    wxMessageBox(wxString::Format("Found %zu reference signs in PDF", m_detectedRefSigns.size()),
                 "OCR Complete", wxOK | wxICON_INFORMATION);

    return true;
}

bool PDFPanel::RenderPDFPages() {
    if (!m_fzDocument) {
        return false;
    }

    int pageCount = fz_count_pages(m_fzContext, m_fzDocument);
    m_pageImages.clear();
    m_pageImages.reserve(pageCount);

    for (int i = 0; i < pageCount; i++) {
        wxImage img = RenderPage(i);
        if (img.IsOk()) {
            m_pageImages.push_back(img);
        }
    }

    return !m_pageImages.empty();
}

wxImage PDFPanel::RenderPage(int pageNumber) {
    fz_page* page = nullptr;
    fz_pixmap* pixmap = nullptr;
    wxImage result;

    fz_try(m_fzContext) {
        // Load page
        page = fz_load_page(m_fzContext, m_fzDocument, pageNumber);

        // Get page dimensions
        fz_rect bounds = fz_bound_page(m_fzContext, page);

        // Create transformation matrix (scale for better quality)
        float zoom = 2.0f; // 2x resolution for better OCR
        fz_matrix transform = fz_scale(zoom, zoom);

        // Render page to pixmap
        pixmap = fz_new_pixmap_from_page(m_fzContext, page, transform, fz_device_rgb(m_fzContext), 0);

        // Convert pixmap to wxImage
        int width = fz_pixmap_width(m_fzContext, pixmap);
        int height = fz_pixmap_height(m_fzContext, pixmap);
        int stride = fz_pixmap_stride(m_fzContext, pixmap);
        unsigned char* samples = fz_pixmap_samples(m_fzContext, pixmap);

        result = wxImage(width, height);
        unsigned char* imageData = result.GetData();

        // MuPDF uses RGB(A), wxImage uses RGB
        int n = fz_pixmap_components(m_fzContext, pixmap);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int srcIdx = y * stride + x * n;
                int dstIdx = (y * width + x) * 3;
                imageData[dstIdx + 0] = samples[srcIdx + 0]; // R
                imageData[dstIdx + 1] = samples[srcIdx + 1]; // G
                imageData[dstIdx + 2] = samples[srcIdx + 2]; // B
            }
        }
    }
    fz_always(m_fzContext) {
        if (pixmap) fz_drop_pixmap(m_fzContext, pixmap);
        if (page) fz_drop_page(m_fzContext, page);
    }
    fz_catch(m_fzContext) {
        wxLogError("Failed to render page %d", pageNumber);
    }

    return result;
}

std::vector<DetectedRefSign> PDFPanel::ProcessPageOCR(const wxImage& pageImage, int pageNumber) {
    std::vector<DetectedRefSign> results;

    if (!m_tesseract || !pageImage.IsOk()) {
        return results;
    }

    // Convert wxImage to Leptonica PIX format
    int width = pageImage.GetWidth();
    int height = pageImage.GetHeight();
    unsigned char* data = pageImage.GetData();

    // Create PIX from RGB data
    PIX* pix = pixCreate(width, height, 32);
    if (!pix) {
        return results;
    }

    l_uint32* pixData = pixGetData(pix);
    int wpl = pixGetWpl(pix);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int srcIdx = (y * width + x) * 3;
            unsigned char r = data[srcIdx + 0];
            unsigned char g = data[srcIdx + 1];
            unsigned char b = data[srcIdx + 2];

            l_uint32 pixel = (r << 24) | (g << 16) | (b << 8) | 0xff;
            pixData[y * wpl + x] = pixel;
        }
    }

    // Run Tesseract OCR
    m_tesseract->SetImage(pix);
    m_tesseract->Recognize(nullptr);

    // Get word-level results with bounding boxes
    tesseract::ResultIterator* ri = m_tesseract->GetIterator();
    if (ri != nullptr) {
        tesseract::PageIteratorLevel level = tesseract::RIL_WORD;

        do {
            const char* word = ri->GetUTF8Text(level);
            if (word == nullptr) continue;

            float conf = ri->Confidence(level);

            // Only consider detections with reasonable confidence
            if (conf > 30.0f) {
                std::wstring wWord = wxString::FromUTF8(word).ToStdWstring();

                // Check if this looks like a reference sign
                if (IsValidReferenceSign(wWord)) {
                    int x1, y1, x2, y2;
                    ri->BoundingBox(level, &x1, &y1, &x2, &y2);

                    DetectedRefSign sign;
                    sign.refSign = wWord;
                    sign.boundingBox = wxRect(x1, y1, x2 - x1, y2 - y1);
                    sign.pageNumber = pageNumber;
                    sign.existsInText = false; // Will be set in CompareWithTextAnalysis

                    results.push_back(sign);
                }
            }

            delete[] word;
        } while (ri->Next(level));

        delete ri;
    }

    pixDestroy(&pix);

    return results;
}

bool PDFPanel::IsValidReferenceSign(const std::wstring& text) const {
    if (text.empty()) {
        return false;
    }

    // Reference signs typically start with a digit and may have letters/apostrophes
    // Examples: "10", "12a", "10'", "100"
    std::wregex refSignPattern(L"^\\d+[a-zA-Z']*$");
    return std::regex_match(text, refSignPattern);
}

void PDFPanel::CompareWithTextAnalysis() {
    // Mark each detected ref sign as existing in text or not
    for (auto& sign : m_detectedRefSigns) {
        sign.existsInText = (m_textRefSigns.find(sign.refSign) != m_textRefSigns.end());
    }
}

void PDFPanel::AnnotateImages() {
    m_annotatedImages.clear();
    m_annotatedImages.reserve(m_pageImages.size());

    // Create annotated version of each page
    for (size_t pageNum = 0; pageNum < m_pageImages.size(); pageNum++) {
        wxImage annotated = m_pageImages[pageNum].Copy();

        // Draw bounding boxes for all detected ref signs on this page
        for (const auto& sign : m_detectedRefSigns) {
            if (sign.pageNumber == static_cast<int>(pageNum)) {
                wxColour color = sign.existsInText ? wxColour(0, 255, 0) : wxColour(255, 0, 0);
                DrawBoundingBox(annotated, sign.boundingBox, color, 3);
            }
        }

        m_annotatedImages.push_back(annotated);
    }
}

void PDFPanel::DrawBoundingBox(wxImage& image, const wxRect& box, const wxColour& color, int thickness) {
    if (!image.IsOk()) {
        return;
    }

    int width = image.GetWidth();
    int height = image.GetHeight();
    unsigned char* data = image.GetData();

    unsigned char r = color.Red();
    unsigned char g = color.Green();
    unsigned char b = color.Blue();

    auto setPixel = [&](int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            int idx = (y * width + x) * 3;
            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
        }
    };

    // Draw rectangle with thickness
    for (int t = 0; t < thickness; t++) {
        int x1 = box.x - t;
        int y1 = box.y - t;
        int x2 = box.x + box.width + t;
        int y2 = box.y + box.height + t;

        // Top and bottom edges
        for (int x = x1; x <= x2; x++) {
            setPixel(x, y1);
            setPixel(x, y2);
        }

        // Left and right edges
        for (int y = y1; y <= y2; y++) {
            setPixel(x1, y);
            setPixel(x2, y);
        }
    }
}

void PDFPanel::SetTextReferenceSignsForComparison(const std::set<std::wstring>& refSigns) {
    m_textRefSigns = refSigns;

    // Re-compare and re-annotate if we have detected signs
    if (!m_detectedRefSigns.empty()) {
        CompareWithTextAnalysis();
        AnnotateImages();
        Refresh();
    }
}

void PDFPanel::Clear() {
    m_pageImages.clear();
    m_annotatedImages.clear();
    m_detectedRefSigns.clear();
    m_currentPDFPath.clear();

    if (m_fzDocument) {
        fz_drop_document(m_fzContext, m_fzDocument);
        m_fzDocument = nullptr;
    }

    Refresh();
}

void PDFPanel::OnPaint(wxPaintEvent& event) {
    wxBufferedPaintDC dc(this);
    DoPrepareDC(dc);

    dc.SetBackground(*wxLIGHT_GREY_BRUSH);
    dc.Clear();

    if (m_annotatedImages.empty()) {
        // Draw placeholder text
        dc.SetTextForeground(*wxBLACK);
        wxFont font = dc.GetFont();
        font.SetPointSize(12);
        dc.SetFont(font);
        dc.DrawText("Drag and drop a PDF file here", 50, 50);
        dc.DrawText("Reference signs will be detected and compared with text", 50, 80);
        dc.DrawText("Green boxes = found in text, Red boxes = not found in text", 50, 110);
        return;
    }

    // Draw all annotated pages vertically
    int currentY = m_margin;

    for (const auto& image : m_annotatedImages) {
        if (!image.IsOk()) continue;

        wxBitmap bitmap(image);
        dc.DrawBitmap(bitmap, m_margin, currentY);
        currentY += image.GetHeight() + m_pageSpacing;
    }

    // Update virtual size for scrolling
    if (!m_annotatedImages.empty()) {
        int maxWidth = 0;
        for (const auto& img : m_annotatedImages) {
            if (img.GetWidth() > maxWidth) {
                maxWidth = img.GetWidth();
            }
        }
        SetVirtualSize(maxWidth + 2 * m_margin, currentY);
    }
}

void PDFPanel::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}
