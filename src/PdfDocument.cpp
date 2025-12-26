#include "PdfDocument.h"

#ifdef HAVE_PDF_SUPPORT

#include <mupdf/fitz.h>
#include <stdexcept>
#include <cstring>

PdfDocument::PdfDocument() {
    // Create muPDF context with default allocators
    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!m_ctx) {
        throw std::runtime_error("Failed to create muPDF context");
    }

    // Register document handlers
    fz_try(m_ctx) {
        fz_register_document_handlers(m_ctx);
    }
    fz_catch(m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
        throw std::runtime_error("Failed to register muPDF document handlers");
    }
}

PdfDocument::~PdfDocument() {
    cleanup();
}

PdfDocument::PdfDocument(PdfDocument&& other) noexcept
    : m_ctx(other.m_ctx),
      m_doc(other.m_doc),
      m_filePath(std::move(other.m_filePath)),
      m_pageCount(other.m_pageCount) {
    other.m_ctx = nullptr;
    other.m_doc = nullptr;
    other.m_pageCount = 0;
}

PdfDocument& PdfDocument::operator=(PdfDocument&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_ctx = other.m_ctx;
        m_doc = other.m_doc;
        m_filePath = std::move(other.m_filePath);
        m_pageCount = other.m_pageCount;
        other.m_ctx = nullptr;
        other.m_doc = nullptr;
        other.m_pageCount = 0;
    }
    return *this;
}

void PdfDocument::cleanup() {
    if (m_doc) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
    }
    if (m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
    }
    m_pageCount = 0;
    m_filePath.clear();
}

bool PdfDocument::loadFromFile(const std::string& path) {
    if (!m_ctx) {
        return false;
    }

    // Close any existing document
    if (m_doc) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
        m_pageCount = 0;
    }

    // Try to open the PDF
    fz_try(m_ctx) {
        m_doc = fz_open_document(m_ctx, path.c_str());
        m_pageCount = fz_count_pages(m_ctx, m_doc);
        m_filePath = path;
    }
    fz_catch(m_ctx) {
        m_doc = nullptr;
        m_pageCount = 0;
        m_filePath.clear();
        return false;
    }

    return true;
}

wxImage PdfDocument::renderPage(size_t pageIndex, float dpi) {
    if (!m_doc || pageIndex >= static_cast<size_t>(m_pageCount)) {
        return wxImage();  // Return invalid image
    }

    fz_page* page = nullptr;
    fz_pixmap* pix = nullptr;
    wxImage result;

    fz_try(m_ctx) {
        // Load the page
        page = fz_load_page(m_ctx, m_doc, static_cast<int>(pageIndex));

        // Calculate transform matrix for desired DPI
        // Default PDF resolution is 72 DPI
        float scale = dpi / 72.0f;
        fz_matrix transform = fz_scale(scale, scale);

        // Get page bounds
        fz_rect bounds = fz_bound_page(m_ctx, page);
        bounds = fz_transform_rect(bounds, transform);

        // Create a pixmap to render into (RGB, no alpha)
        fz_irect bbox = fz_round_rect(bounds);
        pix = fz_new_pixmap_with_bbox(m_ctx, fz_device_rgb(m_ctx), bbox, nullptr, 0);

        // Clear pixmap to white background
        fz_clear_pixmap_with_value(m_ctx, pix, 0xff);

        // Render the page
        fz_device* dev = fz_new_draw_device(m_ctx, transform, pix);
        fz_run_page(m_ctx, page, dev, fz_identity, nullptr);
        fz_close_device(m_ctx, dev);
        fz_drop_device(m_ctx, dev);

        // Convert pixmap to wxImage
        int width = fz_pixmap_width(m_ctx, pix);
        int height = fz_pixmap_height(m_ctx, pix);
        int n = fz_pixmap_components(m_ctx, pix);  // Should be 3 for RGB
        unsigned char* samples = fz_pixmap_samples(m_ctx, pix);

        if (n == 3) {
            // RGB format - direct copy
            result = wxImage(width, height, false);
            unsigned char* data = result.GetData();
            std::memcpy(data, samples, width * height * 3);
        } else if (n == 4) {
            // RGBA format - strip alpha channel
            result = wxImage(width, height, false);
            unsigned char* data = result.GetData();
            for (int i = 0; i < width * height; ++i) {
                data[i * 3 + 0] = samples[i * 4 + 0];  // R
                data[i * 3 + 1] = samples[i * 4 + 1];  // G
                data[i * 3 + 2] = samples[i * 4 + 2];  // B
                // Skip alpha channel (samples[i * 4 + 3])
            }
        }

        // Clean up
        fz_drop_pixmap(m_ctx, pix);
        fz_drop_page(m_ctx, page);
    }
    fz_catch(m_ctx) {
        // Clean up on error
        if (pix) {
            fz_drop_pixmap(m_ctx, pix);
        }
        if (page) {
            fz_drop_page(m_ctx, page);
        }
        return wxImage();  // Return invalid image on error
    }

    return result;
}

size_t PdfDocument::getPageCount() const {
    return static_cast<size_t>(m_pageCount);
}

bool PdfDocument::isLoaded() const {
    return m_doc != nullptr;
}

void PdfDocument::close() {
    if (m_doc) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
        m_pageCount = 0;
        m_filePath.clear();
    }
}

#endif // HAVE_PDF_SUPPORT
