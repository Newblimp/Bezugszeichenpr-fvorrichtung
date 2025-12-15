#pragma once
#include "GermanTextAnalyzer.h"
#include "EnglishTextAnalyzer.h"
#include "RE2RegexHelper.h"
#include "utils.h"
#ifdef HAVE_MUPDF
#include "PDFLoader.h"
#endif
#ifdef HAVE_OPENCV
#include "OCREngine.h"
#endif
#include "wx/notebook.h"
#include "wx/richtext/richtextctrl.h"
#include "wx/textctrl.h"
#include "wx/timer.h"
#include "wx/treelist.h"
#include "wx/splitter.h"
#include "wx/scrolwin.h"
#include <map>
#include <memory>
#include <re2/re2.h>
#include <wx/dataview.h>
#include <wx/listctrl.h>
#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

// Custom scrolled window for rendering images with zoom and pan
class ImageViewerPanel : public wxScrolledWindow {
public:
  ImageViewerPanel(wxWindow* parent, wxWindowID id = wxID_ANY)
    : wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetScrollRate(1, 1);  // Pixel-level scrolling
    Bind(wxEVT_PAINT, &ImageViewerPanel::onPaint, this);
    Bind(wxEVT_LEFT_DOWN, &ImageViewerPanel::onMouseDown, this);
    Bind(wxEVT_LEFT_UP, &ImageViewerPanel::onMouseUp, this);
    Bind(wxEVT_MOTION, &ImageViewerPanel::onMouseMove, this);
    Bind(wxEVT_MOUSEWHEEL, &ImageViewerPanel::onMouseWheel, this);
    SetCursor(wxCursor(wxCURSOR_HAND));
  }

  ~ImageViewerPanel() {
    if (HasCapture()) {
      ReleaseMouse();
    }
  }

  void SetBitmap(const wxBitmap& bitmap) {
    m_bitmap = bitmap;
    UpdateVirtualSize();
    Refresh();
  }

  void SetZoom(double zoom) {
    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 5.0) zoom = 5.0;
    m_zoom = zoom;
    UpdateVirtualSize();
    Refresh();
  }

  double GetZoom() const {
    return m_zoom;
  }

  wxSize GetImageSize() const {
    if (m_bitmap.IsOk()) {
      return m_bitmap.GetSize();
    }
    return wxSize(0, 0);
  }

  // Convert screen coordinates to image coordinates
  wxPoint ScreenToImage(const wxPoint& screenPos) {
    int scrollX, scrollY;
    GetViewStart(&scrollX, &scrollY);
    return wxPoint((screenPos.x + scrollX) / m_zoom, (screenPos.y + scrollY) / m_zoom);
  }

private:
  void UpdateVirtualSize() {
    if (m_bitmap.IsOk()) {
      int w = m_bitmap.GetWidth() * m_zoom;
      int h = m_bitmap.GetHeight() * m_zoom;
      SetVirtualSize(w, h);
    }
  }

  void onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    DoPrepareDC(dc);  // Apply scroll offset

    // Clear background
    dc.SetBackground(*wxLIGHT_GREY_BRUSH);
    dc.Clear();

    if (!m_bitmap.IsOk()) {
      return;
    }

    // Apply zoom
    dc.SetUserScale(m_zoom, m_zoom);

    // Draw bitmap at origin (DoPrepareDC handles scroll offset)
    dc.DrawBitmap(m_bitmap, 0, 0, false);
  }

  void onMouseDown(wxMouseEvent& event) {
    m_dragging = true;
    m_dragStart = event.GetPosition();
    CaptureMouse();
  }

  void onMouseUp(wxMouseEvent& event) {
    if (m_dragging) {
      m_dragging = false;
      if (HasCapture()) {
        ReleaseMouse();
      }
    }
  }

  void onMouseMove(wxMouseEvent& event) {
    if (m_dragging && event.Dragging()) {
      wxPoint currentPos = event.GetPosition();
      wxPoint delta = currentPos - m_dragStart;

      int scrollX, scrollY;
      GetViewStart(&scrollX, &scrollY);
      Scroll(scrollX - delta.x, scrollY - delta.y);

      m_dragStart = currentPos;
    }
  }

  void onMouseWheel(wxMouseEvent& event) {
    if (!m_bitmap.IsOk()) {
      event.Skip();
      return;
    }

    // Get mouse position in screen coordinates
    wxPoint mousePos = event.GetPosition();

    // Convert to image coordinates before zoom
    wxPoint imagePosBefore = ScreenToImage(mousePos);

    // Change zoom level
    double oldZoom = m_zoom;
    int rotation = event.GetWheelRotation();
    if (rotation > 0) {
      m_zoom *= 1.1;
    } else {
      m_zoom /= 1.1;
    }

    // Clamp zoom
    if (m_zoom > 5.0) m_zoom = 5.0;
    if (m_zoom < 0.1) m_zoom = 0.1;

    // Update virtual size
    UpdateVirtualSize();

    // Calculate new scroll position to keep image point under mouse
    int newScrollX = imagePosBefore.x * m_zoom - mousePos.x;
    int newScrollY = imagePosBefore.y * m_zoom - mousePos.y;

    Scroll(newScrollX, newScrollY);
    Refresh();

    // Notify parent of zoom change
    wxCommandEvent zoomEvent(wxEVT_COMMAND_TEXT_UPDATED, GetId());
    zoomEvent.SetString(wxString::Format("%.1f", m_zoom * 100));
    GetParent()->GetEventHandler()->ProcessEvent(zoomEvent);
  }

  wxBitmap m_bitmap;
  double m_zoom{1.0};
  bool m_dragging{false};
  wxPoint m_dragStart;
};

class MainWindow : public wxFrame {
public:
  MainWindow();
  ~MainWindow();

private:
  // Setup methods
  void setupUi();
  void setupBindings();
  void loadIcons();

  // Core scanning logic
  void scanText(wxTimerEvent &event);
  void scanTextBackground();
  void updateUIAfterScan();
  void debounceFunc(wxCommandEvent &event);

  // Display methods
  void fillListTree();
  void fillBzList();
  bool isUniquelyAssigned(const std::wstring &bz);

  // Error detection
  void findUnnumberedWords();
  void checkArticleUsage();
  void highlightConflicts();

  // Navigation methods
  void selectNextAllError(wxCommandEvent &event);
  void selectPreviousAllError(wxCommandEvent &event);
  void selectNextNoNumber(wxCommandEvent &event);
  void selectPreviousNoNumber(wxCommandEvent &event);
  void selectNextWrongNumber(wxCommandEvent &event);
  void selectPreviousWrongNumber(wxCommandEvent &event);
  void selectNextSplitNumber(wxCommandEvent &event);
  void selectPreviousSplitNumber(wxCommandEvent &event);
  void selectNextWrongArticle(wxCommandEvent &event);
  void selectPreviousWrongArticle(wxCommandEvent &event);

  // Context menu handling
  void onTreeListContextMenu(wxTreeListEvent &event);
  void onTreeListItemActivated(wxTreeListEvent &event);
  void onTextRightClick(wxMouseEvent &event);
  void clearTextError(size_t start, size_t end);
  bool isPositionCleared(size_t start, size_t end) const;

  // Menu handlers for restoring errors
  void onRestoreTextboxErrors(wxCommandEvent &event);
  void onRestoreOverviewErrors(wxCommandEvent &event);
  void onRestoreAllErrors(wxCommandEvent &event);
  void toggleMultiWordTerm(const std::wstring &baseStem);
  void clearError(const std::wstring &bz);

  // Image handling
  void onOpenImage(wxCommandEvent &event);
  void loadImage(const wxString &filePath);
  void onPreviousImage(wxCommandEvent &event);
  void onNextImage(wxCommandEvent &event);
  void updateImageDisplay();
  void updateImageNavigationButtons();
  void onImagePanelResize(wxSizeEvent &event);

  // Zoom handling
  void onZoomIn(wxCommandEvent &event);
  void onZoomOut(wxCommandEvent &event);
  void onZoomReset(wxCommandEvent &event);
  void onImageViewerZoomChanged(wxCommandEvent &event);
  void updateImageWithZoom();

#ifdef HAVE_MUPDF
  // PDF handling (reuses image infrastructure)
  void loadPDF(const wxString &filePath);
#endif

#ifdef HAVE_OPENCV
  // OCR handling
  void onScanOCR(wxCommandEvent &event);
  void updateOCRResults(const std::vector<OCRResult>& results);
  void onOCRThreadComplete(wxThreadEvent &event);
  void ocrProcessingThread(const wxBitmap& image);
#endif

    // RE2 regex patterns (optimized for performance)
    // Single word + number: captures (word)(number)
    // Pattern: word followed by whitespace and number
    re2::RE2 m_singleWordRegex;

    // Two words + number: captures (word1)(word2)(number)
    // Pattern: word followed by word followed by number
    re2::RE2 m_twoWordRegex;

    // Single word (for finding unnumbered references)
    re2::RE2 m_wordRegex;

public:
  // Static text analyzers for different languages
  // Static ensures single instance with persistent cache across all scans
  static GermanTextAnalyzer s_germanAnalyzer;
  static EnglishTextAnalyzer s_englishAnalyzer;
  static bool s_useGerman; // true = German, false = English

  // Helper methods to access current analyzer based on language selection
  // These are public to allow TextScanner and ErrorDetectorHelper to use them
  static StemVector createCurrentStemVector(std::wstring word);
  static StemVector createCurrentMultiWordStemVector(std::wstring firstWord, std::wstring secondWord);
  static bool isCurrentMultiWordBase(std::wstring word, const std::unordered_set<std::wstring>& multiWordBaseStems);
  static bool isCurrentIndefiniteArticle(const std::wstring& word);
  static bool isCurrentDefiniteArticle(const std::wstring& word);
  static bool isCurrentIgnoredWord(const std::wstring& word);
  static std::pair<std::wstring, size_t> findCurrentPrecedingWord(const std::wstring& text, size_t pos);

private:
  std::wstring m_fullText;

  // Text styles
  wxTextAttr m_neutralStyle;
  wxTextAttr m_warningStyle;
  wxTextAttr m_articleWarningStyle;

  // Debounce timer for text changes
  wxTimer m_debounceTimer;

  // Thread synchronization for background scanning
  std::jthread m_scanThread;
  std::mutex m_dataMutex;
  std::atomic<bool> m_cancelScan{false};

  // Main data structure: BZ -> set of StemVectors
  // Example: "10" -> {{"lager"}, {"zweit", "lager"}}
  std::map<std::wstring, std::unordered_set<StemVector, StemVectorHash>,
           BZComparatorForMap>
      m_bzToStems;

  // Reverse mapping: StemVector -> set of BZs
  // Example: {"zweit", "lager"} -> {"12"}
  std::unordered_map<StemVector, std::unordered_set<std::wstring>,
                     StemVectorHash>
      m_stemToBz;

  // Original (unstemmed) words for display
  // BZ -> set of original word strings
  std::unordered_map<std::wstring, std::unordered_set<std::wstring>>
      m_bzToOriginalWords;

  // Position tracking for highlighting and navigation
  // BZ -> list of (start, length) pairs
  std::unordered_map<std::wstring, std::vector<std::pair<size_t, size_t>>>
      m_bzToPositions;

  // StemVector -> list of (start, length) pairs
  std::unordered_map<StemVector, std::vector<std::pair<size_t, size_t>>,
                     StemVectorHash>
      m_stemToPositions;

  // keeping track of the position of the cursor when browsing occurences
  std::unordered_map<std::wstring, int> m_bzCurrentOccurrence;

  // Set of base word STEMS that should trigger multi-word matching
  // Example: if "lager" is in this set, "erstes Lager 10" becomes {"erst",
  // "lager"}
  std::unordered_set<std::wstring> m_multiWordBaseStems;

  // Set of BZ numbers whose errors have been cleared/ignored by user
  std::unordered_set<std::wstring> m_clearedErrors;

  // Track cleared text positions (for right-click clear on highlighted text)
  std::set<std::pair<size_t, size_t>> m_clearedTextPositions;

  // All unique stems encountered
  // std::unordered_set<StemVector, StemVectorHash> m_allStems;

  // UI components
  wxNotebook *m_notebookList;
  wxRichTextCtrl *m_textBox;
  wxRadioBox *m_languageSelector;
  void onLanguageChanged(wxCommandEvent &event);
  std::shared_ptr<wxRichTextCtrl> m_bzList;
  std::shared_ptr<wxImageList> m_imageList;
  std::shared_ptr<wxTreeListCtrl> m_treeList;

  // Image viewer components
  wxSplitterWindow *m_splitter;
  std::shared_ptr<wxScrolledWindow> m_imagePanel;
  std::shared_ptr<ImageViewerPanel> m_imageViewer;
  std::shared_ptr<wxStaticText> m_imageInfoText;

  // Image navigation
  std::shared_ptr<wxButton> m_buttonPreviousImage;
  std::shared_ptr<wxButton> m_buttonNextImage;
  std::shared_ptr<wxStaticText> m_imageNavigationLabel;

  // Zoom controls
  std::shared_ptr<wxButton> m_buttonZoomIn;
  std::shared_ptr<wxButton> m_buttonZoomOut;
  std::shared_ptr<wxButton> m_buttonZoomReset;
  std::shared_ptr<wxStaticText> m_zoomLabel;
  double m_zoomLevel{1.0};  // 1.0 = 100%
  bool m_updatingImageDisplay{false};  // Prevent resize event loops

  std::vector<wxBitmap> m_loadedImages;
  std::vector<wxString> m_imagePaths;
  int m_currentImageIndex{-1};

  // Right panel horizontal splitter (contains Image and OCR side-by-side)
  wxSplitterWindow* m_rightNotebook;  // Name kept for compatibility

#ifdef HAVE_OPENCV
  // OCR components
  std::unique_ptr<OCREngine> m_ocrEngine;
  std::shared_ptr<wxPanel> m_ocrPanel;
  std::shared_ptr<wxListCtrl> m_ocrResultsList;
  std::shared_ptr<wxButton> m_buttonScanOCR;
  std::shared_ptr<wxStaticText> m_ocrStatusLabel;
  std::vector<OCRResult> m_ocrResults;

  // OCR threading
  std::unique_ptr<std::thread> m_ocrThread;
  std::mutex m_ocrMutex;
  std::vector<OCRResult> m_ocrThreadResults;
  wxBitmap m_ocrThreadDebugImage;

  // OCR loading animation
  wxTimer m_ocrLoadingTimer{this, wxID_ANY};
  int m_ocrLoadingFrame{0};
  void onOCRLoadingAnimation(wxTimerEvent &event);
#endif

  // Navigation buttons
  std::shared_ptr<wxButton> m_buttonForwardAllErrors;
  std::shared_ptr<wxButton> m_buttonBackwardAllErrors;
  std::shared_ptr<wxButton> m_buttonForwardNoNumber;
  std::shared_ptr<wxButton> m_buttonBackwardNoNumber;
  std::shared_ptr<wxButton> m_buttonForwardWrongNumber;
  std::shared_ptr<wxButton> m_buttonBackwardWrongNumber;
  std::shared_ptr<wxButton> m_buttonForwardSplitNumber;
  std::shared_ptr<wxButton> m_buttonBackwardSplitNumber;
  std::shared_ptr<wxButton> m_buttonForwardWrongArticle;
  std::shared_ptr<wxButton> m_buttonBackwardWrongArticle;

  // Error position lists: stores (start, end) position pairs
  std::vector<std::pair<int, int>> m_allErrorsPositions;
  int m_allErrorsSelected{-1};
  std::shared_ptr<wxStaticText> m_allErrorsLabel;

  std::vector<std::pair<int, int>> m_noNumberPositions;
  int m_noNumberSelected{-1};
  std::shared_ptr<wxStaticText> m_noNumberLabel;

  std::vector<std::pair<int, int>> m_wrongNumberPositions;
  int m_wrongNumberSelected{-1};
  std::shared_ptr<wxStaticText> m_wrongNumberLabel;

  std::vector<std::pair<int, int>> m_splitNumberPositions;
  int m_splitNumberSelected{-1};
  std::shared_ptr<wxStaticText> m_splitNumberLabel;

  std::vector<std::pair<int, int>> m_wrongArticlePositions;
  int m_wrongArticleSelected{-1};
  std::shared_ptr<wxStaticText> m_wrongArticleLabel;
};
