#ifdef HAVE_OCR_SUPPORT

#include "ReferencePanel.h"
#include "utils.h"  // For BZComparatorForMap
#include "../img/check_16.xpm"
#include "../img/warning_16.xpm"
#include <wx/sizer.h>
#include <algorithm>

wxDEFINE_EVENT(EVT_REFERENCE_SELECTED, wxCommandEvent);

ReferencePanel::ReferencePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    // Initialize icon image list (match MainWindow pattern)
    m_imageList = std::make_shared<wxImageList>(16, 16, false, 0);
    wxBitmap check(check_16_xpm);
    wxBitmap warning(warning_16_xpm);
    m_imageList->Add(check);     // Index 0 = VALID
    m_imageList->Add(warning);   // Index 1 = Invalid

    setupUI();
}

void ReferencePanel::setupUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Statistics label
    m_statsLabel = new wxStaticText(this, wxID_ANY, "No OCR results");
    mainSizer->Add(m_statsLabel, 0, wxALL | wxEXPAND, 5);

    // Tree list control (like MainWindow)
    m_treeList = new wxTreeListCtrl(this, wxID_ANY,
                                     wxDefaultPosition, wxDefaultSize);

    // Configure columns: BZ (with icon) | Page | Confidence
    m_treeList->AppendColumn("BZ", 100);          // BZ number with icon
    m_treeList->AppendColumn("Page", 60);         // Page
    m_treeList->AppendColumn("Confidence", 100);  // Confidence

    // Attach image list
    m_treeList->SetImageList(m_imageList.get());

    mainSizer->Add(m_treeList, 1, wxALL | wxEXPAND, 5);

    // Bindings
    m_treeList->Bind(wxEVT_TREELIST_SELECTION_CHANGED,
                     &ReferencePanel::onItemSelected, this);
    m_treeList->Bind(wxEVT_TREELIST_ITEM_ACTIVATED,
                     &ReferencePanel::onItemActivated, this);

    SetSizer(mainSizer);
}

void ReferencePanel::setResults(const std::vector<OcrResult>& allResults) {
    m_allReferences.clear();
    updateStatistics(allResults);
    populateList(allResults);
}

void ReferencePanel::clear() {
    m_treeList->DeleteAllItems();
    m_statsLabel->SetLabel("No OCR results");
    m_allReferences.clear();
}

void ReferencePanel::updateStatistics(const std::vector<OcrResult>& allResults) {
    size_t total = 0;
    size_t valid = 0;
    size_t errors = 0;
    double totalTimeMs = 0.0;

    for (const auto& result : allResults) {
        if (!result.success) continue;

        total += result.references.size();
        totalTimeMs += result.totalTimeMs;

        for (const auto& ref : result.references) {
            if (ref.status == DetectedReference::ValidationStatus::VALID) {
                valid++;
            } else if (ref.status == DetectedReference::ValidationStatus::MISSING_IN_TEXT) {
                errors++;
            }
        }
    }

    wxString stats = wxString::Format(
        "Detected: %zu | Valid: %zu | Errors: %zu | Total Time: %.1fms",
        total, valid, errors, totalTimeMs
    );

    m_statsLabel->SetLabel(stats);
}

void ReferencePanel::populateList(const std::vector<OcrResult>& allResults) {
    m_treeList->DeleteAllItems();
    m_allReferences.clear();

    // Collect all references from all pages
    for (const auto& result : allResults) {
        if (!result.success) continue;

        for (const auto& ref : result.references) {
            ReferenceListItem item;
            item.pageIndex = result.pageIndex;
            item.reference = ref;
            m_allReferences.push_back(item);
        }
    }

    // Sort references by BZ number
    std::sort(m_allReferences.begin(), m_allReferences.end(),
              [](const ReferenceListItem& a, const ReferenceListItem& b) {
                  return BZComparatorForMap()(a.reference.text, b.reference.text);
              });

    // Populate tree list
    for (auto& item : m_allReferences) {
        const auto& ref = item.reference;

        // Determine icon based on validation status
        int iconIndex = (ref.status == DetectedReference::ValidationStatus::VALID) ? 0 : 1;

        // Append item with icon and BZ text in first column
        wxTreeListItem treeItem = m_treeList->AppendItem(
            m_treeList->GetRootItem(),
            ref.text,  // BZ text
            iconIndex, // Icon (check or warning)
            iconIndex  // Selected icon (same)
        );

        // Store tree item reference for selection
        item.treeItem = treeItem;

        // Page column (1-indexed)
        m_treeList->SetItemText(treeItem, COL_PAGE,
                                wxString::Format("%zu", item.pageIndex + 1));

        // Confidence column
        m_treeList->SetItemText(treeItem, COL_CONFIDENCE,
                                wxString::Format("%.1f%%", ref.confidence * 100));
    }
}

void ReferencePanel::onItemSelected(wxTreeListEvent& event) {
    // Fire event to notify parent
    wxCommandEvent evt(EVT_REFERENCE_SELECTED, GetId());
    wxPostEvent(this, evt);
}

void ReferencePanel::onItemActivated(wxTreeListEvent& event) {
    // Double-click: zoom to reference on canvas
    // Event already processed, just delegate
    wxCommandEvent evt(EVT_REFERENCE_SELECTED, GetId());
    wxPostEvent(this, evt);
}

int ReferencePanel::getSelectedReferenceIndex() const {
    wxTreeListItem selected = m_treeList->GetSelection();
    if (!selected.IsOk()) {
        return -1;
    }

    // Find index in m_allReferences
    for (size_t i = 0; i < m_allReferences.size(); ++i) {
        if (m_allReferences[i].treeItem == selected) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool ReferencePanel::getSelectedReference(size_t& outPageIndex, DetectedReference& outRef) const {
    wxTreeListItem selected = m_treeList->GetSelection();
    if (!selected.IsOk()) {
        return false;
    }

    // Find matching reference
    for (const auto& item : m_allReferences) {
        if (item.treeItem == selected) {
            outPageIndex = item.pageIndex;
            outRef = item.reference;
            return true;
        }
    }
    return false;
}

#endif // HAVE_OCR_SUPPORT
