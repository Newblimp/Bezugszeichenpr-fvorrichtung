#pragma once

#ifdef HAVE_OPENCV

#include <wx/wx.h>
#include <wx/bitmap.h>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <regex>
#include <memory>

/**
 * @brief OCR result structure for a single detected text region
 */
struct OCRResult {
    std::wstring text;           // Recognized text (e.g., "10", "12a")
    cv::Rect boundingBox;        // Location in original image
    float confidence;            // Recognition confidence (0-1)
};

/**
 * @brief OCR Engine using YOLOv11n detection + TrOCR recognition
 *
 * Pipeline:
 * 1. Text detection (YOLOv11n via OpenCV DNN) - finds text regions
 * 2. Text recognition (TrOCR via ONNXRuntime) - recognizes handwritten text
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
     * @brief Initialize models from embedded data
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
     * @brief Get debug image with detection boxes drawn
     * @return wxBitmap with colored rectangles:
     *         - Green: Accepted reference numbers
     *         - Red: Detected but filtered out (not reference numbers)
     *         - Yellow: Detection boxes before recognition
     */
    wxBitmap getDebugImage() const { return m_debugImage; }

    /**
     * @brief Get last error message
     */
    wxString getLastError() const { return m_lastError; }

private:
    // Detection network (YOLOv11n via OpenCV DNN)
    cv::dnn::Net m_detNet;

    // TrOCR models (via ONNXRuntime)
    std::unique_ptr<Ort::Env> m_ortEnv;
    std::unique_ptr<Ort::Session> m_encoderSession;
    std::unique_ptr<Ort::Session> m_decoderSession;
    Ort::SessionOptions m_sessionOptions;

    // Vocabulary for token decoding (token ID -> string)
    std::vector<std::wstring> m_vocabulary;

    // Reference number pattern
    std::wregex m_refNumberPattern;

    // State
    bool m_initialized{false};
    wxString m_lastError;
    wxBitmap m_debugImage;

    // Detection parameters (YOLOv11s)
    static constexpr int DET_INPUT_SIZE = 640;        // YOLOv11 input size
    static constexpr float DET_CONF_THRESHOLD = 0.25f; // Confidence threshold
    static constexpr float DET_NMS_THRESHOLD = 0.45f;  // NMS IoU threshold

    // TrOCR parameters
    static constexpr int TROCR_IMAGE_SIZE = 384;      // Input image size
    static constexpr int TROCR_MAX_LENGTH = 20;       // Max output tokens
    static constexpr int TROCR_START_TOKEN = 2;       // decoder_start_token_id
    static constexpr int TROCR_END_TOKEN = 2;         // eos_token_id
    static constexpr int TROCR_PAD_TOKEN = 1;         // pad_token_id

    // Pipeline methods
    cv::Mat wxBitmapToCvMat(const wxBitmap& bitmap);
    wxBitmap cvMatToWxBitmap(const cv::Mat& mat);
    std::wstring recognizeText(const cv::Mat& textRegion);
    bool isReferenceNumber(const std::wstring& text);
    void loadVocabulary(const std::vector<unsigned char>& vocabData);

    // Detection helpers (YOLOv11 via OpenCV DNN)
    cv::Mat preprocessForDetection(const cv::Mat& image, float& scaleX, float& scaleY,
                                   int& padX, int& padY);
    std::vector<cv::Rect> postprocessDetection(
        const cv::Mat& output, const cv::Size& origSize,
        float scaleX, float scaleY, int padX, int padY);

    // TrOCR recognition helpers (ONNXRuntime)
    std::vector<float> preprocessForTrOCR(const cv::Mat& textCrop);
    std::wstring runTrOCRInference(const std::vector<float>& pixelValues);
    std::wstring decodeTokens(const std::vector<int64_t>& tokens);

};

#endif // HAVE_OPENCV
