#pragma once

#ifdef HAVE_PDF_SUPPORT

#include <wx/image.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations for muPDF types to avoid including muPDF headers in public interface
struct fz_context;
struct fz_document;

/**
 * @brief PDF document loader and renderer using muPDF
 *
 * Features:
 * - Loads PDF files and renders pages to wxImage
 * - Multi-page support with configurable DPI
 * - RAII-based resource management for muPDF objects
 * - Thread-safe (each instance has its own fz_context)
 *
 * Implementation notes:
 * - Default DPI is 150 (good balance between quality and performance)
 * - Pages are rendered on-demand to save memory
 * - muPDF context is locked during rendering operations
 */
class PdfDocument {
public:
    PdfDocument();
    ~PdfDocument();

    // Non-copyable due to muPDF resource management
    PdfDocument(const PdfDocument&) = delete;
    PdfDocument& operator=(const PdfDocument&) = delete;

    // Movable
    PdfDocument(PdfDocument&& other) noexcept;
    PdfDocument& operator=(PdfDocument&& other) noexcept;

    /**
     * @brief Load a PDF file
     * @param path Path to PDF file
     * @return true if successful, false on error
     */
    bool loadFromFile(const std::string& path);

    /**
     * @brief Render a specific page to wxImage
     * @param pageIndex Zero-based page index
     * @param dpi Resolution for rendering (default: 150)
     * @return Rendered page as wxImage, or invalid wxImage on error
     */
    wxImage renderPage(size_t pageIndex, float dpi = 150.0f);

    /**
     * @brief Get the number of pages in the loaded PDF
     * @return Page count, or 0 if no document is loaded
     */
    size_t getPageCount() const;

    /**
     * @brief Check if a PDF document is currently loaded
     * @return true if document is loaded
     */
    bool isLoaded() const;

    /**
     * @brief Close the current document and free resources
     */
    void close();

    /**
     * @brief Get the path of the currently loaded PDF
     * @return File path, or empty string if no document is loaded
     */
    std::string getFilePath() const { return m_filePath; }

private:
    void cleanup();

    fz_context* m_ctx{nullptr};
    fz_document* m_doc{nullptr};
    std::string m_filePath;
    int m_pageCount{0};
};

#endif // HAVE_PDF_SUPPORT
