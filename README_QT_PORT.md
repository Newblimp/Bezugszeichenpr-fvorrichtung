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

## Distribution

The build system automatically runs `windeployqt` after building to copy all necessary Qt DLLs into the output directory.

**To create a distribution package:**

1. Build in Release mode:
```bash
cmake --build . --config Release
```

2. The executable and all required DLLs will be in `build\Release\`

3. **Distribute the entire `Release` folder** - do NOT distribute just the .exe alone!
   - Zip the entire `Release` folder
   - Users extract and run the .exe from the extracted folder
   - All DLLs must stay in the same folder as the .exe

**What gets deployed (all required for the application to run):**
- `Bezugszeichenvorrichtung.exe` - your application
- `Qt6Core.dll`, `Qt6Widgets.dll`, `Qt6Gui.dll` - Qt libraries
- `platforms\qwindows.dll` - Qt platform plugin (must be in platforms subfolder)
- MSVC runtime DLLs
- Other necessary Qt dependencies

**Important:** The .exe will NOT run without these DLLs. Always distribute the complete folder.

### Alternative: Static Linking (Single .exe file)

If you want a single .exe file with no DLLs:
- Requires building Qt from source with static configuration, OR
- Commercial Qt license with static builds
- Results in much larger .exe file (~20-50 MB instead of ~1 MB)
- Slower build times but simpler distribution

For most use cases, distributing the folder with DLLs is the recommended approach.

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
