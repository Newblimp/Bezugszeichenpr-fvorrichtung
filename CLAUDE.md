# CLAUDE.md - AI Assistant Guide

## Project Overview

**Bezugszeichenprüfvorrichtung** (Reference Number Verification Tool) is a desktop GUI application for validating reference numbers in German patent applications. It automatically checks that technical terms are consistently numbered throughout a patent document, highlighting errors such as missing numbers, conflicting assignments, or incorrect article usage.

### Key Features
- Scans text for terms paired with reference numbers (e.g., "Lager 10" or "bearing 10")
- **Bilingual Support**: Both German and English language analysis with selectable UI
- Detects missing reference numbers for previously defined terms
- Identifies conflicting number assignments (same term with different numbers)
- Validates article usage (definite/indefinite) for both languages
- Supports multi-word term matching (e.g., "zweites Lager" or "second bearing")
- **Automatic multi-word detection**: Automatically enables multi-word mode for terms that appear with both "first" and "second" ordinal prefixes
- Uses Oleander stemming library for language morphology (handles plurals, cases)
- Stemming cache for improved performance on large documents
- Minimum word length filtering (3+ characters) to reduce noise
- User-configurable error clearing and restoration

## Technology Stack

- **Language**: C++20
- **GUI Framework**: wxWidgets 3.x (statically linked)
- **Build System**: CMake 3.14+
- **Regex Engine**: Google RE2 (for performance and safety)
- **NLP**: Oleander stemming library (header-only, German and English support)
- **Testing**: Google Test framework with ~30 unit tests
- **Platform**: Cross-platform (Linux, Windows)
- **Standard Library**: STL with modern C++ features

## Project Structure

```
Bezugszeichenprüfvorrichtung/
├── CMakeLists.txt           # Build configuration
├── main.cpp                 # Application entry point (minimal)
├── res.rc                   # Windows resource file (icons)
├── .gitmodules              # Git submodules (wxWidgets)
├── include/                 # Header files (~18 headers)
│   ├── MainWindow.h         # Main window class declaration
│   ├── GermanTextAnalyzer.h # German language analysis
│   ├── EnglishTextAnalyzer.h# English language analysis
│   ├── OrdinalDetector.h    # Automatic multi-word term detection
│   ├── TextScanner.h        # Text scanning logic (separated)
│   ├── ErrorDetectorHelper.h# Error detection logic (separated)
│   ├── ErrorNavigator.h     # Navigation through errors
│   ├── UIBuilder.h          # UI construction (separated)
│   ├── RegexPatterns.h      # Shared regex pattern constants
│   ├── RE2RegexHelper.h     # RE2/wstring conversion utilities
│   ├── TimerHelper.h        # Performance timing utilities
│   ├── utils.h              # Display utility functions
│   ├── utils_core.h         # Core types (StemVector, hash functions)
│   ├── german_stem.h        # German stemming (Oleander)
│   ├── english_stem.h       # English stemming (Oleander)
│   ├── german_stem_umlaut_preserving.h  # Specialized stemmer
│   ├── stemming.h           # General stemming library
│   ├── stem_collector.h     # Stem collection utilities
│   └── common_lang_constants.h  # Language constants
├── src/                     # Source files (~12 implementations)
│   ├── MainWindow.cpp       # Main window (~650 lines, refactored)
│   ├── GermanTextAnalyzer.cpp  # German language implementation
│   ├── EnglishTextAnalyzer.cpp # English language implementation
│   ├── OrdinalDetector.cpp  # Automatic multi-word term detection
│   ├── TextScanner.cpp      # Text scanning implementation
│   ├── ErrorDetectorHelper.cpp # Error detection implementation
│   ├── ErrorNavigator.cpp   # Navigation implementation
│   ├── UIBuilder.cpp        # UI construction implementation
│   ├── RE2RegexHelper.cpp   # RE2 helper implementation
│   ├── utils.cpp            # Display utilities
│   ├── utils_core.cpp       # Core utilities
│   └── stem_collector.cpp   # Stem collection implementation
├── img/                     # Image assets
│   ├── app_icon.ico         # Application icon
│   ├── check_16.xpm/.png    # Success icon
│   └── warning_16.xpm/.png  # Warning icon
├── tests/                   # Unit tests (~5 test files, 145+ tests)
│   ├── test_german_analyzer.cpp
│   ├── test_english_analyzer.cpp
│   ├── test_re2_regex_helper.cpp
│   ├── test_ordinal_detector.cpp
│   └── test_utils.cpp
└── libs/                    # Third-party libraries
    └── wxWidgets/           # Git submodule
```

## Core Architecture

### Main Components

1. **MainWindow** (`include/MainWindow.h`, `src/MainWindow.cpp`)
   - Central GUI class extending `wxFrame`
   - Coordinates between language analyzers, scanners, and error detectors
   - Implements debounced text scanning (200ms delay)
   - Manages static analyzers for German and English
   - Delegates to specialized helper classes (TextScanner, ErrorDetectorHelper, UIBuilder)

2. **Language Analyzers** (`GermanTextAnalyzer`, `EnglishTextAnalyzer`)
   - Encapsulate language-specific logic (stemming, articles, ignored words)
   - Implement caching for stemming operations (significant performance boost)
   - Use Oleander stemming library internally
   - Provide static methods for article detection and word filtering

3. **TextScanner** (`include/TextScanner.h`, `src/TextScanner.cpp`)
   - Separated scanning logic from MainWindow
   - Handles regex matching for single-word and two-word patterns
   - Builds data structures mapping BZ ↔ stems
   - Prevents overlapping matches

4. **ErrorDetectorHelper** (`include/ErrorDetectorHelper.h`, `src/ErrorDetectorHelper.cpp`)
   - Separated error detection logic from MainWindow
   - Detects unnumbered words, article errors, conflicting assignments
   - Handles text highlighting with wxRichTextCtrl
   - Respects user-cleared error positions

5. **ErrorNavigator** (`include/ErrorNavigator.h`, `src/ErrorNavigator.cpp`)
   - Generic navigation through error lists
   - Reduces code duplication (used by all 5 error types)
   - Updates labels and text selection

6. **UIBuilder** (`include/UIBuilder.h`, `src/UIBuilder.cpp`)
   - Separated UI construction from MainWindow
   - Creates all widgets, layouts, and navigation buttons
   - Returns structured component bundle

7. **OrdinalDetector** (`include/OrdinalDetector.h`, `src/OrdinalDetector.cpp`)
   - Automatically detects multi-word terms with ordinal prefixes
   - Scans for "first"/"second" (English) or "erste"/"zweite" (German) patterns
   - Identifies base words appearing with BOTH first AND second ordinals
   - Auto-enables multi-word mode for detected terms
   - Supports all declensions of German ordinals
   - Respects manual user toggles (manual settings take precedence)

8. **RE2RegexHelper** (`include/RE2RegexHelper.h`, `src/RE2RegexHelper.cpp`)
   - Bridges RE2 (UTF-8) and wxWidgets/STL (wstring)
   - Provides MatchIterator for convenient iteration
   - Handles position mapping between encodings

9. **Data Structures** (see `include/utils_core.h`)
   - `StemVector`: Vector of stemmed word(s) representing a term
   - `StemVectorHash`: Custom hash function for unordered containers
   - `BZComparatorForMap`: Custom comparator for reference numbers (sorts numerically)

10. **Key Mappings** (in MainWindow class)
   ```cpp
   // Reference number -> stemmed terms
   std::map<std::wstring, std::unordered_set<StemVector>, BZComparatorForMap> m_bzToStems;

   // Stemmed term -> reference numbers
   std::unordered_map<StemVector, std::unordered_set<std::wstring>, StemVectorHash> m_stemToBz;

   // Reference number -> original words (for display)
   std::unordered_map<std::wstring, std::unordered_set<std::wstring>> m_bzToOriginalWords;

   // Position tracking for highlighting
   std::unordered_map<std::wstring, std::vector<std::pair<size_t,size_t>>> m_bzToPositions;
   std::unordered_map<StemVector, std::vector<std::pair<size_t,size_t>>, StemVectorHash> m_stemToPositions;

   // Multi-word base stems: combined manual + auto (e.g., "lager" triggers "erstes Lager" matching)
   std::unordered_set<std::wstring> m_multiWordBaseStems;

   // Auto-detection tracking: stems automatically detected by OrdinalDetector
   std::unordered_set<std::wstring> m_autoDetectedMultiWordStems;

   // Manual toggles: stems user explicitly enabled via context menu
   std::unordered_set<std::wstring> m_manualMultiWordToggles;

   // Manual disables: stems user explicitly disabled (prevents auto re-enabling)
   std::unordered_set<std::wstring> m_manuallyDisabledMultiWord;
   ```

### Text Processing Pipeline

1. **Text Input** → `debounceFunc()` → 200ms timer → `scanText()`
2. **Language Selection**: Static `s_useGerman` flag determines which analyzer to use
3. **Automatic Ordinal Detection** (new):
   - `OrdinalDetector::detectOrdinalPatterns()` scans for ordinal prefix patterns
   - Identifies base words appearing with BOTH "first" AND "second" ordinals
   - Examples: "erste Lager ... zweite Lager", "first bearing ... second bearing"
   - Auto-enables multi-word mode for detected terms
   - Respects manual user toggles (manual settings override auto-detection)
   - Clears previous auto-detected stems on each scan (language-specific)
4. **Regex Matching**: Three RE2 regex patterns (defined in `RegexPatterns.h`)
   - `SINGLE_WORD_PATTERN`: Matches "word number" (e.g., "Lager 10", min 3 chars)
   - `TWO_WORD_PATTERN`: Matches "word word number" (e.g., "erstes Lager 10")
   - `WORD_PATTERN`: Matches isolated words (for finding unnumbered terms)
5. **Stemming**: Language-specific stemmer with caching
   - German: `german_stem<>` from Oleander library
   - English: `english_stem<>` from Oleander library
   - Cache prevents redundant stemming of same words
   - Handles morphology (umlauts, cases, plurals)
6. **Multi-word Detection**: Context-sensitive matching
   - If base word is in `m_multiWordBaseStems` (manual + auto-detected), match two-word patterns
   - Example: "lager" in set → "erstes Lager 10" → StemVector{"erst", "lager"}
   - User can manually toggle via context menu in tree view
7. **Error Detection** (delegated to `ErrorDetectorHelper`):
   - `findUnnumberedWords()`: Finds terms missing reference numbers
   - `isUniquelyAssigned()`: Detects conflicting/split number assignments
   - `checkArticleUsage()`: Validates definite/indefinite articles
8. **Word Filtering**:
   - Minimum 3-character words (filters out articles, prepositions)
   - Ignored words list (e.g., "Figure", "Figur" for German/English)

### UI Components

- **Main Text Box**: `wxRichTextCtrl` for input text with syntax highlighting
- **Notebook Tabs**:
  - Reference Number List (`m_bzList`): Shows all BZ → terms mappings
  - Term Tree (`m_treeList`): Hierarchical view of terms and assignments
- **Navigation Buttons**: Jump to next/previous errors
  - No number errors (yellow highlight)
  - Wrong number errors (split assignments)
  - Wrong article errors (cyan highlight)

## Build System

### CMake Configuration

**Key Settings**:
- C++ Standard: C++20
- Build Type: Release (with O3 optimization)
- wxWidgets: Statically linked
- Optimization flags: `-ffunction-sections -fdata-sections -Wl,--gc-sections`

**Platform-Specific**:

**Windows** (MSVC or MinGW):
```cmake
add_definitions(-DwxUSE_UNICODE_WCHAR=1 -DwxUSE_RICHEDIT=1 -DwxUSE_WINRT=1)
# MSVC: /O2 /GL /LTCG optimization
# Resources: res.rc compiled into executable
```

**Linux**:
```cmake
add_compile_options(-Wno-write-strings)
add_definitions(-DwxUSE_UNICODE_WCHAR=1 -DwxUSE_STL=1)
```

### Build Instructions

```bash
# Initialize wxWidgets submodule (first time only)
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . -j$(nproc)

# Binary location: build/Bezugszeichenvorrichtung[.exe]
```

## Development Conventions

### Code Style

1. **Naming**:
   - Member variables: `m_camelCase` (e.g., `m_bzToStems`)
   - Local variables: `camelCase`
   - Functions: `camelCase`
   - Types: `PascalCase` (e.g., `StemVector`)
   - Constants: Not many in codebase, use `UPPER_CASE` if needed

2. **String Types**:
   - Use `std::wstring` for all text (wide strings for Unicode/German)
   - wxWidgets types: `wxString` (UTF-8 compatible)
   - Convert with `wxString::FromUTF8()` and `.ToStdWstring()`

3. **Smart Pointers**:
   - UI components mostly use `std::shared_ptr` (e.g., `m_treeList`)
   - wxWidgets manages some objects internally (don't delete manually)
   - Raw pointers acceptable for wxWidgets parent-child relationships

4. **Regex**:
   - Use `re2::RE2` for patterns (not `std::wregex` - RE2 is faster and safer)
   - Patterns defined in `RegexPatterns.h` namespace
   - Case-insensitive matching with `(?i)` prefix
   - Unicode support: `\p{L}` for letters (handles German, English, etc.)
   - Helper class `RE2RegexHelper` handles wstring ↔ UTF-8 conversion

### Important Patterns

1. **Stemming Pattern** (with caching):
   ```cpp
   void GermanTextAnalyzer::stemWord(std::wstring& word) {
       if (word.empty()) return;

       // Normalize: lowercase first character
       std::wstring normalized = word;
       normalized[0] = m_ctypeFacet->tolower(normalized[0]);

       // Check cache first
       auto it = m_stemCache.find(normalized);
       if (it != m_stemCache.end()) {
           word = it->second;
           return;
       }

       // Apply stemmer and cache result
       m_germanStemmer(normalized);
       m_stemCache[word] = normalized;
       word = normalized;
   }
   ```

2. **Multi-word Detection**:
   - Check if second word's stem is in `m_multiWordBaseStems`
   - If yes, create two-word StemVector
   - If no, treat first word as adjective (ignore)

3. **Error Position Tracking**:
   - Store positions as `vector<int>` with alternating start/end pairs
   - Index stored in `m_*Selected` variables (-2 = not selected)
   - Update selection to highlight in text control

4. **Debouncing**:
   - Text changes trigger `debounceFunc()`
   - Starts 200ms one-shot timer (`m_debounceTimer`)
   - Timer event calls `scanText()` to avoid excessive processing

5. **Language Switching**:
   - Static flag `MainWindow::s_useGerman` controls active language
   - Static helper methods dispatch to appropriate analyzer
   - Example: `MainWindow::createCurrentStemVector()` calls German or English analyzer
   - Switching language triggers immediate rescan

6. **Error Clearing**:
   - Two clearing mechanisms: by BZ number (overview) or by text position (textbox)
   - `m_clearedErrors`: Set of BZ numbers user has cleared
   - `m_clearedTextPositions`: Set of text ranges user has cleared
   - Menu options to restore: all errors, textbox errors only, or overview errors only

### Text Highlighting Colors

- **Neutral**: Default text (black on white)
- **Yellow Background**: Missing reference number or conflicting assignment
- **Cyan Background**: Incorrect article usage (definite vs indefinite)

## Key Algorithms

### 1. Text Scanning Algorithm (`scanText()`)

```
1. Get full text from m_textBox
2. Clear all data structures
3. Apply m_twoWordRegex to find "word1 word2 number" patterns
   - If word2 stem is in m_multiWordBaseStems:
     * Create StemVector{word1_stem, word2_stem}
     * Map to reference number
4. Apply m_singleWordRegex to find "word number" patterns
   - Create StemVector{word_stem}
   - Map to reference number
5. Find unnumbered words (terms that should have numbers)
6. Detect conflicts (same term with different numbers)
7. Check article usage
8. Update UI (tree, list, highlighting)
```

### 2. Conflict Detection (`highlightConflicts()`)

```
For each StemVector that maps to multiple reference numbers:
1. Get all positions where this stem appears
2. Highlight each position in yellow
3. Add positions to m_wrongNumberPositions for navigation
```

### 3. Article Validation (`checkArticleUsage()`)

```
For each reference number:
1. Check if it's uniquely assigned to one term
2. For each occurrence of the number:
   a. Find preceding word
   b. If it's an article (der/die/das/ein/eine/etc.):
      - First occurrence: Record article type (definite/indefinite)
      - Subsequent: Compare with first occurrence
      - If different: Highlight in cyan
```

## File-Specific Notes

### `include/MainWindow.h`
- Lines 70-80: RE2 regex patterns (initialized from `RegexPatterns.h`)
- Lines 83-97: Static analyzers and language-dispatch helper methods
- Lines 110-143: Core data structures (understand before modifying)
- Lines 175-194: Error tracking (position vectors and selection indices for 5 error types)

### `src/MainWindow.cpp`
- Lines 20-23: Static analyzer initialization and language flag
- Lines 26-80: Language-dispatch helper methods (delegate to analyzers)
- Line 120: Debounce timer (200ms, adjust if needed)
- Lines 123-182: `scanText()` - orchestrates scanning, error detection, UI updates
- Lines 315-351: `setupUi()` - delegates to UIBuilder
- Lines 418-483: Context menu handlers (multi-word toggle, error clearing)
- Lines 569-624: Right-click error clearing in textbox
- Lines 647-653: Language change handler

### `include/utils_core.h`
- Line 12: `StemVector` type alias (fundamental to the design)
- Lines 15-25: `StemVectorHash` (don't modify without good reason)
- Lines 28-69: `BZComparatorForMap` (ensures numeric sorting: 2, 10, 10a, 11)
- Lines 72-84: `StemVectorComparator` for ordered containers

### `include/utils.h`
- Display utility functions: `stemVectorToString()`, `stemsToDisplayString()`
- Used for showing terms in tree view and BZ list

### `include/RegexPatterns.h`
- Namespace with `constexpr` pattern constants
- Shared between MainWindow and tests
- All patterns require 3+ character words
- All patterns use case-insensitive matching `(?i)`

### `CMakeLists.txt`
- Line 7: wxBUILD_SHARED OFF (static linking, increases binary size but simplifies distribution)
- Lines 29-49: FetchContent for Abseil (required by RE2)
- Lines 42-49: FetchContent for RE2 regex library
- Lines 52-62: FetchContent for Google Test
- Lines 64-84: Code coverage support (ENABLE_COVERAGE option)
- Lines 120-127: Windows executable with all source files
- Lines 129-132: Linux executable with all source files
- Line 134: Link against wx::core, wx::base, wx::richtext, and re2
- Line 137: Add tests subdirectory

## Common Development Tasks

### Adding a New Source File

1. Create `src/NewFile.cpp` and `include/NewFile.h`
2. Edit `CMakeLists.txt` line 27 or 31 (platform-dependent)
3. Add to executable: `src/NewFile.cpp`
4. Rebuild

### Modifying Regex Patterns

1. Edit patterns in `include/RegexPatterns.h` (namespace constants)
2. Patterns are RE2 syntax (not std::regex), use RE2 documentation
3. Test with unit tests in `tests/test_re2_regex_helper.cpp`
4. Use `\p{L}` for Unicode letters (handles German, English, and more)
5. Use `(?i)` prefix for case-insensitive matching
6. Remember: patterns are shared between application and tests

### Adding New Error Type

1. Add position vector: `std::vector<int> m_newErrorPositions;`
2. Add selection index: `int m_newErrorSelected{-2};`
3. Add navigation buttons and handlers
4. Implement detection logic in scanning pipeline
5. Add highlighting in appropriate method

### Working with wxWidgets

- Use `wxString::FromUTF8()` for string literals with German characters
- Connect events with `Bind()` method
- UI updates must happen on main thread
- Use `wxRichTextCtrl::SetStyle()` for text highlighting

## Git Workflow

### Current Branch
- Active branch: `claude/claude-md-mikjpsukk114yoi5-01MoikcjC4cnCxE2mSBX8r76`
- Main branch: (not specified, likely `main` or `master`)

### Submodule Management
- wxWidgets is a git submodule at `libs/wxWidgets`
- Initialize with: `git submodule update --init --recursive`
- Don't modify submodule files directly

### Commit Conventions
- Recent commits show descriptive messages
- Focus on "what changed" not implementation details
- Examples: "fixed highlighting of multi-word missing number terms", "fix windows compilation flags"

### Platform Testing
- Test builds on both Windows and Linux when modifying:
  - CMakeLists.txt
  - Platform-specific code (#ifdef _WIN32)
  - Resource files (res.rc)

## Testing Guidelines

### Automated Testing

**Unit Tests**: ~30 tests in 4 test files using Google Test framework

1. **test_german_analyzer.cpp**: Tests German stemming, articles, ignored words
2. **test_english_analyzer.cpp**: Tests English stemming, articles, ignored words
3. **test_re2_regex_helper.cpp**: Tests RE2/wstring conversion, pattern matching
4. **test_utils.cpp**: Tests core utilities (BZ comparator, hash functions)

**Running Tests**:
```bash
cd build
cmake --build . --target unit_tests
./tests/unit_tests
```

**Code Coverage**:
```bash
cmake -DENABLE_COVERAGE=ON ..
cmake --build .
./tests/unit_tests
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/libs/*' '*/tests/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### Manual Testing Checklist

1. **Basic Functionality**:
   - Paste German and English patent text
   - Switch between languages with radio button
   - Verify reference numbers are detected
   - Check term list and tree view populate

2. **Error Detection**:
   - Missing numbers: Remove number from a term, verify yellow highlight
   - Conflicting numbers: Assign different numbers to same term
   - Split numbers: Same number for different terms
   - Article errors: Mix "der Lager 10" and "ein Lager 10"

3. **Multi-word Terms**:
   - Right-click term in tree
   - Enable multi-word mode
   - Verify "erstes/zweites Lager" or "first/second bearing" detected correctly

4. **Error Clearing**:
   - Right-click on highlighted text → Clear error
   - Right-click on tree item → Clear error
   - Use menu: Restore All Errors / Restore Textbox Errors / Restore Overview Errors
   - Verify errors disappear and reappear correctly

5. **Navigation**:
   - Click forward/backward buttons for each error type (5 types total)
   - Verify text selection jumps to errors
   - Check counter updates (e.g., "1/5")

6. **Cross-platform** (if applicable):
   - Test on Windows and Linux
   - Verify icon displays correctly
   - Check text rendering (umlauts, special characters)

## Debugging Tips

1. **Text not highlighting**:
   - Check `m_*Positions` vectors are populated
   - Verify `SetStyle()` calls with correct ranges
   - Ensure text didn't change between scan and highlight

2. **Stemming issues**:
   - Print stem outputs to verify stemmer behavior
   - Check lowercase conversion happens before stemming
   - Test with edge cases (single letter, umlauts)

3. **Regex not matching**:
   - Test patterns in isolation (regex101.com with C++ flavor)
   - Verify whitespace in pattern matches actual text
   - Check for Unicode issues (use `std::wregex`, not `std::regex`)

4. **Multi-word not working**:
   - Verify base stem is in `m_multiWordBaseStems`
   - Check second word is stemmed correctly
   - Debug `isMultiWordBase()` return value

## External Dependencies

### Google RE2
- **Location**: Fetched via CMake FetchContent
- **Type**: Compiled library (static)
- **Purpose**: Fast, safe regular expression matching
- **Advantages**: Linear time complexity, no catastrophic backtracking
- **Usage**: Pattern objects (`re2::RE2`), matching with `RE2::PartialMatch()`
- **Helper**: `RE2RegexHelper` class for wstring integration

### Abseil
- **Location**: Fetched via CMake FetchContent
- **Type**: Compiled library (static)
- **Purpose**: Required dependency for RE2
- **Modules Used**: Various base utilities

### Google Test
- **Location**: Fetched via CMake FetchContent
- **Type**: Testing framework
- **Purpose**: Unit testing
- **Usage**: TEST() macros, EXPECT_* assertions

### Oleander Stemming Library
- **Location**: `include/stemming.h`, `include/german_stem.h`, `include/english_stem.h`
- **Type**: Header-only template library
- **Language Support**: German and English (others available but unused)
- **Usage**: `stemming::german_stem<>` / `stemming::english_stem<>` with `operator()`
- **Caching**: Wrapped by TextAnalyzer classes for performance
- **Documentation**: See header comments

### wxWidgets
- **Location**: Git submodule at `libs/wxWidgets`
- **Version**: Latest from git submodule (likely 3.2+)
- **Modules Used**: `wx::core`, `wx::base`, `wx::richtext`
- **Key Classes**: `wxFrame`, `wxRichTextCtrl`, `wxTreeListCtrl`, `wxNotebook`, `wxTimer`, `wxRadioBox`
- **Build**: Static linking enabled (wxBUILD_SHARED OFF)
- **Setup**: `git submodule update --init --recursive`

## Performance Considerations

1. **Debouncing**: 200ms delay prevents excessive rescanning during typing
2. **Stemming Cache**: Significant speedup by caching stemmed words (avoids redundant Oleander calls)
3. **RE2 Regex Engine**: Linear time complexity, significantly faster than std::regex
4. **Freeze/Thaw Pattern**: Text control frozen during updates to batch visual changes
5. **Build Optimizations**: O2, LTO (on MSVC), dead code elimination, function sections
6. **Static Linking**: No runtime dependencies, larger binary but better performance
7. **Text Size**: Designed for patent applications (~10-50 pages), performs well even on large documents
8. **Timer Profiling**: `TimerHelper` class used during development to identify bottlenecks

## Known Issues & Limitations

1. **Language**: German and English supported; other languages require new analyzer classes
2. **Multi-word**: Requires manual activation via context menu (no auto-detection)
3. **Article Detection**: May have false positives with complex grammar
4. **Reference Number Format**: Assumes digits followed by optional letters (10, 10a, 10')
5. **Text Encoding**: Requires Unicode (UTF-8/UTF-16), issues with legacy encodings
6. **Threading**: Single-threaded; scanning happens on main thread (future: separate thread)
7. **Word Length**: 3-character minimum may miss legitimate 2-letter technical abbreviations

## Future Enhancement Ideas

(Note: These are potential improvements, not current features)
- Auto-detect multi-word terms based on frequency
- Support for other languages (requires new TextAnalyzer classes)
- Export validation report to PDF/HTML
- Integrate with patent office XML formats
- Undo/redo for text editing
- Configurable highlight colors
- Separate scanning thread (non-blocking UI during large document scans)
- Persistent settings (language preference, cleared errors, multi-word terms)
- Import/export of multi-word term lists

---

## Quick Reference for AI Assistants

When working on this codebase:

✅ **DO**:
- Use `std::wstring` for all text manipulation
- Test with German and English text containing special characters (äöüÄÖÜß)
- Maintain parallel data structures (BZ→Stem and Stem→BZ)
- Follow the member variable naming convention (`m_prefix`)
- Consider both Windows and Linux when modifying build files
- Use RE2 regex patterns (not `std::regex`) via `RE2RegexHelper`
- Add unit tests for new functionality (use Google Test framework)
- Run tests before committing: `cd build && cmake --build . --target unit_tests && ./tests/unit_tests`
- Update line numbers in this document when making significant changes

❌ **DON'T**:
- Mix narrow strings (`std::string`) with wide strings (use `RE2RegexHelper` for conversions)
- Modify Oleander stemming library headers
- Delete UI objects manually (wxWidgets manages lifecycle)
- Break the debounce pattern (performance will suffer)
- Change data structure types without updating hash/comparator functions
- Forget to update both CMakeLists.txt sections (Windows/Linux) when adding source files
- Use `std::regex` or `std::wregex` (use RE2 instead for performance and safety)
- Modify regex patterns without testing them in unit tests

🔧 **Key Files**:
- Core orchestration: `src/MainWindow.cpp` (~650 lines, refactored)
- Language analysis: `src/GermanTextAnalyzer.cpp`, `src/EnglishTextAnalyzer.cpp`
- Scanning logic: `src/TextScanner.cpp` (separated from MainWindow)
- Error detection: `src/ErrorDetectorHelper.cpp` (separated from MainWindow)
- Data structures: `include/MainWindow.h`, `include/utils_core.h`
- Regex patterns: `include/RegexPatterns.h` (shared constants)
- Build config: `CMakeLists.txt`
- Entry point: `main.cpp` (minimal, just creates MainWindow)
- Tests: `tests/*.cpp` (~30 tests)

📊 **Architecture Summary**:
```
User Input → Debounce Timer (200ms) →
MainWindow::scanText() →
  TextScanner::scanText() (RE2 regex matching) →
  Language Analyzer (stemming with cache) →
  Data Structure Update (BZ↔Stem mappings) →
  ErrorDetectorHelper (find errors) →
  UI Update (Freeze→Highlighting→Thaw + Lists)
```

🏗️ **Recent Major Refactoring** (Dec 2024):
- Separated concerns: MainWindow now delegates to specialized classes
- Added English language support (bilingual: German/English)
- Migrated from `std::wregex` to Google RE2 for better performance
- Implemented stemming cache for significant speed improvements
- Added 30+ unit tests with Google Test
- Minimum 3-character word filtering to reduce noise
- User-configurable error clearing (textbox and overview)
- Reduced MainWindow.cpp from ~900 to ~650 lines

---

## 🚨 IMPORTANT: Update This Documentation

**RULE**: After any significant refactoring, new features, or bug fixes that involve compilation, **ALWAYS update this CLAUDE.md file** before ending the session.

**What to update**:
1. Project structure if files were added/removed/renamed
2. Architecture descriptions if logic was refactored
3. Line number references in "File-Specific Notes" section
4. Technology stack if dependencies changed
5. Build instructions if CMake configuration changed
6. Testing section if tests were added
7. Known issues/limitations if bugs were fixed or new ones discovered
8. The "Last updated" date at the bottom

**How to update**:
- Review the changes made during the session
- Read the affected source files to verify current state
- Update relevant sections with accurate information
- Test that line number references are still correct
- Ensure build instructions work

This documentation is critical for maintaining context across sessions and helping future development.

---

Last updated: 2025-12-16
