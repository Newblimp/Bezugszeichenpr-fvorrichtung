# CLAUDE.md - AI Assistant Guide

## Project Overview

**Bezugszeichenprüfvorrichtung** (Reference Number Verification Tool) is a desktop GUI application for validating reference numbers in patent applications. It automatically checks that technical terms are consistently numbered, highlighting errors such as missing numbers, conflicting assignments, or incorrect article usage.

### Key Features
- Scans text for terms paired with reference numbers (e.g., "Lager 10" or "bearing 10")
- **Bilingual Support**: German and English language analysis with polymorphic analyzer architecture
- Detects missing reference numbers for previously defined terms
- Identifies conflicting number assignments (same term with different numbers)
- Validates article usage (definite/indefinite) based on term occurrence
- Supports multi-word term matching (e.g., "erstes Lager 10")
- **Automatic multi-word detection**: Detects terms used with both "first" and "second" ordinal prefixes
- Uses Oleander stemming library with a caching layer for high performance
- User-configurable error clearing and restoration

## Technology Stack

- **Language**: C++20
- **GUI Framework**: wxWidgets 3.x (statically linked)
- **Regex Engine**: Google RE2 (fast, linear-time matching)
- **NLP**: Oleander stemming library (German and English support)
- **Testing**: Google Test framework (~150 unit tests)
- **Build System**: CMake 3.14+

## Project Structure

```
Bezugszeichenprüfvorrichtung/
├── include/                 # Header files
│   ├── TextAnalyzer.h       # Abstract base class for language analysis
│   ├── GermanTextAnalyzer.h # German implementation
│   ├── EnglishTextAnalyzer.h# English implementation
│   ├── AnalysisContext.h    # Global scanning state and configuration
│   ├── ReferenceDatabase.h  # Consolidated BZ ↔ Stem mapping data
│   ├── MainWindow.h         # Main GUI coordinator
│   ├── TextScanner.h        # Regex-based scanning logic
│   ├── ErrorDetectorHelper.h# Business logic for error detection
│   └── OrdinalDetector.h    # Logic for auto-detecting multi-word terms
├── src/                     # Source files
│   ├── MainWindow.cpp       # GUI orchestration
│   ├── TextAnalyzer.cpp     # Shared analysis logic
│   ├── German/English...    # Language-specific implementations
│   └── ...                  # Component implementations
└── tests/                   # Unit tests
```

## Core Architecture

### Polymorphic Analysis
The system uses a `TextAnalyzer` interface to decouple business logic from language specifics. `MainWindow` holds a `std::unique_ptr<TextAnalyzer> m_currentAnalyzer` which is swapped when the language changes.

### State Management
Analysis state is encapsulated in two primary structures:
1. **ReferenceDatabase**: Stores the results of a scan (mappings, positions, original words).
2. **AnalysisContext**: Combines the database with user configuration (manual multi-word toggles, cleared errors).

### Text Processing Pipeline
1. **Input**: Text change triggers debounced `scanText()` (200ms delay).
2. **Setup**: `AnalysisContext` is cleared; current `TextAnalyzer` is used.
3. **Multi-word Detection**: `OrdinalDetector` scans for "first/second" patterns to auto-enable multi-word mode.
4. **Scanning**: `TextScanner` uses RE2 to find terms and populate the `ReferenceDatabase`.
5. **Error Detection**: `ErrorDetectorHelper` identifies unnumbered terms, conflicts, and article errors.
6. **UI Update**: `MainWindow` highlights text and populates navigation lists/trees.

## Development Conventions

### String Handling
- Use `std::wstring` for all text processing to support Unicode/German characters.
- Use `RE2RegexHelper` for conversions between `std::wstring` and RE2's UTF-8 requirements.
- wxWidgets conversions: `wxString::FromUTF8(s.c_str())` and `s.ToStdWstring()`.

### Error Navigation
Errors are tracked via `std::vector<std::pair<int, int>>` position lists in `MainWindow`. The `ErrorNavigator` utility handles cycling through these positions and updating UI labels.

### Threading
Scanning runs in a background thread (`m_scanThread`) using `std::jthread`. UI updates are dispatched back to the main thread via `CallAfter`. Mutex protection (`m_dataMutex`) is required for all `AnalysisContext` access.

## Build & Test

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
./tests/unit_tests  # Run all 150+ tests
```

## Last updated: 2025-12-20