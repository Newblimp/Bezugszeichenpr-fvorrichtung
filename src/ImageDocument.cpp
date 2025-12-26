#include "ImageDocument.h"
#include <wx/image.h>

bool ImageDocument::loadFromFile(const std::string& path) {
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
