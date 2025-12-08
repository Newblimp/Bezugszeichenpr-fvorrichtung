#pragma once

#ifdef HAVE_OPENCV

#include <wx/wx.h>
#include <wx/bitmap.h>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>
#include <regex>

/**
 * @brief OCR result structure for a single detected text region
 */
struct OCRResult {
    std::wstring text;           // Recognized text (e.g., "10", "12a")
    cv::Rect boundingBox;        // Location in original image
    float confidence;            // Recognition confidence (0-1)
};

/**
 * @brief PaddleOCR inference engine using OpenCV DNN
 *
 * Implements the PaddleOCR pipeline:
 * 1. Text detection (DBNet) - finds text regions
 * 2. Text recognition (CRNN) - recognizes characters
 * 3. Post-processing - filters for reference numbers
 *
 * Only returns strings starting with digits, optionally followed
 * by letters (e.g., "10", "12a", "15'").
 */
class OCREngine {
public:
    OCREngine();
    ~OCREngine();

    /**
     * @brief Initialize models from embedded compressed data
     * @return true on success
     */
    bool initialize();

    /**
     * @brief Check if engine is ready
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Process an image and extract reference numbers
     * @param bitmap Input image (wxBitmap from image viewer)
     * @return Vector of OCR results, filtered to reference numbers only
     */
    std::vector<OCRResult> processImage(const wxBitmap& bitmap);

    /**
     * @brief Get last error message
     */
    wxString getLastError() const { return m_lastError; }

private:
    // Model networks
    cv::dnn::Net m_detNet;       // Detection network (DBNet)
    cv::dnn::Net m_recNet;       // Recognition network (CRNN)

    // Character dictionary for recognition decoder
    std::vector<std::wstring> m_charDict;

    // Reference number pattern
    std::wregex m_refNumberPattern;

    // State
    bool m_initialized{false};
    wxString m_lastError;

    // Detection parameters
    static constexpr int DET_INPUT_SIZE = 960;      // Input size for detection
    static constexpr float DET_THRESHOLD = 0.3f;    // Detection threshold
    static constexpr float DET_BOX_THRESH = 0.5f;   // Box score threshold
    static constexpr float DET_UNCLIP_RATIO = 1.6f; // Unclip ratio for boxes

    // Recognition parameters
    static constexpr int REC_HEIGHT = 48;           // Recognition input height
    static constexpr int REC_WIDTH = 320;           // Recognition input width

    // Pipeline methods
    cv::Mat wxBitmapToCvMat(const wxBitmap& bitmap);
    std::vector<cv::RotatedRect> detectTextRegions(const cv::Mat& image);
    std::wstring recognizeText(const cv::Mat& textRegion);
    bool isReferenceNumber(const std::wstring& text);
    void initCharDictionary();

    // Pre/post processing helpers
    cv::Mat preprocessForDetection(const cv::Mat& image, float& ratio);
    std::vector<std::vector<cv::Point>> postprocessDetection(
        const cv::Mat& output, const cv::Size& origSize, float ratio);
    cv::Mat preprocessForRecognition(const cv::Mat& textCrop);
    std::wstring decodeRecognition(const cv::Mat& output);

    // Geometry helpers
    cv::Mat cropRotatedRect(const cv::Mat& image, const cv::RotatedRect& rect);
    std::vector<cv::Point> unclipPolygon(const std::vector<cv::Point>& polygon, float ratio);
};

#endif // HAVE_OPENCV
