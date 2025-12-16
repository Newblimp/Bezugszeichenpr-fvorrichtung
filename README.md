# Bezugszeichenprüfvorrichtung

This is a small software I wrote to keep track of reference signs assigned to terms in patent applications. It will check if all the numbers are correct.

## Features

- **Bilingual Support**: German and English language analysis with switchable UI
- **Smart Stemming**: Handles plurals and different cases using the Oleander stemming library
- **Multi-word Terms**: Support for terms composed of two words (e.g., "zweites Lager" or "second bearing")
  - Manual mode: Right-click term → "Enable multi-word mode"
  - **Automatic mode (NEW)**: Auto-detects terms appearing with both "first" and "second" ordinal prefixes
- **Error Highlighting**:
  - Yellow: Wrong or missing reference numbers
  - Cyan: Wrong definite/indefinite articles
- **Error Navigation**: Jump through errors with next/previous buttons
- **Error Management**: Clear and restore errors individually or in bulk

# TODO

- image viewer for PDF patent drawings
- scan image using CRNN for finding reference signs
- ~~separate scanning and GUI thread~~
- ~~clearing errors in the textbox should clear errors in the list if applicable~~
- ~~ignore "Figure", "Figur", "Figuren" etc. when scanning for terms~~
- ~~ignore der, die, das, dem, und, and other short connectors when scanning for terms~~

# Changelog

__version 0.5 (Dec. 16, 2025)__
- automatic multi-word term detection for ordinal prefixes (first/second, erste/zweite)
- auto-detection respects manual user toggles (manual settings take precedence)
- language-specific auto-detection (German and English ordinals)
- 12 new unit tests for ordinal detection (145 total tests passing)

__version 0.4 (Dec. 04, 2025)__
- separate scanning and GUI thread

__version 0.3 (Dec. 02, 2025)__
- clearing errors in the textbox should clear errors in the list if applicable

__version 0.2 (Nov. 30, 2025)__
- ignore "Figure", "Figur", "Figuren" etc. when scanning for terms
- ignore der, die, das, dem, und, and other short connectors when scanning for terms

