#include "PDFPanel.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <mupdf/fitz.h>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <algorithm>
#include <regex>
#include <iostream>

wxBEGIN_EVENT_TABLE(PDFPanel, wxScrolledWindow)
    EVT_PAINT(PDFPanel::onPaint)
    EVT_SIZE(PDFPanel::onSize)
wxEND_EVENT_TABLE()

// PDFDropTarget implementation
bool PDFDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) {
    if (filenames.GetCount() == 0) {
        return false;
    }

    wxString filename = filenames[0];

    // Check if it's a PDF file
    wxFileName fn(filename);
    if (fn.GetExt().Lower() != "pdf") {
        wxMessageBox("Please drop a PDF file.", "Invalid File", wxOK | wxICON_WARNING);
        return false;
    }

    return m_panel->loadPDF(filename);
}

// PDFPanel implementation
PDFPanel::PDFPanel(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxVSCROLL | wxHSCROLL | wxFULL_REPAINT_ON_RESIZE),
      m_fzContext(nullptr),
      m_fzDocument(nullptr),
      m_tesseract(nullptr),
      m_pageCount(0),
      m_spacing(20) {

    SetBackgroundColour(*wxWHITE);
    SetScrollRate(10, 10);

    // Set up drag-and-drop
    SetDropTarget(new PDFDropTarget(this));

    // Initialize MuPDF
    m_fzContext = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!m_fzContext) {
        wxLogError("Failed to initialize MuPDF context");
        return;
    }
    fz_register_document_handlers(m_fzContext);

    // Initialize Tesseract
    m_tesseract = std::make_unique<tesseract::TessBaseAPI>();
    if (m_tesseract->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) != 0) {
        wxLogError("Failed to initialize Tesseract OCR");
        m_tesseract.reset();
        return;
    }

    // Configure Tesseract for number detection
    m_tesseract->SetVariable("tessedit_char_whitelist", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'");
    m_tesseract->SetPageSegMode(tesseract::PSM_AUTO);
}

PDFPanel::~PDFPanel() {
    clear();

    if (m_fzDocument) {
        fz_drop_document(m_fzContext, m_fzDocument);
        m_fzDocument = nullptr;
    }

    if (m_fzContext) {
        fz_drop_context(m_fzContext);
        m_fzContext = nullptr;
    }

    m_tesseract.reset();
}

void PDFPanel::clear() {
    if (m_fzDocument) {
        fz_drop_document(m_fzContext, m_fzDocument);
        m_fzDocument = nullptr;
    }

    m_renderedPages.clear();
    m_originalPages.clear();
    m_detectedReferences.clear();
    m_pageCount = 0;
    m_pdfFilename.clear();

    Refresh();
}

bool PDFPanel::loadPDF(const wxString& filename) {
    clear();

    if (!m_fzContext) {
        wxMessageBox("MuPDF context not initialized", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    // Open the PDF
    fz_try(m_fzContext) {
        m_fzDocument = fz_open_document(m_fzContext, filename.mb_str());
    }
    fz_catch(m_fzContext) {
        wxMessageBox(wxString::Format("Failed to open PDF: %s", filename),
                    "Error", wxOK | wxICON_ERROR);
        return false;
    }

    m_pdfFilename = filename;
    m_pageCount = fz_count_pages(m_fzContext, m_fzDocument);

    // Render all pages
    renderPDFPages();

    // Perform OCR on all pages
    performOCR();

    // Annotate pages with colored rectangles
    annotatePages();

    // Update scroll range
    if (!m_renderedPages.empty()) {
        int totalHeight = 0;
        int maxWidth = 0;

        for (const auto& bitmap : m_renderedPages) {
            totalHeight += bitmap.GetHeight() + m_spacing;
            maxWidth = std::max(maxWidth, bitmap.GetWidth());
        }

        SetVirtualSize(maxWidth, totalHeight);
    }

    Refresh();
    return true;
}

void PDFPanel::renderPDFPages() {
    if (!m_fzDocument) return;

    m_originalPages.clear();
    m_originalPages.reserve(m_pageCount);

    for (int i = 0; i < m_pageCount; ++i) {
        fz_page* page = nullptr;
        fz_pixmap* pix = nullptr;

        fz_try(m_fzContext) {
            // Load page
            page = fz_load_page(m_fzContext, m_fzDocument, i);

            // Get page dimensions and create transform
            fz_rect bounds = fz_bound_page(m_fzContext, page);
            fz_matrix transform = fz_scale(2.0f, 2.0f);  // 2x scaling for better quality

            // Render page to pixmap
            pix = fz_new_pixmap_from_page(m_fzContext, page, transform,
                                         fz_device_rgb(m_fzContext), 0);

            // Convert pixmap to wxImage
            int width = fz_pixmap_width(m_fzContext, pix);
            int height = fz_pixmap_height(m_fzContext, pix);
            int stride = fz_pixmap_stride(m_fzContext, pix);
            unsigned char* samples = fz_pixmap_samples(m_fzContext, pix);

            wxImage image(width, height);
            unsigned char* imageData = image.GetData();

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int pixIdx = y * stride + x * 3;
                    int imgIdx = (y * width + x) * 3;

                    imageData[imgIdx] = samples[pixIdx];       // R
                    imageData[imgIdx + 1] = samples[pixIdx + 1]; // G
                    imageData[imgIdx + 2] = samples[pixIdx + 2]; // B
                }
            }

            m_originalPages.push_back(image);
        }
        fz_always(m_fzContext) {
            if (pix) fz_drop_pixmap(m_fzContext, pix);
            if (page) fz_drop_page(m_fzContext, page);
        }
        fz_catch(m_fzContext) {
            wxLogError("Failed to render page %d", i);
        }
    }
}

void PDFPanel::performOCR() {
    if (!m_tesseract || m_originalPages.empty()) return;

    m_detectedReferences.clear();

    for (int pageNum = 0; pageNum < static_cast<int>(m_originalPages.size()); ++pageNum) {
        ocrPage(pageNum, m_originalPages[pageNum]);
    }
}

void PDFPanel::ocrPage(int pageNum, const wxImage& pageImage) {
    if (!m_tesseract) return;

    // Convert wxImage to Leptonica Pix
    int width = pageImage.GetWidth();
    int height = pageImage.GetHeight();
    unsigned char* data = pageImage.GetData();

    PIX* pix = pixCreate(width, height, 32);
    if (!pix) {
        wxLogError("Failed to create Leptonica image for page %d", pageNum);
        return;
    }

    // Copy image data
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            unsigned char r = data[idx];
            unsigned char g = data[idx + 1];
            unsigned char b = data[idx + 2];

            l_uint32 pixel = (r << 24) | (g << 16) | (b << 8) | 0xFF;
            pixSetPixel(pix, x, y, pixel);
        }
    }

    // Run OCR
    m_tesseract->SetImage(pix);
    m_tesseract->Recognize(nullptr);

    // Get results with bounding boxes
    tesseract::ResultIterator* iter = m_tesseract->GetIterator();
    if (iter) {
        do {
            const char* word = iter->GetUTF8Text(tesseract::RIL_WORD);
            if (word) {
                std::string wordStr(word);
                std::wstring wword(wordStr.begin(), wordStr.end());

                // Check if this looks like a reference number
                if (isReferenceNumber(wword)) {
                    int x1, y1, x2, y2;
                    iter->BoundingBox(tesseract::RIL_WORD, &x1, &y1, &x2, &y2);

                    DetectedReference ref;
                    ref.number = wword;
                    ref.page = pageNum;
                    ref.x = x1;
                    ref.y = y1;
                    ref.width = x2 - x1;
                    ref.height = y2 - y1;
                    ref.existsInText = (m_textReferences.count(wword) > 0);

                    m_detectedReferences.push_back(ref);
                }

                delete[] word;
            }
        } while (iter->Next(tesseract::RIL_WORD));

        delete iter;
    }

    pixDestroy(&pix);
}

bool PDFPanel::isReferenceNumber(const std::wstring& text) const {
    // Match patterns like: 10, 10a, 10', etc.
    std::wregex pattern(L"^\\d+[a-zA-Z']*$");
    return std::regex_match(text, pattern);
}

void PDFPanel::annotatePages() {
    m_renderedPages.clear();

    for (int pageNum = 0; pageNum < static_cast<int>(m_originalPages.size()); ++pageNum) {
        // Make a copy of the original page
        wxImage annotatedImage = m_originalPages[pageNum].Copy();

        // Draw rectangles around detected references
        for (const auto& ref : m_detectedReferences) {
            if (ref.page == pageNum) {
                wxColour color = ref.existsInText ? *wxGREEN : *wxRED;
                drawRectangle(annotatedImage, ref.x, ref.y, ref.width, ref.height, color, 3);
            }
        }

        // Convert to bitmap for display
        m_renderedPages.push_back(wxBitmap(annotatedImage));
    }
}

void PDFPanel::drawRectangle(wxImage& image, int x, int y, int w, int h,
                             const wxColour& color, int thickness) {
    unsigned char r = color.Red();
    unsigned char g = color.Green();
    unsigned char b = color.Blue();

    int width = image.GetWidth();
    int height = image.GetHeight();
    unsigned char* data = image.GetData();

    // Draw rectangle outline
    for (int t = 0; t < thickness; ++t) {
        // Top and bottom edges
        for (int i = x; i < x + w; ++i) {
            if (i >= 0 && i < width) {
                // Top edge
                int yTop = y + t;
                if (yTop >= 0 && yTop < height) {
                    int idx = (yTop * width + i) * 3;
                    data[idx] = r;
                    data[idx + 1] = g;
                    data[idx + 2] = b;
                }

                // Bottom edge
                int yBot = y + h - t - 1;
                if (yBot >= 0 && yBot < height) {
                    int idx = (yBot * width + i) * 3;
                    data[idx] = r;
                    data[idx + 1] = g;
                    data[idx + 2] = b;
                }
            }
        }

        // Left and right edges
        for (int j = y; j < y + h; ++j) {
            if (j >= 0 && j < height) {
                // Left edge
                int xLeft = x + t;
                if (xLeft >= 0 && xLeft < width) {
                    int idx = (j * width + xLeft) * 3;
                    data[idx] = r;
                    data[idx + 1] = g;
                    data[idx + 2] = b;
                }

                // Right edge
                int xRight = x + w - t - 1;
                if (xRight >= 0 && xRight < width) {
                    int idx = (j * width + xRight) * 3;
                    data[idx] = r;
                    data[idx + 1] = g;
                    data[idx + 2] = b;
                }
            }
        }
    }
}

void PDFPanel::setTextReferences(const std::unordered_set<std::wstring>& refs) {
    m_textReferences = refs;

    // Update existsInText flag for all detected references
    for (auto& ref : m_detectedReferences) {
        ref.existsInText = (m_textReferences.count(ref.number) > 0);
    }

    // Re-annotate pages with updated colors
    if (!m_originalPages.empty()) {
        annotatePages();
        Refresh();
    }
}

std::vector<std::wstring> PDFPanel::getMissingInText() const {
    std::vector<std::wstring> missing;
    std::unordered_set<std::wstring> seen;

    for (const auto& ref : m_detectedReferences) {
        if (!ref.existsInText && seen.find(ref.number) == seen.end()) {
            missing.push_back(ref.number);
            seen.insert(ref.number);
        }
    }

    // Sort numerically
    std::sort(missing.begin(), missing.end(), [](const std::wstring& a, const std::wstring& b) {
        // Extract numeric part
        int numA = std::stoi(a);
        int numB = std::stoi(b);
        if (numA != numB) return numA < numB;
        return a < b;  // If numbers are equal, compare full string (for 10a vs 10b)
    });

    return missing;
}

std::vector<std::wstring> PDFPanel::getMissingInPDF() const {
    // Collect all PDF reference numbers
    std::unordered_set<std::wstring> pdfRefs;
    for (const auto& ref : m_detectedReferences) {
        pdfRefs.insert(ref.number);
    }

    // Find text references not in PDF
    std::vector<std::wstring> missing;
    for (const auto& textRef : m_textReferences) {
        if (pdfRefs.find(textRef) == pdfRefs.end()) {
            missing.push_back(textRef);
        }
    }

    // Sort numerically
    std::sort(missing.begin(), missing.end(), [](const std::wstring& a, const std::wstring& b) {
        int numA = std::stoi(a);
        int numB = std::stoi(b);
        if (numA != numB) return numA < numB;
        return a < b;
    });

    return missing;
}

void PDFPanel::onPaint(wxPaintEvent& event) {
    wxBufferedPaintDC dc(this);
    DoPrepareDC(dc);

    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    if (m_renderedPages.empty()) {
        // Draw instructions
        wxString instructions = "Drag and drop a PDF file here\nto analyze patent figures";
        dc.SetTextForeground(*wxLIGHT_GREY);

        wxSize textSize = dc.GetTextExtent(instructions);
        wxSize clientSize = GetClientSize();

        int x = (clientSize.GetWidth() - textSize.GetWidth()) / 2;
        int y = (clientSize.GetHeight() - textSize.GetHeight()) / 2;

        dc.DrawText(instructions, x, y);
        return;
    }

    // Draw all pages
    int yOffset = m_spacing;
    for (const auto& bitmap : m_renderedPages) {
        int x = (GetVirtualSize().GetWidth() - bitmap.GetWidth()) / 2;
        dc.DrawBitmap(bitmap, x, yOffset);
        yOffset += bitmap.GetHeight() + m_spacing;
    }
}

void PDFPanel::onSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}
