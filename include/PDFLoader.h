#pragma once

#ifdef HAVE_MUPDF

#include <wx/wx.h>
#include <wx/bitmap.h>
#include <vector>
#include <string>

/**
 * @brief Helper class for loading PDF files using muPDF
 *
 * This class handles all muPDF interaction, converting PDF pages
 * to wxBitmap objects that can be displayed by the existing image viewer.
 */
class PDFLoader {
public:
    /**
     * @brief Load all pages from a PDF file
     * @param filePath Path to PDF file (UTF-8 encoded)
     * @param dpi Rendering resolution (300 recommended for technical drawings)
     * @param outBitmaps Output vector to receive wxBitmap for each page
     * @param errorMsg Output error message if loading fails
     * @return true on success, false on failure
     */
    static bool loadPDF(
        const wxString& filePath,
        int dpi,
        std::vector<wxBitmap>& outBitmaps,
        wxString& errorMsg
    );

private:
    /**
     * @brief Convert muPDF fz_pixmap to wxImage
     * @param pix muPDF pixmap (must be RGB or RGBA)
     * @return wxImage containing the pixel data
     */
    static wxImage pixmapToWxImage(struct fz_pixmap* pix);
};

#endif // HAVE_MUPDF
