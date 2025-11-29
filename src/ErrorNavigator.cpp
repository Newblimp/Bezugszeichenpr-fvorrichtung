#include "ErrorNavigator.h"

void ErrorNavigator::selectNext(const std::vector<std::pair<int, int>>& positions,
                                int& currentIndex,
                                wxRichTextCtrl* textCtrl,
                                wxStaticText* label) {
    currentIndex += 1;
    if (currentIndex >= static_cast<int>(positions.size()) || currentIndex < 0) {
        currentIndex = 0;
    }
    updateSelection(positions, currentIndex, textCtrl, label);
}

void ErrorNavigator::selectPrevious(const std::vector<std::pair<int, int>>& positions,
                                   int& currentIndex,
                                   wxRichTextCtrl* textCtrl,
                                   wxStaticText* label) {
    currentIndex -= 1;
    if (currentIndex >= static_cast<int>(positions.size()) || currentIndex < 0) {
        currentIndex = positions.size() - 1;
    }
    updateSelection(positions, currentIndex, textCtrl, label);
}

void ErrorNavigator::updateSelection(const std::vector<std::pair<int, int>>& positions,
                                     int currentIndex,
                                     wxRichTextCtrl* textCtrl,
                                     wxStaticText* label) {
    if (!positions.empty() && currentIndex >= 0 && currentIndex < static_cast<int>(positions.size())) {
        const auto& pos = positions[currentIndex];
        textCtrl->SetSelection(pos.first, pos.second);
        textCtrl->ShowPosition(pos.first);
        label->SetLabel(
            std::to_wstring(currentIndex + 1) + L"/" +
            std::to_wstring(positions.size()) + L"\t");
    }
}
