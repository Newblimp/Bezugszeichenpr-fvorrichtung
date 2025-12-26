#include "ImageCanvas.h"
#include <wx/dcbuffer.h>
#include <algorithm>

ImageCanvas::ImageCanvas(wxWindow* parent)
    : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxHSCROLL | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE) {

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);

    // Set scroll rate (pixels per scroll unit)
    SetScrollRate(10, 10);

    // Bind events
    Bind(wxEVT_PAINT, &ImageCanvas::onPaint, this);
    Bind(wxEVT_MOUSEWHEEL, &ImageCanvas::onMouseWheel, this);
    Bind(wxEVT_LEFT_DOWN, &ImageCanvas::onMouseLeftDown, this);
    Bind(wxEVT_LEFT_UP, &ImageCanvas::onMouseLeftUp, this);
    Bind(wxEVT_MOTION, &ImageCanvas::onMouseMotion, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST, &ImageCanvas::onMouseCaptureLost, this);
    Bind(wxEVT_SIZE, &ImageCanvas::onSize, this);
}

void ImageCanvas::setImage(const wxImage& image) {
    m_originalImage = image.Copy();
    m_bitmapCacheDirty = true;
    m_zoomFactor = 1.0;
    updateVirtualSize();
    Refresh();
}

void ImageCanvas::clearImage() {
    m_originalImage = wxImage();
    m_cachedBitmap = wxBitmap();
    m_bitmapCacheDirty = true;
    SetVirtualSize(0, 0);
    Refresh();
}

bool ImageCanvas::hasImage() const {
    return m_originalImage.IsOk();
}

void ImageCanvas::setZoom(double zoomFactor) {
    zoomFactor = std::clamp(zoomFactor, MIN_ZOOM, MAX_ZOOM);
    if (zoomFactor != m_zoomFactor) {
        m_zoomFactor = zoomFactor;
        m_bitmapCacheDirty = true;
        updateVirtualSize();
        Refresh();
    }
}

double ImageCanvas::getZoom() const {
    return m_zoomFactor;
}

void ImageCanvas::zoomIn() {
    setZoom(m_zoomFactor * ZOOM_STEP);
}

void ImageCanvas::zoomOut() {
    setZoom(m_zoomFactor / ZOOM_STEP);
}

void ImageCanvas::zoomToFit() {
    if (!hasImage()) return;

    wxSize clientSize = GetClientSize();
    int imgWidth = m_originalImage.GetWidth();
    int imgHeight = m_originalImage.GetHeight();

    if (imgWidth == 0 || imgHeight == 0) return;

    double zoomX = static_cast<double>(clientSize.GetWidth()) / imgWidth;
    double zoomY = static_cast<double>(clientSize.GetHeight()) / imgHeight;

    setZoom(std::min(zoomX, zoomY));

    // Center the image
    Scroll(0, 0);
}

void ImageCanvas::zoomToActual() {
    setZoom(1.0);
}

void ImageCanvas::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    DoPrepareDC(dc);

    // Clear background
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    if (!hasImage()) return;

    // Update cache if needed
    if (m_bitmapCacheDirty) {
        updateBitmapCache();
    }

    // Draw the image
    if (m_cachedBitmap.IsOk()) {
        dc.DrawBitmap(m_cachedBitmap, 0, 0, false);
    }
}

void ImageCanvas::onMouseWheel(wxMouseEvent& event) {
    if (!event.ControlDown() || !hasImage()) {
        event.Skip();
        return;
    }

    // Get mouse position in client coordinates
    wxPoint mouseClient = event.GetPosition();

    // Convert to image coordinates before zoom
    wxPoint mouseImage = clientToImage(mouseClient);

    // Calculate new zoom
    double oldZoom = m_zoomFactor;
    if (event.GetWheelRotation() > 0) {
        m_zoomFactor *= ZOOM_STEP;
    } else {
        m_zoomFactor /= ZOOM_STEP;
    }
    m_zoomFactor = std::clamp(m_zoomFactor, MIN_ZOOM, MAX_ZOOM);

    if (m_zoomFactor != oldZoom) {
        m_bitmapCacheDirty = true;
        updateVirtualSize();

        // Keep the image point under cursor in the same screen position
        centerOnImagePoint(mouseImage, mouseClient);

        Refresh();
    }
}

void ImageCanvas::onMouseLeftDown(wxMouseEvent& event) {
    if (hasImage()) {
        m_isPanning = true;
        m_panStartMouse = event.GetPosition();

        int scrollX, scrollY;
        GetViewStart(&scrollX, &scrollY);
        m_panStartScroll = wxPoint(scrollX, scrollY);

        CaptureMouse();
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
}

void ImageCanvas::onMouseLeftUp(wxMouseEvent& event) {
    if (m_isPanning) {
        m_isPanning = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
}

void ImageCanvas::onMouseMotion(wxMouseEvent& event) {
    if (m_isPanning && event.LeftIsDown()) {
        wxPoint delta = m_panStartMouse - event.GetPosition();
        int ppuX, ppuY;
        GetScrollPixelsPerUnit(&ppuX, &ppuY);

        if (ppuX > 0 && ppuY > 0) {
            Scroll(m_panStartScroll.x + delta.x / ppuX,
                   m_panStartScroll.y + delta.y / ppuY);
        }
    }
}

void ImageCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    m_isPanning = false;
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

void ImageCanvas::onSize(wxSizeEvent& event) {
    event.Skip();
    Refresh();
}

void ImageCanvas::updateVirtualSize() {
    if (!hasImage()) {
        SetVirtualSize(0, 0);
        return;
    }

    int scaledWidth = static_cast<int>(m_originalImage.GetWidth() * m_zoomFactor);
    int scaledHeight = static_cast<int>(m_originalImage.GetHeight() * m_zoomFactor);

    SetVirtualSize(scaledWidth, scaledHeight);
}

void ImageCanvas::updateBitmapCache() {
    if (!hasImage()) return;

    int scaledWidth = static_cast<int>(m_originalImage.GetWidth() * m_zoomFactor);
    int scaledHeight = static_cast<int>(m_originalImage.GetHeight() * m_zoomFactor);

    if (scaledWidth <= 0 || scaledHeight <= 0) return;

    // Use high-quality scaling
    wxImage scaled = m_originalImage.Scale(scaledWidth, scaledHeight,
                                           wxIMAGE_QUALITY_BILINEAR);
    m_cachedBitmap = wxBitmap(scaled);
    m_bitmapCacheDirty = false;
}

wxPoint ImageCanvas::clientToImage(const wxPoint& clientPos) const {
    if (!hasImage() || m_zoomFactor <= 0) {
        return wxPoint(0, 0);
    }

    // Convert client coordinates to scrolled coordinates
    int scrollX, scrollY;
    GetViewStart(&scrollX, &scrollY);
    int ppuX, ppuY;
    GetScrollPixelsPerUnit(&ppuX, &ppuY);

    int scrolledX = clientPos.x + scrollX * ppuX;
    int scrolledY = clientPos.y + scrollY * ppuY;

    // Convert to image coordinates
    int imageX = static_cast<int>(scrolledX / m_zoomFactor);
    int imageY = static_cast<int>(scrolledY / m_zoomFactor);

    return wxPoint(imageX, imageY);
}

wxPoint ImageCanvas::imageToClient(const wxPoint& imagePos) const {
    if (!hasImage() || m_zoomFactor <= 0) {
        return wxPoint(0, 0);
    }

    // Convert image coordinates to scrolled coordinates
    int scrolledX = static_cast<int>(imagePos.x * m_zoomFactor);
    int scrolledY = static_cast<int>(imagePos.y * m_zoomFactor);

    // Convert to client coordinates
    int scrollX, scrollY;
    GetViewStart(&scrollX, &scrollY);
    int ppuX, ppuY;
    GetScrollPixelsPerUnit(&ppuX, &ppuY);

    int clientX = scrolledX - scrollX * ppuX;
    int clientY = scrolledY - scrollY * ppuY;

    return wxPoint(clientX, clientY);
}

void ImageCanvas::centerOnImagePoint(const wxPoint& imagePoint, const wxPoint& clientPoint) {
    if (!hasImage() || m_zoomFactor <= 0) return;

    // Calculate where the image point should be in scrolled coordinates
    int targetScrolledX = static_cast<int>(imagePoint.x * m_zoomFactor);
    int targetScrolledY = static_cast<int>(imagePoint.y * m_zoomFactor);

    // Calculate the scroll position needed to put the image point at the client position
    int ppuX, ppuY;
    GetScrollPixelsPerUnit(&ppuX, &ppuY);

    if (ppuX > 0 && ppuY > 0) {
        int scrollX = (targetScrolledX - clientPoint.x) / ppuX;
        int scrollY = (targetScrolledY - clientPoint.y) / ppuY;

        Scroll(scrollX, scrollY);
    }
}
