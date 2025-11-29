#pragma once
#include <utility>
#include <vector>
#include "wx/richtext/richtextctrl.h"
#include "wx/stattext.h"

/**
 * @brief Generic error navigation handler
 *
 * Manages navigation through error positions in text with forward/backward buttons.
 * Reduces code duplication by providing a single implementation for all error types.
 */
class ErrorNavigator {
public:
    /**
     * @brief Navigate to next error in the list
     * @param positions Vector of (start, end) position pairs
     * @param currentIndex Reference to current selection index (-1 = none selected)
     * @param textCtrl Text control to update selection
     * @param label Label to update with "N/Total" text
     */
    static void selectNext(const std::vector<std::pair<int, int>>& positions,
                          int& currentIndex,
                          wxRichTextCtrl* textCtrl,
                          wxStaticText* label);

    /**
     * @brief Navigate to previous error in the list
     * @param positions Vector of (start, end) position pairs
     * @param currentIndex Reference to current selection index (-1 = none selected)
     * @param textCtrl Text control to update selection
     * @param label Label to update with "N/Total" text
     */
    static void selectPrevious(const std::vector<std::pair<int, int>>& positions,
                              int& currentIndex,
                              wxRichTextCtrl* textCtrl,
                              wxStaticText* label);

private:
    static void updateSelection(const std::vector<std::pair<int, int>>& positions,
                               int currentIndex,
                               wxRichTextCtrl* textCtrl,
                               wxStaticText* label);
};
