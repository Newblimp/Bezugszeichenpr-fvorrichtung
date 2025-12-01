# PDF Patent Figures Analysis Feature

## Overview

This optional feature adds a third panel to the application for analyzing reference numbers in patent figure PDFs using OCR (Optical Character Recognition).

## Status

**OPTIONAL FEATURE** - Disabled by default. Must be explicitly enabled during CMake configuration.

## Features

When enabled, the application provides:

1. **Three-Column Layout**:
   - Left: Text editor for patent description
   - Middle: Reference number analysis (existing feature)
   - Right: PDF viewer with OCR analysis

2. **PDF Analysis**:
   - Drag-and-drop support for multi-page PDFs
   - Automatic OCR detection of reference numbers
   - Visual annotation with colored rectangles:
     - **Green**: Reference number exists in both text and PDF
     - **Red**: Reference number only in PDF (missing from text)

3. **Automatic Synchronization**:
   - Reference numbers are automatically compared between text and PDF
   - Real-time updates when text is rescanned

## Dependencies

To enable this feature, you need to install:

### Windows (via vcpkg recommended)
```bash
vcpkg install tesseract leptonica mupdf
```

Or manually install:
- **Tesseract OCR** (5.0+): https://github.com/tesseract-ocr/tesseract
- **Leptonica** (1.80+): http://www.leptonica.org/
- **MuPDF** (1.20+): https://mupdf.com/

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install tesseract-ocr libtesseract-dev libleptonica-dev libmupdf-dev mupdf-tools
```

### macOS (Homebrew)
```bash
brew install tesseract leptonica mupdf
```

## Building with PDF Feature

### Enable the feature during CMake configuration:

```bash
mkdir build
cd build
cmake -DENABLE_PDF_ANALYSIS=ON ..
cmake --build .
```

### Windows-specific (if not using vcpkg)

If you installed the dependencies manually, you may need to specify library paths:

```bash
cmake -DENABLE_PDF_ANALYSIS=ON \
      -DTESSERACT_INCLUDE_DIR="C:/path/to/tesseract/include" \
      -DTESSERACT_LIB="C:/path/to/tesseract/lib/tesseract.lib" \
      -DLEPTONICA_INCLUDE_DIR="C:/path/to/leptonica/include" \
      -DLEPTONICA_LIB="C:/path/to/leptonica/lib/lept.lib" \
      -DMUPDF_INCLUDE_DIR="C:/path/to/mupdf/include" \
      -DMUPDF_LIB="C:/path/to/mupdf/lib/mupdf.lib" \
      ..
```

## Building without PDF Feature (Default)

If you don't need PDF analysis, simply build normally:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The application will work perfectly fine without this feature, just without the third PDF panel.

## Usage

1. Start the application
2. Paste patent description text in the left panel
3. Drag a PDF file containing patent figures into the right panel
4. The application will:
   - Render each PDF page
   - Run OCR to detect reference numbers
   - Draw green squares around numbers that match the text
   - Draw red squares around numbers missing from the text

## Technical Details

### Libraries Used
- **Tesseract OCR**: Text recognition in images
- **Leptonica**: Image processing for Tesseract
- **MuPDF**: PDF rendering and page extraction

### Implementation
- `PDFPanel` class handles all PDF operations
- OCR configured to recognize alphanumeric reference numbers (e.g., "10", "10a", "10'")
- Supports multi-page PDFs with vertical scrolling
- Automatic memory management with smart pointers

### Performance Notes
- PDF rendering is done once when file is dropped
- OCR processing may take a few seconds for large PDFs
- Annotated images are cached for fast display

## Troubleshooting

### CMake can't find dependencies
- **Windows**: Ensure vcpkg is integrated (`vcpkg integrate install`)
- **Linux**: Check that dev packages are installed (`*-dev`)
- **All**: Verify library paths are in your system PATH

### Build fails with linking errors
- Ensure all required libraries are installed
- Check that library versions are compatible (Tesseract 5.0+)
- On Windows, ensure you're using the correct architecture (x64 vs x86)

### Runtime: PDF won't load
- Check that Tesseract language data is installed (tessdata)
- Verify PDF is not encrypted or password-protected
- Ensure PDF contains actual images (not just vector graphics)

## License

Same as main project. See LICENSE file for details.

## Contributing

When modifying PDF-related code:
1. Ensure changes work with `ENABLE_PDF_ANALYSIS=OFF` (disabled)
2. Test on both Windows and Linux if possible
3. Keep the feature optional - don't break existing functionality
4. Update this README if adding new dependencies or features
