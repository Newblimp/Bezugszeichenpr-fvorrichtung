#ifdef HAVE_OCR_SUPPORT

#include "ReferencePanel.h"
#include "utils.h"  // For BZComparatorForMap
#include <wx/sizer.h>
#include <algorithm>

wxDEFINE_EVENT(EVT_REFERENCE_SELECTED, wxCommandEvent);

ReferencePanel::ReferencePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    setupUI();
}

void ReferencePanel::setupUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Statistics label
    m_statsLabel = new wxStaticText(this, wxID_ANY, "No OCR results");
    mainSizer->Add(m_statsLabel, 0, wxALL | wxEXPAND, 5);

    // List view
    m_listView = new wxListView(this, wxID_ANY,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL);

    // Configure columns
    m_listView->AppendColumn("BZ", wxLIST_FORMAT_LEFT, 80);
    m_listView->AppendColumn("Confidence", wxLIST_FORMAT_CENTER, 90);
    m_listView->AppendColumn("Status", wxLIST_FORMAT_LEFT, 120);
    m_listView->AppendColumn("Position", wxLIST_FORMAT_LEFT, 100);

    mainSizer->Add(m_listView, 1, wxALL | wxEXPAND, 5);

    // Bindings
    m_listView->Bind(wxEVT_LIST_ITEM_SELECTED,
                     &ReferencePanel::onItemSelected, this);
    m_listView->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                     &ReferencePanel::onItemActivated, this);

    SetSizer(mainSizer);
}

void ReferencePanel::setResults(const OcrResult& results) {
    m_currentResults = results;
    updateStatistics(results);
    populateList(results);
}

void ReferencePanel::clear() {
    m_listView->DeleteAllItems();
    m_statsLabel->SetLabel("No OCR results");
    m_currentResults = OcrResult();
}

void ReferencePanel::updateStatistics(const OcrResult& results) {
    size_t total = results.references.size();
    size_t valid = 0;
    size_t errors = 0;

    for (const auto& ref : results.references) {
        if (ref.status == DetectedReference::ValidationStatus::VALID) {
            valid++;
        } else if (ref.status == DetectedReference::ValidationStatus::MISSING_IN_TEXT) {
            errors++;
        }
    }

    wxString stats = wxString::Format(
        "Detected: %zu | Valid: %zu | Errors: %zu | Time: %.1fms",
        total, valid, errors, results.totalTimeMs
    );

    m_statsLabel->SetLabel(stats);
}

void ReferencePanel::populateList(const OcrResult& results) {
    m_listView->DeleteAllItems();

    // Sort references by BZ number
    std::vector<DetectedReference> sorted = results.references;
    std::sort(sorted.begin(), sorted.end(),
              [](const DetectedReference& a, const DetectedReference& b) {
                  return BZComparatorForMap()(a.text, b.text);
              });

    // Populate list
    long idx = 0;
    for (const auto& ref : sorted) {
        long item = m_listView->InsertItem(idx, ref.text);

        // Confidence
        m_listView->SetItem(item, COL_CONFIDENCE,
                           wxString::Format("%.1f%%", ref.confidence * 100));

        // Status
        wxString statusText;
        wxColour statusColor;
        switch (ref.status) {
            case DetectedReference::ValidationStatus::VALID:
                statusText = "Valid";
                statusColor = wxColour(0, 180, 0);  // Green
                break;
            case DetectedReference::ValidationStatus::MISSING_IN_TEXT:
                statusText = "Missing in text";
                statusColor = wxColour(255, 0, 0);  // Red
                break;
            case DetectedReference::ValidationStatus::NOT_VALIDATED:
                statusText = "Not validated";
                statusColor = wxColour(128, 128, 128);  // Gray
                break;
            default:
                statusText = "Unknown";
                statusColor = wxColour(0, 0, 0);
        }
        m_listView->SetItem(item, COL_STATUS, statusText);
        m_listView->SetItemTextColour(item, statusColor);

        // Position
        m_listView->SetItem(item, COL_POSITION,
                           wxString::Format("(%d, %d)", ref.x, ref.y));

        idx++;
    }
}

void ReferencePanel::onItemSelected(wxListEvent& event) {
    // Fire event to notify parent
    wxCommandEvent evt(EVT_REFERENCE_SELECTED, GetId());
    evt.SetInt(event.GetIndex());
    wxPostEvent(this, evt);
}

void ReferencePanel::onItemActivated(wxListEvent& event) {
    // Double-click: zoom to reference on canvas
    onItemSelected(event);
}

int ReferencePanel::getSelectedReferenceIndex() const {
    return m_listView->GetFirstSelected();
}

#endif // HAVE_OCR_SUPPORT
