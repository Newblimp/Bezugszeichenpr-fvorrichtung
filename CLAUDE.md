# CLAUDE.md - AI Assistant Guide

## Project Overview

**Bezugszeichenprüfvorrichtung** (Reference Number Verification Tool) is a desktop GUI application for validating reference numbers in German patent applications. It automatically checks that technical terms are consistently numbered throughout a patent document, highlighting errors such as missing numbers, conflicting assignments, or incorrect article usage.

### Key Features
- Scans text for terms paired with reference numbers (e.g., "Lager 10")
- Detects missing reference numbers for previously defined terms
- Identifies conflicting number assignments (same term with different numbers)
- Validates German article usage (definite/indefinite)
- Supports multi-word term matching (e.g., "zweites Lager")
- Uses Oleander stemming library for German language morphology (handles plurals, cases)

## Technology Stack

- **Language**: C++20
- **GUI Framework**: wxWidgets 3.x (statically linked)
- **Build System**: CMake 3.10+
- **NLP**: Oleander stemming library (header-only, German language support)
- **Platform**: Cross-platform (Linux, Windows)
- **Standard Library**: STL with modern C++ features

## Project Structure

```
Bezugszeichenprüfvorrichtung/
├── CMakeLists.txt           # Build configuration
├── main.cpp                 # Application entry point
├── res.rc                   # Windows resource file (icons)
├── .gitmodules              # Git submodules (wxWidgets)
├── include/                 # Header files
│   ├── MainWindow.h         # Main window class declaration
│   ├── utils.h              # Utility types and functions
│   ├── stem_collector.h     # Stem collection utilities
│   ├── german_stem.h        # German stemming (Oleander)
│   ├── stemming.h           # General stemming library
│   └── common_lang_constants.h  # Language constants
├── src/                     # Source files
│   ├── MainWindow.cpp       # Main window implementation (~900 lines)
│   ├── utils.cpp            # Utility function implementations
│   └── stem_collector.cpp   # Stem collection logic
├── img/                     # Image assets
│   ├── app_icon.ico         # Application icon
│   ├── check_16.xpm/.png    # Success icon
│   └── warning_16.xpm/.png  # Warning icon
└── libs/                    # Third-party libraries
    └── wxWidgets/           # Git submodule
```

## Core Architecture

### Main Components

1. **MainWindow** (`include/MainWindow.h`, `src/MainWindow.cpp`)
   - Central GUI class extending `wxFrame`
   - Manages all UI components and application logic
   - Implements debounced text scanning (500ms delay)
   - Handles error detection and navigation

2. **Data Structures** (see `include/utils.h`)
   - `StemVector`: Vector of stemmed word(s) representing a term
   - `StemVectorHash`: Custom hash function for unordered containers
   - `BZComparatorForMap`: Custom comparator for reference numbers (sorts numerically)

3. **Key Mappings** (in MainWindow class)
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

   // Multi-word base stems (e.g., "lager" triggers "erstes Lager" matching)
   std::unordered_set<std::wstring> m_multiWordBaseStems;
   ```

### Text Processing Pipeline

1. **Text Input** → `debounceFunc()` → 500ms timer → `scanText()`
2. **Regex Matching**: Three regex patterns
   - `m_singleWordRegex`: Matches "word number" (e.g., "Lager 10")
   - `m_twoWordRegex`: Matches "word word number" (e.g., "erstes Lager 10")
   - `m_wordRegex`: Matches isolated words (for finding unnumbered terms)
3. **Stemming**: German stemmer from Oleander library
   - Converts words to lowercase stems
   - Handles German morphology (umlauts, cases, plurals)
4. **Multi-word Detection**: Context-sensitive matching
   - If base word is in `m_multiWordBaseStems`, match two-word patterns
   - Example: "lager" in set → "erstes Lager 10" → StemVector{"erst", "lager"}
5. **Error Detection**:
   - `findUnnumberedWords()`: Finds terms missing reference numbers
   - `highlightConflicts()`: Detects conflicting number assignments
   - `checkArticleUsage()`: Validates definite/indefinite articles

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
   - Use `std::wregex` for wide string patterns
   - Compile with flags: `ECMAScript | optimize | icase`
   - German characters: `[[:alpha:]äöüÄÖÜß]+`

### Important Patterns

1. **Stemming Pattern**:
   ```cpp
   void stemWord(std::wstring& word) {
       if (word.empty()) return;
       word[0] = std::tolower(word[0]);  // Lowercase first
       m_germanStemmer(word);             // Apply stemmer
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
   - Starts 500ms one-shot timer (`m_debounceTimer`)
   - Timer event calls `scanText()` to avoid excessive processing

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
- Line 64-79: Regex patterns (modify carefully, test with German text)
- Line 92-106: Core data structures (understand before modifying)
- Line 141-157: Error tracking (parallel arrays of positions and selection indices)

### `src/MainWindow.cpp`
- Line 28-30: Debounce timer (500ms, adjust if needed)
- Line 32-59: Stemming and multi-word logic
- Line 61-144: Article detection helpers (German-specific)
- Scanning logic starts around line 200+
- UI setup in `setupUi()` method

### `include/utils.h`
- Line 11: `StemVector` type alias (fundamental to the design)
- Line 14-24: Hash function (don't modify without good reason)
- Line 27-46: BZ comparator (ensures numeric sorting: 2, 10, 10a, 11)

### `CMakeLists.txt`
- Line 7: wxBUILD_SHARED OFF (static linking, increases binary size but simplifies distribution)
- Line 19-27: Windows-specific defines and optimizations
- Line 27/31: Executable sources (add new .cpp files here)

## Common Development Tasks

### Adding a New Source File

1. Create `src/NewFile.cpp` and `include/NewFile.h`
2. Edit `CMakeLists.txt` line 27 or 31 (platform-dependent)
3. Add to executable: `src/NewFile.cpp`
4. Rebuild

### Modifying Regex Patterns

1. Edit patterns in `MainWindow.h` (lines 64-79)
2. Test with sample German text containing umlauts
3. Ensure patterns use `\b` for word boundaries
4. Use `[[:alpha:]äöüÄÖÜß]+` for German words

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

### Manual Testing Checklist

1. **Basic Functionality**:
   - Paste German patent text
   - Verify reference numbers are detected
   - Check term list and tree view populate

2. **Error Detection**:
   - Missing numbers: Remove number from a term, verify yellow highlight
   - Conflicting numbers: Assign different numbers to same term
   - Article errors: Mix "der Lager 10" and "ein Lager 10"

3. **Multi-word Terms**:
   - Right-click term in tree
   - Enable multi-word mode
   - Verify "erstes/zweites Lager" detected correctly

4. **Navigation**:
   - Click forward/backward buttons for each error type
   - Verify text selection jumps to errors
   - Check counter updates (e.g., "1/5")

5. **Cross-platform** (if applicable):
   - Test on Windows and Linux
   - Verify icon displays correctly
   - Check text rendering (umlauts)

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

### Oleander Stemming Library
- **Location**: `include/stemming.h`, `include/german_stem.h`
- **Type**: Header-only template library
- **Language Support**: German (others available but unused)
- **Usage**: `stemming::german_stem<>` class with `operator()` for stemming
- **Documentation**: See header comments

### wxWidgets
- **Version**: Latest from git submodule (likely 3.2+)
- **Modules Used**: `wx::core`, `wx::base`, `wx::richtext`
- **Key Classes**: `wxFrame`, `wxRichTextCtrl`, `wxTreeListCtrl`, `wxNotebook`, `wxTimer`
- **Build**: Static linking enabled (wxBUILD_SHARED OFF)

## Performance Considerations

1. **Debouncing**: 500ms delay prevents excessive rescanning during typing
2. **Regex Optimization**: Patterns compiled with `optimize` flag
3. **Build Optimizations**: O3, LTO (on MSVC), dead code elimination
4. **Static Linking**: No runtime dependencies, larger binary but better performance
5. **Text Size**: Designed for patent applications (~10-50 pages), may slow with very large texts

## Known Issues & Limitations

1. **Language**: German only (stemmer is language-specific)
2. **Multi-word**: Requires manual activation via context menu
3. **Article Detection**: May have false positives with complex grammar
4. **Reference Number Format**: Assumes digits followed by optional letters (10, 10a, 10')
5. **Text Encoding**: Requires Unicode (UTF-8/UTF-16), issues with legacy encodings

## Future Enhancement Ideas

(Note: These are potential improvements, not current features)
- Auto-detect multi-word terms based on frequency
- Support for other languages (requires different stemmers)
- Export validation report to PDF/HTML
- Integrate with patent office XML formats
- Undo/redo for text editing
- Configurable highlight colors
- Spell checking for German technical terms

---

## Quick Reference for AI Assistants

When working on this codebase:

✅ **DO**:
- Use `std::wstring` for all text manipulation
- Test with German text containing äöüÄÖÜß
- Maintain parallel data structures (BZ→Stem and Stem→BZ)
- Follow the member variable naming convention (`m_prefix`)
- Consider both Windows and Linux when modifying build files
- Check that regex patterns use wide string types (`std::wregex`)

❌ **DON'T**:
- Mix narrow strings (`std::string`) with wide strings
- Modify Oleander stemming library headers
- Delete UI objects manually (wxWidgets manages lifecycle)
- Break the debounce pattern (performance will suffer)
- Change data structure types without updating hash/comparator functions
- Forget to update both CMakeLists.txt sections (Windows/Linux)

🔧 **Key Files**:
- Core logic: `src/MainWindow.cpp` (~900 lines)
- Data structures: `include/MainWindow.h`, `include/utils.h`
- Build config: `CMakeLists.txt`
- Entry point: `main.cpp` (minimal, just creates MainWindow)

📊 **Architecture Summary**:
```
User Input → Debounce Timer → Regex Scan → Stemming →
Data Structure Update → Error Detection → UI Update (Highlighting + Lists)
```

Last updated: 2025-11-29
