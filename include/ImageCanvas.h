#pragma once
#include <wx/scrolwin.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <memory>

#ifdef HAVE_OCR_SUPPORT
#include "IOcrEngine.h"
#include <vector>
#endif

/**
 * @brief Custom canvas widget for displaying images with zoom and pan support
 *
 * Features:
 * - Zoom with Ctrl+MouseWheel (cursor-centered)
 * - Pan with mouse drag
 * - Double-buffered rendering for smooth display
 * - Bitmap caching for performance
 */
class ImageCanvas : public wxScrolledCanvas {
public:
    explicit ImageCanvas(wxWindow* parent);

    // Image management
    void setImage(const wxImage& image);
    void clearImage();
    bool hasImage() const;

    // Zoom controls
    void setZoom(double zoomFactor);
    double getZoom() const;
    void zoomIn();   // Zoom by 1.25x
    void zoomOut();  // Zoom by 0.8x
    void zoomToFit();
    void zoomToActual(); // 100%

    // Zoom limits
    static constexpr double MIN_ZOOM = 0.1;   // 10%
    static constexpr double MAX_ZOOM = 10.0;  // 1000%
    static constexpr double ZOOM_STEP = 1.25; // 25% per scroll

#ifdef HAVE_OCR_SUPPORT
    // OCR overlay
    void setOcrResults(const std::vector<DetectedReference>& refs);
    void clearOcrResults();
    void highlightBoundingBox(int x, int y, int width, int height);
#endif

private:
    // Event handlers
    void onPaint(wxPaintEvent& event);
    void onMouseWheel(wxMouseEvent& event);
    void onMouseLeftDown(wxMouseEvent& event);
    void onMouseLeftUp(wxMouseEvent& event);
    void onMouseMotion(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void onSize(wxSizeEvent& event);

    // Helper methods
    void updateVirtualSize();
    void updateBitmapCache();
    wxPoint clientToImage(const wxPoint& clientPos) const;
    wxPoint imageToClient(const wxPoint& imagePos) const;
    void centerOnImagePoint(const wxPoint& imagePoint, const wxPoint& clientPoint);

    // Image data
    wxImage m_originalImage;
    wxBitmap m_cachedBitmap;  // Pre-scaled for current zoom
    bool m_bitmapCacheDirty{true};

    // Zoom state
    double m_zoomFactor{1.0};

    // Pan state
    bool m_isPanning{false};
    wxPoint m_panStartMouse;
    wxPoint m_panStartScroll;

#ifdef HAVE_OCR_SUPPORT
    // OCR overlay data
    std::vector<DetectedReference> m_ocrReferences;
    int m_highlightedIdx{-1};
#endif
};
