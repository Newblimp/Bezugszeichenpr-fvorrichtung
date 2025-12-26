# HUMANS.md - Quick Reference Guide

## Program Pipeline

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Text       │    │   Debounce   │    │   Scanner    │    │   Error      │
│   Input      │───►│   (200ms)    │───►│  (background │───►│   Detector   │
│              │    │              │    │    thread)   │    │              │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
                                                                    │
                                                                    ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   UI Update  │◄───│  Highlight   │◄───│  Fill Lists  │◄───│  Results in  │
│   (CallAfter)│    │   Errors     │    │  & Trees     │    │  RefDatabase │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
```

## Code Structure

```
include/                        src/
├── MainWindow.h         ◄────► MainWindow.cpp      # Main window, orchestration
├── UIBuilder.h          ◄────► UIBuilder.cpp       # UI construction helper
├── TextAnalyzer.h       ◄────► TextAnalyzer.cpp    # Abstract language analysis
├── GermanTextAnalyzer.h ◄────► GermanTextAnalyzer.cpp
├── EnglishTextAnalyzer.h◄────► EnglishTextAnalyzer.cpp
├── TextScanner.h        ◄────► TextScanner.cpp     # RE2 regex scanning
├── ErrorDetectorHelper.h◄────► ErrorDetectorHelper.cpp
├── ReferenceDatabase.h  ◄────► (header-only)       # BZ↔Term mappings
├── AnalysisContext.h    ◄────► (header-only)       # Scan state container
├── OrdinalDetector.h    ◄────► OrdinalDetector.cpp # "first/second" detection
├── ErrorNavigator.h     ◄────► ErrorNavigator.cpp  # Error position cycling
├── RE2RegexHelper.h     ◄────► RE2RegexHelper.cpp  # wstring↔UTF8 conversion
├── ImageViewerWindow.h  ◄────► ImageViewerWindow.cpp  # Image viewer window
├── ImageCanvas.h        ◄────► ImageCanvas.cpp     # Zoom/pan canvas widget
└── ImageDocument.h      ◄────► ImageDocument.cpp   # Multi-page image data
```

## Key Functions

| Function | File | Purpose |
|----------|------|---------|
| `MainWindow::scanText()` | MainWindow.cpp | Triggers background scan on timer |
| `MainWindow::scanTextBackground()` | MainWindow.cpp | Actual scanning in thread |
| `TextScanner::scan()` | TextScanner.cpp | RE2 regex matching, populates DB |
| `ErrorDetectorHelper::detectErrors()` | ErrorDetectorHelper.cpp | Finds conflicts, unnumbered, article errors |
| `MainWindow::updateUIAfterScan()` | MainWindow.cpp | Updates trees, highlights, labels |
| `TextAnalyzer::stem()` | TextAnalyzer.cpp | Polymorphic stemming (DE/EN) |
| `UIBuilder::buildUI()` | UIBuilder.cpp | Constructs all UI components |
| `ImageCanvas::onMouseWheel()` | ImageCanvas.cpp | Cursor-centered zoom with Ctrl+Scroll |
| `ImageCanvas::updateBitmapCache()` | ImageCanvas.cpp | Pre-scales image for current zoom level |
| `ImageViewerWindow::openFile()` | ImageViewerWindow.cpp | Loads image file(s) into viewer |

## Data Structures

- **`ReferenceDatabase`**: Maps `BZ→Stems` and `Stem→BZs` with positions
- **`AnalysisContext`**: Combines `ReferenceDatabase` + user config (cleared errors, multi-word toggles)
- **`StemVector`**: `std::vector<std::wstring>` for multi-word term stems

## Threading Model

- Main thread: UI events, updates
- Scan thread: `std::jthread` for background scanning
- Sync: `std::mutex m_dataMutex` protects `m_ctx`
- Cancel: `std::atomic<bool> m_cancelScan`
- UI update: `wxWindow::CallAfter()` dispatches to main thread

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
./tests/unit_tests
```

Static linking: `wxBUILD_SHARED OFF` in CMakeLists.txt

## Phase 1: Image Viewer

- **ImageViewerWindow**: Separate window for viewing scanned drawings
  - Open via File → Open Image (Ctrl+O) in main window
  - Supports PNG, JPEG, BMP, TIFF formats (Phase 1) + PDF (Phase 2)
  - Multi-page support for multiple selected files
- **ImageCanvas**: Custom wxScrolledCanvas widget
  - Zoom: Ctrl+Scroll (cursor-centered, 10%-1000%)
  - Pan: Click and drag with mouse
  - Double-buffered rendering with bitmap cache
- **ImageDocument**: Data model for multi-page images

## Phase 2: PDF Import

- **PdfDocument**: PDF file loader using muPDF
  - Statically linked (no external dependencies)
  - Renders PDF pages to wxImage at configurable DPI (default 150)
  - RAII-based resource management
  - Thread-safe (each instance has own fz_context)
- **muPDF Integration**: External CMake build (cmake/MuPDFExternal.cmake)
  - Built from source in libs/mupdf/ (version 1.24.11)
  - Fully static linkage (23MB binary size, minimal config)
  - No source modifications (as required)
  - Disabled: XPS, SVG, CBZ, HTML, EPUB, JavaScript, CJK fonts
  - Enabled: PDF rendering only (sufficient for German/English patents)
- **ImageDocument**: Transparently handles both images and PDFs
  - Auto-detects PDF files by extension
  - Renders all pages to image format for unified display

---
*Last updated: 2025-12-26*
