# Implementation Plan: Drawing Reference Sign Verification

## Overview

This document provides a detailed implementation plan for adding scanned drawing verification capabilities to the Bezugszeichenprüfvorrichtung application. The implementation is divided into three phases:

1. **Phase 1**: Image visualization with zoom, pan, and multi-page support
2. **Phase 2**: PDF import using muPDF
3. **Phase 3**: OCR integration (modular preparation)

---

## Phase 1: Image Visualization

### 1.1 Objective

Create a new window for displaying scanned patent drawings with:
- Open images via menu bar (File → Open Image) or Ctrl+O
- Zoom using scroll wheel (Ctrl+Scroll for consistency with standards)
- Pan by click-and-drag with mouse
- Multi-page support with page navigation
- Prepared structure for future reference sign tabs (like "BZ->features")

### 1.2 Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                       ImageViewerWindow                          │
│ ┌───────────────────────────────────────────────────────────────┐│
│ │ Toolbar: [◄ Page] [Page X/Y] [Page ►] [Zoom: 100%] [Fit]     ││
│ ├───────────────────────────────────────────────────────────────┤│
│ │                                                               ││
│ │                    ImageCanvas (wxScrolledCanvas)             ││
│ │                    - Handles zoom/pan                         ││
│ │                    - Double-buffered rendering                ││
│ │                                                               ││
│ ├───────────────────────────────────────────────────────────────┤│
│ │ Status Bar: [Image path] [Dimensions] [Zoom level]           ││
│ └───────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 New Files to Create

| File | Purpose |
|------|---------|
| `include/ImageViewerWindow.h` | Window class for image viewing |
| `src/ImageViewerWindow.cpp` | Implementation of image viewer window |
| `include/ImageCanvas.h` | Custom canvas widget for zoom/pan |
| `src/ImageCanvas.cpp` | Implementation of canvas with zoom/pan logic |
| `include/ImageDocument.h` | Data model for multi-page images |
| `src/ImageDocument.cpp` | Image loading and page management |
| `tests/test_image_viewer.cpp` | Unit tests for image viewing functionality |

### 1.4 Detailed Class Specifications

#### 1.4.1 ImageDocument Class

**Purpose**: Manages a collection of image pages (for multi-page documents or multiple files).

```cpp
// include/ImageDocument.h
#pragma once
#include <wx/image.h>
#include <vector>
#include <string>
#include <memory>

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
```

**Implementation Notes**:
- Use `wxImage::LoadFile()` with format auto-detection
- Support formats: PNG, JPEG, BMP, TIFF (common for scanned documents)
- Store original file paths for status bar display
- Initialize wxWidgets image handlers in constructor or ensure they're initialized

#### 1.4.2 ImageCanvas Class

**Purpose**: Custom widget handling image rendering with zoom and pan.

```cpp
// include/ImageCanvas.h
#pragma once
#include <wx/scrolwin.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <memory>

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
};
```

**Implementation Details**:

1. **Zoom with Mouse Wheel**:
   - Use Ctrl+Scroll for zoom (standard convention)
   - Zoom centered on mouse cursor position (not center of view)
   - Formula for cursor-centered zoom:
     ```cpp
     void ImageCanvas::onMouseWheel(wxMouseEvent& event) {
         if (!event.ControlDown() || !hasImage()) {
             event.Skip();
             return;
         }

         // Get mouse position in image coordinates before zoom
         wxPoint mouseClient = event.GetPosition();
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
     ```

2. **Pan with Mouse Drag**:
   ```cpp
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

   void ImageCanvas::onMouseMotion(wxMouseEvent& event) {
       if (m_isPanning && event.LeftIsDown()) {
           wxPoint delta = m_panStartMouse - event.GetPosition();
           int ppuX, ppuY;
           GetScrollPixelsPerUnit(&ppuX, &ppuY);
           Scroll(m_panStartScroll.x + delta.x / ppuX,
                  m_panStartScroll.y + delta.y / ppuY);
       }
   }
   ```

3. **Double-Buffered Painting**:
   ```cpp
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
       dc.DrawBitmap(m_cachedBitmap, 0, 0, false);
   }
   ```

4. **Bitmap Cache for Performance**:
   ```cpp
   void ImageCanvas::updateBitmapCache() {
       if (!hasImage()) return;

       int scaledWidth = static_cast<int>(m_originalImage.GetWidth() * m_zoomFactor);
       int scaledHeight = static_cast<int>(m_originalImage.GetHeight() * m_zoomFactor);

       // Use high-quality scaling
       wxImage scaled = m_originalImage.Scale(scaledWidth, scaledHeight,
                                               wxIMAGE_QUALITY_BILINEAR);
       m_cachedBitmap = wxBitmap(scaled);
       m_bitmapCacheDirty = false;
   }
   ```

#### 1.4.3 ImageViewerWindow Class

**Purpose**: Main window containing the canvas, toolbar, and page navigation.

```cpp
// include/ImageViewerWindow.h
#pragma once
#include <wx/frame.h>
#include <wx/toolbar.h>
#include <wx/statusbr.h>
#include <memory>
#include "ImageCanvas.h"
#include "ImageDocument.h"

class ImageViewerWindow : public wxFrame {
public:
    ImageViewerWindow(wxWindow* parent);

    // Document management
    bool openFile(const wxString& path);
    bool openFiles(const wxArrayString& paths);
    void closeDocument();

    // Page navigation
    void goToPage(size_t pageIndex);
    void nextPage();
    void previousPage();
    size_t getCurrentPage() const;
    size_t getPageCount() const;

private:
    void setupUI();
    void setupMenuBar();
    void setupToolbar();
    void setupBindings();
    void updateStatusBar();
    void updateToolbarState();
    void updatePageDisplay();

    // Menu/toolbar handlers
    void onOpenFile(wxCommandEvent& event);
    void onClose(wxCommandEvent& event);
    void onZoomIn(wxCommandEvent& event);
    void onZoomOut(wxCommandEvent& event);
    void onZoomFit(wxCommandEvent& event);
    void onZoomActual(wxCommandEvent& event);
    void onNextPage(wxCommandEvent& event);
    void onPreviousPage(wxCommandEvent& event);

    // Zoom update from canvas
    void onZoomChanged();

    // UI components
    std::unique_ptr<ImageCanvas> m_canvas;
    wxToolBar* m_toolbar;
    wxStatusBar* m_statusBar;
    wxStaticText* m_pageLabel;  // "Page X / Y" display

    // Document state
    ImageDocument m_document;
    size_t m_currentPage{0};

    // Toolbar button IDs
    enum {
        ID_PREV_PAGE = wxID_HIGHEST + 1,
        ID_NEXT_PAGE,
        ID_ZOOM_IN,
        ID_ZOOM_OUT,
        ID_ZOOM_FIT,
        ID_ZOOM_ACTUAL
    };
};
```

**Implementation Notes**:

1. **Menu Bar Structure**:
   ```cpp
   void ImageViewerWindow::setupMenuBar() {
       wxMenuBar* menuBar = new wxMenuBar();

       // File menu
       wxMenu* fileMenu = new wxMenu();
       fileMenu->Append(wxID_OPEN, "&Open Image...\tCtrl+O");
       fileMenu->AppendSeparator();
       fileMenu->Append(wxID_CLOSE, "&Close\tCtrl+W");
       menuBar->Append(fileMenu, "&File");

       // View menu
       wxMenu* viewMenu = new wxMenu();
       viewMenu->Append(ID_ZOOM_IN, "Zoom &In\tCtrl++");
       viewMenu->Append(ID_ZOOM_OUT, "Zoom &Out\tCtrl+-");
       viewMenu->Append(ID_ZOOM_FIT, "Zoom to &Fit\tCtrl+0");
       viewMenu->Append(ID_ZOOM_ACTUAL, "&Actual Size\tCtrl+1");
       menuBar->Append(viewMenu, "&View");

       // Navigate menu
       wxMenu* navMenu = new wxMenu();
       navMenu->Append(ID_NEXT_PAGE, "&Next Page\tPage Down");
       navMenu->Append(ID_PREV_PAGE, "&Previous Page\tPage Up");
       menuBar->Append(navMenu, "&Navigate");

       SetMenuBar(menuBar);
   }
   ```

2. **Toolbar Layout**:
   ```cpp
   void ImageViewerWindow::setupToolbar() {
       m_toolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT);

       m_toolbar->AddTool(ID_PREV_PAGE, "◄", wxNullBitmap, "Previous Page");
       m_pageLabel = new wxStaticText(m_toolbar, wxID_ANY, "Page 0/0");
       m_toolbar->AddControl(m_pageLabel);
       m_toolbar->AddTool(ID_NEXT_PAGE, "►", wxNullBitmap, "Next Page");

       m_toolbar->AddSeparator();

       m_toolbar->AddTool(ID_ZOOM_OUT, "-", wxNullBitmap, "Zoom Out");
       m_toolbar->AddTool(ID_ZOOM_IN, "+", wxNullBitmap, "Zoom In");
       m_toolbar->AddTool(ID_ZOOM_FIT, "Fit", wxNullBitmap, "Fit to Window");
       m_toolbar->AddTool(ID_ZOOM_ACTUAL, "100%", wxNullBitmap, "Actual Size");

       m_toolbar->Realize();
   }
   ```

3. **File Open Dialog**:
   ```cpp
   void ImageViewerWindow::onOpenFile(wxCommandEvent& event) {
       wxFileDialog openDialog(
           this,
           "Open Image File(s)",
           "", "",
           "Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif)|"
           "*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif|"
           "All files (*.*)|*.*",
           wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE
       );

       if (openDialog.ShowModal() == wxID_CANCEL) {
           return;
       }

       wxArrayString paths;
       openDialog.GetPaths(paths);
       openFiles(paths);
   }
   ```

4. **Status Bar Updates**:
   ```cpp
   void ImageViewerWindow::updateStatusBar() {
       if (!m_document.hasPages()) {
           m_statusBar->SetStatusText("No image loaded", 0);
           m_statusBar->SetStatusText("", 1);
           m_statusBar->SetStatusText("", 2);
           return;
       }

       const wxImage& img = m_document.getPage(m_currentPage);
       m_statusBar->SetStatusText(
           wxString::FromUTF8(m_document.getPagePath(m_currentPage)), 0);
       m_statusBar->SetStatusText(
           wxString::Format("%dx%d", img.GetWidth(), img.GetHeight()), 1);
       m_statusBar->SetStatusText(
           wxString::Format("%.0f%%", m_canvas->getZoom() * 100), 2);
   }
   ```

### 1.5 Integration with MainWindow

**Modify MainWindow to launch ImageViewerWindow**:

```cpp
// In MainWindow.h, add:
private:
    std::unique_ptr<ImageViewerWindow> m_imageViewer;
    void onOpenImage(wxCommandEvent& event);

// In MainWindow.cpp setupBindings():
Bind(wxEVT_MENU, &MainWindow::onOpenImage, this, ID_OPEN_IMAGE);

// In UIBuilder.cpp, add to createMenuBar():
wxMenu* fileMenu = new wxMenu();
fileMenu->Append(ID_OPEN_IMAGE, "&Open Image...\tCtrl+O");
menuBar->Insert(0, fileMenu, "&File");  // Insert at beginning

// Handler implementation:
void MainWindow::onOpenImage(wxCommandEvent& event) {
    if (!m_imageViewer) {
        m_imageViewer = std::make_unique<ImageViewerWindow>(this);
    }
    m_imageViewer->Show();
    m_imageViewer->Raise();

    // Trigger the file open dialog
    wxCommandEvent openEvent(wxEVT_MENU, wxID_OPEN);
    m_imageViewer->ProcessWindowEvent(openEvent);
}
```

### 1.6 CMakeLists.txt Modifications

```cmake
# Add new source files to executable
set(IMAGE_VIEWER_SOURCES
    src/ImageViewerWindow.cpp
    src/ImageCanvas.cpp
    src/ImageDocument.cpp
)

# Update main executable (find the add_executable line and add new sources)
if(WIN32)
    add_executable(Bezugszeichenvorrichtung WIN32
        main.cpp
        src/MainWindow.cpp
        # ... existing sources ...
        ${IMAGE_VIEWER_SOURCES}
        # ... rest of sources ...
    )
else()
    add_executable(Bezugszeichenvorrichtung
        main.cpp
        src/MainWindow.cpp
        # ... existing sources ...
        ${IMAGE_VIEWER_SOURCES}
        # ... rest of sources ...
    )
endif()
```

### 1.7 Unit Tests for Phase 1

```cpp
// tests/test_image_viewer.cpp
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/image.h>
#include "ImageDocument.h"
#include "ImageCanvas.h"

class ImageDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize image handlers if not already done
        if (!wxImage::FindHandler(wxBITMAP_TYPE_PNG)) {
            wxImage::AddHandler(new wxPNGHandler);
            wxImage::AddHandler(new wxJPEGHandler);
        }
    }
};

TEST_F(ImageDocumentTest, InitiallyEmpty) {
    ImageDocument doc;
    EXPECT_EQ(doc.getPageCount(), 0);
    EXPECT_FALSE(doc.hasPages());
}

TEST_F(ImageDocumentTest, AddPageIncreasesCount) {
    ImageDocument doc;
    wxImage testImage(100, 100);
    doc.addPage(testImage, "/path/test.png");
    EXPECT_EQ(doc.getPageCount(), 1);
    EXPECT_TRUE(doc.hasPages());
}

TEST_F(ImageDocumentTest, ClearRemovesAllPages) {
    ImageDocument doc;
    wxImage testImage(100, 100);
    doc.addPage(testImage, "/path/test1.png");
    doc.addPage(testImage, "/path/test2.png");
    doc.clear();
    EXPECT_EQ(doc.getPageCount(), 0);
}

TEST_F(ImageDocumentTest, PageIndexValidation) {
    ImageDocument doc;
    wxImage testImage(100, 100);
    doc.addPage(testImage, "/path/test.png");
    EXPECT_TRUE(doc.isValidPageIndex(0));
    EXPECT_FALSE(doc.isValidPageIndex(1));
    EXPECT_FALSE(doc.isValidPageIndex(100));
}

class ImageCanvasTest : public ::testing::Test {
protected:
    wxFrame* m_frame{nullptr};
    ImageCanvas* m_canvas{nullptr};

    void SetUp() override {
        m_frame = new wxFrame(nullptr, wxID_ANY, "Test");
        m_canvas = new ImageCanvas(m_frame);
    }

    void TearDown() override {
        m_frame->Destroy();
    }
};

TEST_F(ImageCanvasTest, InitialZoomIs100Percent) {
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), 1.0);
}

TEST_F(ImageCanvasTest, ZoomInIncreasesZoom) {
    double initialZoom = m_canvas->getZoom();
    m_canvas->zoomIn();
    EXPECT_GT(m_canvas->getZoom(), initialZoom);
}

TEST_F(ImageCanvasTest, ZoomOutDecreasesZoom) {
    m_canvas->setZoom(2.0);
    double initialZoom = m_canvas->getZoom();
    m_canvas->zoomOut();
    EXPECT_LT(m_canvas->getZoom(), initialZoom);
}

TEST_F(ImageCanvasTest, ZoomClampedToMinimum) {
    m_canvas->setZoom(0.01);  // Below minimum
    EXPECT_GE(m_canvas->getZoom(), ImageCanvas::MIN_ZOOM);
}

TEST_F(ImageCanvasTest, ZoomClampedToMaximum) {
    m_canvas->setZoom(100.0);  // Above maximum
    EXPECT_LE(m_canvas->getZoom(), ImageCanvas::MAX_ZOOM);
}

TEST_F(ImageCanvasTest, InitiallyNoImage) {
    EXPECT_FALSE(m_canvas->hasImage());
}

TEST_F(ImageCanvasTest, SetImageSetsHasImage) {
    wxImage testImage(100, 100);
    m_canvas->setImage(testImage);
    EXPECT_TRUE(m_canvas->hasImage());
}

TEST_F(ImageCanvasTest, ClearImageRemovesImage) {
    wxImage testImage(100, 100);
    m_canvas->setImage(testImage);
    m_canvas->clearImage();
    EXPECT_FALSE(m_canvas->hasImage());
}
```

### 1.8 Future Preparation: Tab Panel Structure

Reserve space in `ImageViewerWindow` for a future reference sign panel. This will be activated in Phase 3.

```cpp
// In ImageViewerWindow.h, add a placeholder:
private:
    wxPanel* m_referencePanelPlaceholder;  // For Phase 3: Reference signs tab

// In setupUI(), create the layout with a splitter:
void ImageViewerWindow::setupUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create splitter for future reference panel
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetSashGravity(0.7);  // 70% to image, 30% to panel
    splitter->SetMinimumPaneSize(100);

    // Image canvas (left/main pane)
    m_canvas = std::make_unique<ImageCanvas>(splitter);

    // Placeholder panel for future reference signs (right pane)
    // Initially hidden, will be populated in Phase 3
    m_referencePanelPlaceholder = new wxPanel(splitter, wxID_ANY);
    m_referencePanelPlaceholder->SetBackgroundColour(wxColour(240, 240, 240));

    // For now, only show the canvas (unsplit)
    splitter->Initialize(m_canvas.get());
    // Later: splitter->SplitVertically(m_canvas.get(), m_referencePanelPlaceholder);

    mainSizer->Add(splitter, 1, wxEXPAND);
    SetSizer(mainSizer);
}
```

### 1.9 Phase 1 Checklist

- [ ] Create `include/ImageDocument.h` and `src/ImageDocument.cpp`
- [ ] Create `include/ImageCanvas.h` and `src/ImageCanvas.cpp`
- [ ] Create `include/ImageViewerWindow.h` and `src/ImageViewerWindow.cpp`
- [ ] Add File menu to MainWindow with "Open Image..." option
- [ ] Implement Ctrl+O shortcut binding
- [ ] Implement zoom with Ctrl+Scroll (cursor-centered)
- [ ] Implement pan with mouse drag
- [ ] Implement page navigation for multi-file opening
- [ ] Add toolbar with zoom and page controls
- [ ] Add status bar with image info and zoom level
- [ ] Update CMakeLists.txt with new source files
- [ ] Create `tests/test_image_viewer.cpp`
- [ ] Ensure all tests pass
- [ ] Verify static compilation still works
- [ ] Update CLAUDE.md with new components
- [ ] Update HUMANS.md with pipeline changes

---

## Phase 2: PDF Import with muPDF

### 2.1 Objective

Add capability to open PDF files and render each page as an image, integrated with the existing image viewer infrastructure.

### 2.2 muPDF Integration Strategy

**Key Principle**: muPDF will be built outside the main project tree using an external CMakeLists.txt to avoid modifying muPDF's source code.

#### 2.2.1 Directory Structure

```
Bezugszeichenprüfvorrichtung/
├── libs/
│   ├── wxWidgets/          # Existing
│   └── mupdf/              # muPDF source (git submodule or downloaded)
├── cmake/
│   └── MuPDFExternal.cmake # External project configuration
├── include/
│   └── PdfDocument.h       # PDF wrapper class
├── src/
│   └── PdfDocument.cpp     # PDF implementation
└── CMakeLists.txt          # Main build (imports MuPDFExternal)
```

#### 2.2.2 External CMake Configuration

**Create `cmake/MuPDFExternal.cmake`**:

```cmake
include(ExternalProject)

set(MUPDF_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/mupdf)
set(MUPDF_BINARY_DIR ${CMAKE_BINARY_DIR}/mupdf-build)

# Build muPDF as external project without modifying its source
ExternalProject_Add(mupdf_external
    SOURCE_DIR ${MUPDF_SOURCE_DIR}
    BINARY_DIR ${MUPDF_BINARY_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -C ${MUPDF_SOURCE_DIR}
        XCFLAGS=-fPIC
        HAVE_X11=no
        HAVE_GLUT=no
        HAVE_GLFW=no
        prefix=${MUPDF_BINARY_DIR}/install
        libs
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 0
    BUILD_BYPRODUCTS
        ${MUPDF_SOURCE_DIR}/build/release/libmupdf.a
        ${MUPDF_SOURCE_DIR}/build/release/libmupdf-third.a
)

# Create imported library targets
add_library(mupdf STATIC IMPORTED GLOBAL)
set_target_properties(mupdf PROPERTIES
    IMPORTED_LOCATION ${MUPDF_SOURCE_DIR}/build/release/libmupdf.a
)
add_dependencies(mupdf mupdf_external)

add_library(mupdf-third STATIC IMPORTED GLOBAL)
set_target_properties(mupdf-third PROPERTIES
    IMPORTED_LOCATION ${MUPDF_SOURCE_DIR}/build/release/libmupdf-third.a
)
add_dependencies(mupdf-third mupdf_external)

# Include directories
set(MUPDF_INCLUDE_DIRS ${MUPDF_SOURCE_DIR}/include CACHE PATH "muPDF include directory")
```

#### 2.2.3 Main CMakeLists.txt Integration

```cmake
# Add muPDF support
option(ENABLE_PDF_SUPPORT "Enable PDF import using muPDF" ON)

if(ENABLE_PDF_SUPPORT)
    include(cmake/MuPDFExternal.cmake)
    include_directories(${MUPDF_INCLUDE_DIRS})
    add_definitions(-DHAVE_PDF_SUPPORT)

    # Find required libraries for muPDF
    find_package(Threads REQUIRED)

    # Additional platform-specific dependencies
    if(NOT WIN32)
        # muPDF needs these on Linux
        set(MUPDF_DEPS m pthread)
    endif()
endif()

# Link muPDF to executable (add to target_link_libraries)
if(ENABLE_PDF_SUPPORT)
    target_link_libraries(Bezugszeichenvorrichtung
        mupdf
        mupdf-third
        ${MUPDF_DEPS}
    )
endif()
```

### 2.3 PdfDocument Class

```cpp
// include/PdfDocument.h
#pragma once

#ifdef HAVE_PDF_SUPPORT

#include <string>
#include <vector>
#include <memory>
#include <wx/image.h>

// Forward declarations to avoid including mupdf headers in public header
struct fz_context;
struct fz_document;

class PdfDocument {
public:
    PdfDocument();
    ~PdfDocument();

    // Prevent copying (muPDF context is not copyable)
    PdfDocument(const PdfDocument&) = delete;
    PdfDocument& operator=(const PdfDocument&) = delete;

    // Move semantics
    PdfDocument(PdfDocument&& other) noexcept;
    PdfDocument& operator=(PdfDocument&& other) noexcept;

    // Document operations
    bool open(const std::string& path);
    void close();
    bool isOpen() const;

    // Page operations
    int getPageCount() const;
    wxImage renderPage(int pageIndex, float dpi = 150.0f) const;

    // Error handling
    std::string getLastError() const;

private:
    fz_context* m_ctx{nullptr};
    fz_document* m_doc{nullptr};
    std::string m_lastError;

    void initContext();
    void destroyContext();
};

#endif // HAVE_PDF_SUPPORT
```

```cpp
// src/PdfDocument.cpp
#ifdef HAVE_PDF_SUPPORT

#include "PdfDocument.h"
#include <mupdf/fitz.h>

PdfDocument::PdfDocument() {
    initContext();
}

PdfDocument::~PdfDocument() {
    close();
    destroyContext();
}

void PdfDocument::initContext() {
    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!m_ctx) {
        m_lastError = "Failed to create muPDF context";
        return;
    }

    fz_try(m_ctx) {
        fz_register_document_handlers(m_ctx);
    }
    fz_catch(m_ctx) {
        m_lastError = "Failed to register document handlers";
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
    }
}

void PdfDocument::destroyContext() {
    if (m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
    }
}

bool PdfDocument::open(const std::string& path) {
    close();

    if (!m_ctx) {
        m_lastError = "muPDF context not initialized";
        return false;
    }

    fz_try(m_ctx) {
        m_doc = fz_open_document(m_ctx, path.c_str());
    }
    fz_catch(m_ctx) {
        m_lastError = "Failed to open document: " + path;
        return false;
    }

    return true;
}

void PdfDocument::close() {
    if (m_doc && m_ctx) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
    }
}

bool PdfDocument::isOpen() const {
    return m_doc != nullptr;
}

int PdfDocument::getPageCount() const {
    if (!isOpen()) return 0;

    int count = 0;
    fz_try(m_ctx) {
        count = fz_count_pages(m_ctx, m_doc);
    }
    fz_catch(m_ctx) {
        return 0;
    }
    return count;
}

wxImage PdfDocument::renderPage(int pageIndex, float dpi) const {
    if (!isOpen() || pageIndex < 0 || pageIndex >= getPageCount()) {
        return wxImage();
    }

    wxImage result;
    fz_pixmap* pixmap = nullptr;
    fz_page* page = nullptr;

    fz_try(m_ctx) {
        // Load page
        page = fz_load_page(m_ctx, m_doc, pageIndex);

        // Calculate transformation matrix for desired DPI
        fz_matrix transform = fz_scale(dpi / 72.0f, dpi / 72.0f);

        // Get page bounds and apply transform
        fz_rect bounds = fz_bound_page(m_ctx, page);
        fz_irect bbox = fz_round_rect(fz_transform_rect(bounds, transform));

        // Create pixmap
        pixmap = fz_new_pixmap_with_bbox(m_ctx, fz_device_rgb(m_ctx), bbox, nullptr, 0);
        fz_clear_pixmap_with_value(m_ctx, pixmap, 0xFF);  // White background

        // Render page
        fz_device* device = fz_new_draw_device(m_ctx, transform, pixmap);
        fz_run_page(m_ctx, page, device, fz_identity, nullptr);
        fz_close_device(m_ctx, device);
        fz_drop_device(m_ctx, device);

        // Convert to wxImage
        int width = fz_pixmap_width(m_ctx, pixmap);
        int height = fz_pixmap_height(m_ctx, pixmap);
        unsigned char* samples = fz_pixmap_samples(m_ctx, pixmap);

        result.Create(width, height);
        unsigned char* dest = result.GetData();

        // Copy RGB data (muPDF uses RGB, no alpha in our case)
        for (int i = 0; i < width * height; ++i) {
            dest[i * 3 + 0] = samples[i * 3 + 0];  // R
            dest[i * 3 + 1] = samples[i * 3 + 1];  // G
            dest[i * 3 + 2] = samples[i * 3 + 2];  // B
        }
    }
    fz_always(m_ctx) {
        if (pixmap) fz_drop_pixmap(m_ctx, pixmap);
        if (page) fz_drop_page(m_ctx, page);
    }
    fz_catch(m_ctx) {
        return wxImage();
    }

    return result;
}

std::string PdfDocument::getLastError() const {
    return m_lastError;
}

#endif // HAVE_PDF_SUPPORT
```

### 2.4 Integration with ImageDocument

Modify `ImageDocument` to support PDF loading:

```cpp
// In ImageDocument.h, add:
#ifdef HAVE_PDF_SUPPORT
#include "PdfDocument.h"
#endif

class ImageDocument {
public:
    // Add PDF support
    bool loadFromPdf(const std::string& path, float dpi = 150.0f);
    bool isPdf() const;

private:
#ifdef HAVE_PDF_SUPPORT
    std::unique_ptr<PdfDocument> m_pdfDoc;
#endif
    bool m_isPdf{false};
};

// In ImageDocument.cpp:
bool ImageDocument::loadFromPdf(const std::string& path, float dpi) {
#ifdef HAVE_PDF_SUPPORT
    m_pdfDoc = std::make_unique<PdfDocument>();
    if (!m_pdfDoc->open(path)) {
        return false;
    }

    clear();
    m_isPdf = true;

    int pageCount = m_pdfDoc->getPageCount();
    for (int i = 0; i < pageCount; ++i) {
        wxImage page = m_pdfDoc->renderPage(i, dpi);
        if (page.IsOk()) {
            addPage(page, path + " [Page " + std::to_string(i + 1) + "]");
        }
    }

    return getPageCount() > 0;
#else
    return false;
#endif
}
```

### 2.5 Update File Dialog

```cpp
// In ImageViewerWindow::onOpenFile():
wxFileDialog openDialog(
    this,
    "Open Image or PDF File(s)",
    "", "",
#ifdef HAVE_PDF_SUPPORT
    "All supported files|*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif;*.pdf|"
    "PDF files (*.pdf)|*.pdf|"
#endif
    "Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif)|"
    "*.png;*.jpg;*.jpeg;*.bmp;*.tiff;*.tif|"
    "All files (*.*)|*.*",
    wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE
);
```

### 2.6 Phase 2 Tests

```cpp
// tests/test_pdf_document.cpp
#include <gtest/gtest.h>
#include "PdfDocument.h"

#ifdef HAVE_PDF_SUPPORT

class PdfDocumentTest : public ::testing::Test {
protected:
    PdfDocument m_pdf;
};

TEST_F(PdfDocumentTest, InitiallyNotOpen) {
    EXPECT_FALSE(m_pdf.isOpen());
}

TEST_F(PdfDocumentTest, OpenNonExistentFileFails) {
    EXPECT_FALSE(m_pdf.open("/nonexistent/path.pdf"));
    EXPECT_FALSE(m_pdf.isOpen());
}

TEST_F(PdfDocumentTest, GetPageCountZeroWhenNotOpen) {
    EXPECT_EQ(m_pdf.getPageCount(), 0);
}

TEST_F(PdfDocumentTest, RenderInvalidPageReturnsEmptyImage) {
    wxImage img = m_pdf.renderPage(0);
    EXPECT_FALSE(img.IsOk());
}

// Integration test with real PDF file (if available)
TEST_F(PdfDocumentTest, DISABLED_OpenRealPdf) {
    // Enable this test when a test PDF is available
    EXPECT_TRUE(m_pdf.open("test_data/sample.pdf"));
    EXPECT_TRUE(m_pdf.isOpen());
    EXPECT_GT(m_pdf.getPageCount(), 0);

    wxImage page = m_pdf.renderPage(0);
    EXPECT_TRUE(page.IsOk());
    EXPECT_GT(page.GetWidth(), 0);
    EXPECT_GT(page.GetHeight(), 0);
}

#endif // HAVE_PDF_SUPPORT
```

### 2.7 Phase 2 Checklist

- [ ] Download/clone muPDF to `libs/mupdf/`
- [ ] Create `cmake/MuPDFExternal.cmake`
- [ ] Update main `CMakeLists.txt` with muPDF integration
- [ ] Create `include/PdfDocument.h` and `src/PdfDocument.cpp`
- [ ] Update `ImageDocument` with PDF loading capability
- [ ] Update file dialog to include PDF filter
- [ ] Create `tests/test_pdf_document.cpp`
- [ ] Verify static compilation with muPDF
- [ ] Test PDF rendering at various DPI levels
- [ ] Update CLAUDE.md and HUMANS.md

---

## Phase 3: OCR Integration Preparation

### 3.1 Objective

Prepare modular architecture for OCR integration without implementing OCR (user will provide code later).

### 3.2 Interface Design

```cpp
// include/IOcrEngine.h
#pragma once
#include <wx/image.h>
#include <vector>
#include <string>

struct OcrResult {
    std::wstring text;
    int x, y, width, height;  // Bounding box
    float confidence;
};

struct OcrPageResult {
    std::vector<OcrResult> detections;
    int pageIndex;
};

class IOcrEngine {
public:
    virtual ~IOcrEngine() = default;

    // Initialize engine (load models, etc.)
    virtual bool initialize(const std::string& modelPath) = 0;
    virtual bool isInitialized() const = 0;

    // Process image and return detected text regions
    virtual OcrPageResult processImage(const wxImage& image) = 0;

    // Configuration
    virtual void setConfidenceThreshold(float threshold) = 0;
    virtual float getConfidenceThreshold() const = 0;
};
```

```cpp
// include/DrawingAnalyzer.h
#pragma once
#include "IOcrEngine.h"
#include "ImageDocument.h"
#include "ReferenceDatabase.h"
#include <memory>

class DrawingAnalyzer {
public:
    DrawingAnalyzer();

    void setOcrEngine(std::unique_ptr<IOcrEngine> engine);
    void setDocument(ImageDocument* document);

    // Analyze current document using OCR
    bool analyze();

    // Get results
    const ReferenceDatabase& getResults() const;
    const std::vector<OcrPageResult>& getOcrResults() const;

    // Match OCR results against text analysis
    struct MatchResult {
        std::wstring referenceNumber;
        bool foundInText;
        bool foundInDrawing;
        std::vector<std::pair<int, wxRect>> drawingLocations;  // page + bbox
    };
    std::vector<MatchResult> compareWithTextAnalysis(
        const ReferenceDatabase& textResults) const;

private:
    std::unique_ptr<IOcrEngine> m_ocrEngine;
    ImageDocument* m_document{nullptr};
    ReferenceDatabase m_results;
    std::vector<OcrPageResult> m_ocrResults;
};
```

### 3.3 Reference Panel for ImageViewerWindow

```cpp
// include/ReferencePanel.h
#pragma once
#include <wx/panel.h>
#include <wx/treelist.h>
#include <wx/notebook.h>
#include "DrawingAnalyzer.h"

class ReferencePanel : public wxPanel {
public:
    ReferencePanel(wxWindow* parent);

    // Update display with analysis results
    void setAnalysisResults(const DrawingAnalyzer& analyzer);
    void setTextResults(const ReferenceDatabase& textResults);
    void clear();

    // Selection events (to highlight in image)
    using SelectionCallback = std::function<void(const std::wstring& bz, int page)>;
    void setSelectionCallback(SelectionCallback callback);

private:
    void setupUI();
    void fillReferenceTree();
    void fillComparisonTree();

    wxNotebook* m_notebook;
    wxTreeListCtrl* m_bzTree;        // BZ list from drawing
    wxTreeListCtrl* m_comparisonTree; // Comparison with text

    SelectionCallback m_selectionCallback;
    const DrawingAnalyzer* m_analyzer{nullptr};
};
```

### 3.4 Highlighted Region Overlay

```cpp
// In ImageCanvas.h, add overlay support:
public:
    struct HighlightRegion {
        wxRect bounds;
        wxColour color;
        std::wstring label;
    };

    void setHighlightRegions(const std::vector<HighlightRegion>& regions);
    void clearHighlightRegions();
    void setSelectedRegion(int index);  // Highlight one region differently

private:
    std::vector<HighlightRegion> m_highlightRegions;
    int m_selectedRegion{-1};

    void drawHighlights(wxDC& dc);
```

### 3.5 Phase 3 Checklist

- [ ] Create `include/IOcrEngine.h` interface
- [ ] Create `include/DrawingAnalyzer.h` and stub implementation
- [ ] Create `include/ReferencePanel.h` and `src/ReferencePanel.cpp`
- [ ] Add highlight overlay to `ImageCanvas`
- [ ] Integrate `ReferencePanel` with `ImageViewerWindow` splitter
- [ ] Add menu option to show/hide reference panel
- [ ] Create stub OCR engine for testing
- [ ] Write tests for the interface contracts
- [ ] Update CLAUDE.md and HUMANS.md

---

## Appendix A: Code Style Guidelines

Follow existing project conventions:

1. **Headers**: Use `#pragma once`
2. **Naming**:
   - Classes: `PascalCase`
   - Methods: `camelCase`
   - Member variables: `m_camelCase`
   - Constants: `UPPER_SNAKE_CASE`
3. **Pointers**: Use `std::unique_ptr` for owned objects, raw pointers for non-owning references
4. **wxWidgets**: Use modern `Bind()` pattern, not legacy `Connect()`
5. **Strings**: Use `std::wstring` for text processing, `wxString` for UI
6. **Error handling**: Use return values, not exceptions (except where wxWidgets requires)

## Appendix B: Build Verification

After each phase, verify:

```bash
# Clean build
rm -rf build && mkdir build && cd build

# Configure (enable PDF for Phase 2+)
cmake .. -DENABLE_PDF_SUPPORT=ON

# Build
cmake --build . -j$(nproc)

# Run tests
./tests/unit_tests

# Check binary is statically linked
ldd ./bin/Bezugszeichenvorrichtung  # Should show minimal dependencies

# Check binary size
ls -lh ./bin/Bezugszeichenvorrichtung
```

## Appendix C: Resources

- [wxWidgets Image Viewer Zoom](https://forums.wxwidgets.org/viewtopic.php?t=47064)
- [muPDF CMake Integration](https://github.com/FabriceSalvaire/mupdf-cmake)
- [muPDF C++ Wrapper](https://github.com/antonioborondo/mupdf_wrapper)
