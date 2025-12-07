#include "PDFLoader.h"

#ifdef HAVE_MUPDF

extern "C" {
#include <mupdf/fitz.h>
}

#include <wx/image.h>

bool PDFLoader::loadPDF(const wxString& filePath, int dpi,
                        std::vector<wxBitmap>& outBitmaps, wxString& errorMsg)
{
    // Convert wxString to UTF-8 char* for muPDF
    std::string utf8Path = filePath.ToUTF8().data();

    fz_context* ctx = nullptr;
    fz_document* doc = nullptr;

    // Initialize muPDF context
    ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) {
        errorMsg = "Failed to initialize muPDF context";
        return false;
    }

    // Use fz_try/fz_catch for exception handling
    fz_try(ctx) {
        // Register document handlers (PDF, XPS, etc.)
        fz_register_document_handlers(ctx);

        // Open document
        doc = fz_open_document(ctx, utf8Path.c_str());

        // Check if document is encrypted
        if (fz_needs_password(ctx, doc)) {
            errorMsg = "PDF is password-protected. Please decrypt the PDF first.";
            fz_drop_document(ctx, doc);
            fz_drop_context(ctx);
            return false;
        }

        // Count pages
        int pageCount = fz_count_pages(ctx, doc);

        if (pageCount == 0) {
            errorMsg = "PDF contains no pages";
            fz_drop_document(ctx, doc);
            fz_drop_context(ctx);
            return false;
        }

        // Calculate zoom factor (muPDF default is 72 DPI)
        float zoom = static_cast<float>(dpi) / 72.0f;
        fz_matrix ctm = fz_scale(zoom, zoom);

        // Render each page
        for (int i = 0; i < pageCount; ++i) {
            fz_pixmap* pix = nullptr;

            fz_try(ctx) {
                // Render page to RGB pixmap (no alpha for smaller size)
                pix = fz_new_pixmap_from_page_number(
                    ctx, doc, i, ctm, fz_device_rgb(ctx), 0
                );

                // Convert to wxImage then wxBitmap
                wxImage img = pixmapToWxImage(pix);
                if (img.IsOk()) {
                    outBitmaps.push_back(wxBitmap(img));
                }
            }
            fz_always(ctx) {
                fz_drop_pixmap(ctx, pix);
            }
            fz_catch(ctx) {
                // Continue with next page on error
                fz_report_error(ctx);
            }
        }
    }
    fz_catch(ctx) {
        fz_report_error(ctx);
        errorMsg = wxString::Format("Failed to load PDF: %s",
                                     fz_caught_message(ctx));
        fz_drop_document(ctx, doc);
        fz_drop_context(ctx);
        return false;
    }

    // Cleanup
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);

    return !outBitmaps.empty();
}

wxImage PDFLoader::pixmapToWxImage(fz_pixmap* pix)
{
    if (!pix) return wxImage();

    int width = pix->w;
    int height = pix->h;
    int components = pix->n; // 3 for RGB, 4 for RGBA

    // Create wxImage (wxImage expects RGB)
    wxImage img(width, height, false);

    unsigned char* imgData = img.GetData();
    unsigned char* pixData = pix->samples;
    int stride = pix->stride;

    // Copy pixel data row by row
    for (int y = 0; y < height; ++y) {
        unsigned char* srcRow = pixData + (y * stride);
        unsigned char* dstRow = imgData + (y * width * 3);

        for (int x = 0; x < width; ++x) {
            // muPDF RGB: samples[0]=R, [1]=G, [2]=B, [3]=A (if present)
            dstRow[0] = srcRow[0]; // R
            dstRow[1] = srcRow[1]; // G
            dstRow[2] = srcRow[2]; // B

            srcRow += components;
            dstRow += 3;
        }
    }

    return img;
}

#endif // HAVE_MUPDF
