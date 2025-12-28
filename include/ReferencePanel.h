#pragma once

#ifdef HAVE_OCR_SUPPORT

#include <wx/panel.h>
#include <wx/treelist.h>
#include <wx/stattext.h>
#include <wx/imaglist.h>
#include <memory>
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

    // Update display with new OCR results from all pages
    void setResults(const std::vector<OcrResult>& allResults);

    // Clear all results
    void clear();

    // Get currently selected reference index (-1 if none)
    int getSelectedReferenceIndex() const;

    // Get page index and reference for selected item
    bool getSelectedReference(size_t& outPageIndex, DetectedReference& outRef) const;

private:
    void setupUI();
    void updateStatistics(const std::vector<OcrResult>& allResults);
    void populateList(const std::vector<OcrResult>& allResults);
    void onItemSelected(wxTreeListEvent& event);
    void onItemActivated(wxTreeListEvent& event);

    // UI components
    wxTreeListCtrl* m_treeList;
    wxStaticText* m_statsLabel;

    // Image list for validation icons
    std::shared_ptr<wxImageList> m_imageList;

    // Reference list item (combines page index with reference and tree item)
    struct ReferenceListItem {
        size_t pageIndex;
        DetectedReference reference;
        wxTreeListItem treeItem;
    };

    // All references from all pages
    std::vector<ReferenceListItem> m_allReferences;

    // List columns
    enum {
        COL_BZ = 0,        // BZ number (with icon)
        COL_PAGE,          // Page number
        COL_CONFIDENCE     // Confidence %
    };
};

#endif // HAVE_OCR_SUPPORT
