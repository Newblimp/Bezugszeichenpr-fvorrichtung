# PDF Processing Setup Guide

This guide explains how to install the required dependencies for Bezugszeichenprüfvorrichtung. The PDF processing feature allows you to:

- Drag and drop multi-page PDF files containing patent figures
- Automatically detect reference signs using OCR (Optical Character Recognition)
- Compare detected reference signs with those found in the text
- View annotated PDFs with green boxes (sign found in text) and red boxes (sign not found in text)

## Required Dependencies

The application requires three external libraries to compile and run:

1. **MuPDF** - For PDF rendering
2. **Tesseract** - For OCR (text recognition from images)
3. **Leptonica** - Image processing library (required by Tesseract)

## Windows Setup

### Option 1: Using vcpkg (Recommended)

vcpkg is Microsoft's C++ package manager and is the easiest way to install dependencies on Windows.

#### Step 1: Install vcpkg

```powershell
# Clone vcpkg repository
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat
```

#### Step 2: Install Required Libraries

```powershell
# Install MuPDF, Tesseract, and Leptonica
.\vcpkg install mupdf:x64-windows tesseract:x64-windows leptonica:x64-windows

# This will take several minutes as it builds all dependencies
```

#### Step 3: Install Tesseract Language Data

Tesseract requires trained data files for OCR. Download the German language data:

```powershell
# Create tessdata directory
mkdir C:\tessdata
cd C:\tessdata

# Download German trained data (for LSTM engine)
Invoke-WebRequest -Uri "https://github.com/tesseract-ocr/tessdata/raw/main/deu.traineddata" -OutFile "deu.traineddata"

# Optional: Download English data as well
Invoke-WebRequest -Uri "https://github.com/tesseract-ocr/tessdata/raw/main/eng.traineddata" -OutFile "eng.traineddata"

# Set environment variable so Tesseract can find the data
[System.Environment]::SetEnvironmentVariable('TESSDATA_PREFIX', 'C:\tessdata', [System.EnvironmentVariableTarget]::User)
```

#### Step 4: Configure CMake with vcpkg

When building the project, tell CMake to use vcpkg:

```powershell
cd path\to\Bezugszeichenprüfvorrichtung
mkdir build
cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build . --config Release
```

### Option 2: Manual Installation

If you prefer not to use vcpkg, you can manually download and install the libraries:

1. **MuPDF**: Download from https://mupdf.com/releases/
2. **Tesseract**: Download installer from https://github.com/UB-Mannheim/tesseract/wiki
3. **Leptonica**: Usually included with Tesseract installer

After installation, you'll need to:
- Set appropriate environment variables (`MUPDF_ROOT`, `TESSERACT_ROOT`, etc.)
- Update CMakeLists.txt to point to the library locations

**Note**: vcpkg is strongly recommended as it handles all these details automatically.

## Linux Setup

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
    libmupdf-dev \
    libtesseract-dev \
    libleptonica-dev \
    tesseract-ocr-deu

# Build normally
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Fedora/RHEL

```bash
# Install dependencies
sudo dnf install -y \
    mupdf-devel \
    tesseract-devel \
    leptonica-devel \
    tesseract-langpack-deu

# Build normally
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Verifying Installation

When you run CMake, you should see messages indicating the libraries were found:

```
-- MuPDF found via vcpkg
-- Tesseract found via vcpkg
-- Leptonica found via vcpkg
-- PDF processing libraries found - feature will be enabled
```

If any library is missing, CMake will fail with an error message:

```
CMake Error at CMakeLists.txt:108 (message):
  MuPDF not found. Install via vcpkg: vcpkg install mupdf
```

**Note**: All three libraries (MuPDF, Tesseract, and Leptonica) are required. The application will not build without them.

## Using the PDF Feature

Once compiled with PDF support:

1. **Launch the application**
2. **Enter or paste patent text** in the left panel - this will detect reference signs
3. **Drag and drop a PDF file** into the right panel (the third section)
4. **Wait for processing** - OCR may take a few seconds per page
5. **View results**:
   - Green boxes: Reference sign found in both PDF and text ✓
   - Red boxes: Reference sign found in PDF but NOT in text ✗

## Troubleshooting

### "Failed to initialize Tesseract"

**Cause**: Tesseract can't find language data files.

**Solution**:
- Ensure `TESSDATA_PREFIX` environment variable is set
- Verify `deu.traineddata` exists in the tessdata directory
- Restart your terminal/IDE after setting environment variables

### "Failed to open PDF"

**Cause**: MuPDF can't parse the PDF file.

**Solution**:
- Ensure the file is a valid PDF (try opening in Adobe Reader)
- Some encrypted/protected PDFs may not work
- Try re-saving the PDF in a different viewer

### "No reference signs detected"

**Cause**: OCR quality issues or reference signs not in expected format.

**Solution**:
- Ensure PDF has good resolution (at least 150 DPI)
- Reference signs must match pattern: digits optionally followed by letters (e.g., "10", "12a")
- Try adjusting OCR settings in `PDFPanel.cpp` (PSM mode, whitelist characters)

### Build fails with "undefined reference to fz_*"

**Cause**: MuPDF library not properly linked.

**Solution**:
- Reinstall MuPDF via vcpkg: `vcpkg remove mupdf && vcpkg install mupdf:x64-windows`
- Clean build directory: `rm -rf build && mkdir build`
- Verify CMake found MuPDF (check CMake output)

## Performance Considerations

- **OCR is slow**: Processing a 10-page PDF may take 30-60 seconds
- **Memory usage**: Each page is rendered at 2x resolution for better OCR accuracy
- **Progress dialog**: A progress bar shows OCR status during processing

## Customization

You can customize OCR behavior in `src/PDFPanel.cpp`:

```cpp
// Line 117: Change OCR language
m_tesseract->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY);  // For English

// Line 124: Change OCR mode
m_tesseract->SetPageSegMode(tesseract::PSM_AUTO);  // Auto-detect layout

// Line 127: Change character whitelist
m_tesseract->SetVariable("tessedit_char_whitelist", "0123456789");  // Digits only
```

## Additional Resources

- **MuPDF Documentation**: https://mupdf.com/docs/
- **Tesseract Wiki**: https://github.com/tesseract-ocr/tesseract/wiki
- **vcpkg Documentation**: https://github.com/microsoft/vcpkg
- **Leptonica**: http://www.leptonica.org/

## Support

If you encounter issues:

1. Check the CMake output for library detection messages
2. Verify environment variables are set correctly
3. Try the vcpkg approach if manual installation fails
4. Consult the library documentation links above
