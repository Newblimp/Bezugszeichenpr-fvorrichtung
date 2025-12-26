#include "ImageDocument.h"
#include <wx/image.h>
#include <algorithm>

#ifdef HAVE_PDF_SUPPORT
#include "PdfDocument.h"
#endif

namespace {
    // Helper function to check if a file is a PDF based on extension
    bool isPdfFile(const std::string& path) {
        if (path.length() < 4) {
            return false;
        }
        std::string ext = path.substr(path.length() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".pdf";
    }
}

bool ImageDocument::loadFromFile(const std::string& path) {
#ifdef HAVE_PDF_SUPPORT
    // Check if this is a PDF file
    if (isPdfFile(path)) {
        PdfDocument pdfDoc;
        if (!pdfDoc.loadFromFile(path)) {
            return false;
        }

        clear();
        size_t pageCount = pdfDoc.getPageCount();
        for (size_t i = 0; i < pageCount; ++i) {
            wxImage image = pdfDoc.renderPage(i);
            if (image.IsOk()) {
                addPage(image, path);
            }
        }
        return hasPages();
    }
#endif

    // Handle as regular image file
    wxImage image;
    if (!image.LoadFile(wxString::FromUTF8(path.c_str()))) {
        return false;
    }

    clear();
    addPage(image, path);
    return true;
}

bool ImageDocument::loadFromFiles(const std::vector<std::string>& paths) {
    if (paths.empty()) {
        return false;
    }

    clear();
    bool anySuccess = false;

    for (const auto& path : paths) {
#ifdef HAVE_PDF_SUPPORT
        // Check if this is a PDF file
        if (isPdfFile(path)) {
            PdfDocument pdfDoc;
            if (pdfDoc.loadFromFile(path)) {
                size_t pageCount = pdfDoc.getPageCount();
                for (size_t i = 0; i < pageCount; ++i) {
                    wxImage image = pdfDoc.renderPage(i);
                    if (image.IsOk()) {
                        addPage(image, path);
                        anySuccess = true;
                    }
                }
            }
            continue;
        }
#endif

        // Handle as regular image file
        wxImage image;
        if (image.LoadFile(wxString::FromUTF8(path.c_str()))) {
            addPage(image, path);
            anySuccess = true;
        }
    }

    return anySuccess;
}

void ImageDocument::addPage(const wxImage& image, const std::string& sourcePath) {
    PageInfo page;
    page.image = image.Copy();  // Deep copy to avoid shared data
    page.sourcePath = sourcePath;
    m_pages.push_back(std::move(page));
}

void ImageDocument::clear() {
    m_pages.clear();
}

size_t ImageDocument::getPageCount() const {
    return m_pages.size();
}

const wxImage& ImageDocument::getPage(size_t index) const {
    return m_pages.at(index).image;
}

std::string ImageDocument::getPagePath(size_t index) const {
    return m_pages.at(index).sourcePath;
}

bool ImageDocument::hasPages() const {
    return !m_pages.empty();
}

bool ImageDocument::isValidPageIndex(size_t index) const {
    return index < m_pages.size();
}
