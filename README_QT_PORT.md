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

This project uses MSVC with Qt 6.10.1 (MSVC 2022 64-bit).

**Build with Visual Studio:**
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Or open the generated solution file in Visual Studio:
```bash
cmake .. -G "Visual Studio 17 2022" -A x64
start Bezugszeichenvorrichtung.sln
```

**Note:** The Qt path is pre-configured in CMakeLists.txt as `C:\Qt\6.10.1\msvc2022_64`. If your Qt installation is in a different location, update the `CMAKE_PREFIX_PATH` in CMakeLists.txt.

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
