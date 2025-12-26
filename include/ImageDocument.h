#pragma once
#include <wx/image.h>
#include <vector>
#include <string>
#include <memory>

/**
 * @brief Manages a collection of image pages for multi-page documents
 *
 * Supports loading images from individual files or multiple files.
 * Each page stores the original image and its source path.
 */
class ImageDocument {
public:
    ImageDocument() = default;

    // Loading methods
    bool loadFromFile(const std::string& path);
    bool loadFromFiles(const std::vector<std::string>& paths);
    void addPage(const wxImage& image, const std::string& sourcePath);
    void clear();

    // Page access
    size_t getPageCount() const;
    const wxImage& getPage(size_t index) const;
    std::string getPagePath(size_t index) const;
    bool hasPages() const;

    // Validation
    bool isValidPageIndex(size_t index) const;

private:
    struct PageInfo {
        wxImage image;
        std::string sourcePath;
    };
    std::vector<PageInfo> m_pages;
};
