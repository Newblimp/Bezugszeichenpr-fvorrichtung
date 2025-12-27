#pragma once

#ifdef HAVE_OCR_SUPPORT

#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include "IOcrEngine.h"

// Event for when a reference is selected
wxDECLARE_EVENT(EVT_REFERENCE_SELECTED, wxCommandEvent);

/**
 * @brief Panel displaying OCR-detected reference numbers with validation status
 *
 * Features:
 * - List view of detected references (sorted by BZ number)
 * - Columns: BZ, Confidence, Status, Position
 * - Color-coded by validation status (green=valid, red=error, gray=not validated)
 * - Click to highlight bounding box on canvas
 * - Summary statistics panel
 */
class ReferencePanel : public wxPanel {
public:
    explicit ReferencePanel(wxWindow* parent);

    // Update display with new OCR results
    void setResults(const OcrResult& results);

    // Clear all results
    void clear();

    // Get currently selected reference index (-1 if none)
    int getSelectedReferenceIndex() const;

private:
    void setupUI();
    void updateStatistics(const OcrResult& results);
    void populateList(const OcrResult& results);
    void onItemSelected(wxListEvent& event);
    void onItemActivated(wxListEvent& event);

    // UI components
    wxListView* m_listView;
    wxStaticText* m_statsLabel;

    // Current results (cached for selection)
    OcrResult m_currentResults;

    // List columns
    enum {
        COL_BZ = 0,
        COL_CONFIDENCE,
        COL_STATUS,
        COL_POSITION
    };
};

#endif // HAVE_OCR_SUPPORT
