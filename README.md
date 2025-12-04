# Bezugszeichenprüfvorrichtung

This is a small software I wrote to keep track of reference signs assigned to terms in patent applications. It will check if all the numbers are correct.
The software deals with plurals and different cases using the Oleander stemming library.
multi term words are also supported for terms composed with two words (right click -> enable multi-word mode)

Wrong definite/indefinite articles are highlighted cyan, wrong reference signs yellow

# TODO

- image viewer for PDF patent drawings
- scan image using CRNN for finding reference signs
- ~~separate scanning and GUI thread~~
- ~~clearing errors in the textbox should clear errors in the list if applicable~~
- ~~ignore "Figure", "Figur", "Figuren" etc. when scanning for terms~~
- ~~ignore der, die, das, dem, und, and other short connectors when scanning for terms~~

# Changelog
__version 0.4 (Dec. 04, 2025)__
- separate scanning and GUI thread

__version 0.3 (Dec. 02, 2025)__
- clearing errors in the textbox should clear errors in the list if applicable

__version 0.2 (Nov. 30, 2025)__
- ignore "Figure", "Figur", "Figuren" etc. when scanning for terms
- ignore der, die, das, dem, und, and other short connectors when scanning for terms

