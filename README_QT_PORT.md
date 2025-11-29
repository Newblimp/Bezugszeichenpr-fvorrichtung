# Qt Port of Bezugszeichenprüfvorrichtung

This project has been ported from wxWidgets to Qt for cross-platform GUI development.

## Requirements

- CMake 3.10 or higher
- C++20 compatible compiler
- Qt 5.12+ or Qt 6.x with the following modules:
  - Qt Core
  - Qt Widgets
  - Qt Gui

## Building

### Linux/macOS

Install Qt development packages:

**Ubuntu/Debian:**
```bash
sudo apt-get install qt6-base-dev
# or for Qt5:
sudo apt-get install qtbase5-dev
```

**macOS (using Homebrew):**
```bash
brew install qt6
# or for Qt5:
brew install qt@5
```

Then build:
```bash
mkdir build
cd build
cmake ..
make
```

### Windows

This project uses **static linking** with Qt 6.10.1 (MSVC 2022 64-bit) to produce a single standalone .exe without DLL dependencies.

**Qt is built from source automatically during the build process.**

#### Prerequisites

1. Download Qt 6.10.1 source code from https://download.qt.io/official_releases/qt/6.10/6.10.1/single/
   - Get `qt-everywhere-src-6.10.1.zip` or `.tar.xz`
   - Extract to `C:\Qt\6.10.1\Src` (this is the expected location)
   - If you extract to a different location, update `QT_SOURCE_DIR` in CMakeLists.txt

2. Install Visual Studio 2022 with C++ development tools

#### Build the Application

Open "x64 Native Tools Command Prompt for VS 2022" and run:

```bash
cd <your-project-directory>
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**Important Notes:**
- **First build will take 1-3 hours** as Qt is compiled from source statically
- Subsequent builds are much faster as Qt is cached in `build/qt6-build/`
- Only the required Qt modules (Core, Widgets, Gui) are built
- The final .exe is completely standalone with no DLL dependencies

The resulting `Release\Bezugszeichenvorrichtung.exe` will be a standalone executable (~15-30 MB).

## Distribution

**This project uses static linking - you get a single standalone .exe file!**

After building in Release mode:
```bash
cmake --build . --config Release
```

The `build\Release\Bezugszeichenvorrichtung.exe` is a **completely standalone executable**:
- No DLLs required
- No Qt installation needed on user's computer
- Just distribute the single .exe file
- Larger file size (~15-30 MB) but maximum portability

Users can simply copy and run the .exe anywhere on Windows without any dependencies.

**Licensing Note:** Since this uses static linking with Qt (LGPL/GPL), you must either:
- Release your source code under GPL/LGPL, OR
- Provide object files allowing users to relink with different Qt versions (LGPL requirement), OR
- Use a commercial Qt license

Make sure to comply with Qt's licensing requirements for static builds.

## Changes from wxWidgets

The application has been ported to use Qt equivalents:

- `wxApp` → `QApplication`
- `wxFrame` → `QMainWindow`
- `wxRichTextCtrl` → `QTextEdit`
- `wxNotebook` → `QTabWidget`
- `wxTreeListCtrl` → `QTreeWidget`
- `wxButton` → `QPushButton`
- `wxStaticText` → `QLabel`
- `wxTimer` → `QTimer`
- `wxTextAttr` → `QTextCharFormat`
- `wxBoxSizer` → `QVBoxLayout`/`QHBoxLayout`
- Event binding (`Bind`) → Signal/slot connections (`connect`)

All functionality remains the same, including:
- Text scanning and analysis
- Reference number validation
- German article checking
- Multi-word term detection
- Navigation through errors
