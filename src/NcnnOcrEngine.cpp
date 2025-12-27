#ifdef HAVE_OCR_SUPPORT

#include "NcnnOcrEngine.h"
#include "ocr/yolo_detector.h"
#include "ocr/svtr_ncnn.h"
#include "ocr/ctc_decoder.h"
#include "ocr/models/yolo_model_param.h"
#include "ocr/models/yolo_model_weights.h"
#include "ocr/models/svtr_model_param.h"
#include "ocr/models/svtr_model_weights.h"
#include "ocr/models/decoder_weight_data.h"
#include "ocr/models/decoder_bias_data.h"
#include <wx/image.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <algorithm>
#include <iostream>

// Forward declarations for helper functions
static cv::Mat wxImageToCvMat(const wxImage& wxImg);
static cv::Rect expandBbox(const cv::Rect& bbox, int padding, const cv::Size& imgSize);

// PIMPL struct holds ncnn components
struct NcnnOcrEngine::Impl {
    std::unique_ptr<YOLODetector> yolo;
    std::unique_ptr<SVTRncnn> svtr;
    std::unique_ptr<CTCDecoder> decoder;
};

NcnnOcrEngine::NcnnOcrEngine() : m_impl(std::make_unique<Impl>()) {
    // Load YOLO from embedded memory
    m_impl->yolo = std::make_unique<YOLODetector>(
        yolo_model_param, yolo_model_param_len,
        yolo_model_weights, yolo_model_weights_len
    );

    // Load SVTR from embedded memory
    m_impl->svtr = std::make_unique<SVTRncnn>(
        svtr_model_param, svtr_model_param_len,
        svtr_model_weights, svtr_model_weights_len
    );

    // Load CTC decoder from embedded memory
    m_impl->decoder = std::make_unique<CTCDecoder>(
        decoder_weight_data, decoder_weight_data_len,
        decoder_bias_data, decoder_bias_data_len
    );
}

NcnnOcrEngine::~NcnnOcrEngine() = default;

OcrResult NcnnOcrEngine::processImage(const wxImage& image,
                                     size_t pageIndex,
                                     const std::string& sourcePath) {
    OcrResult result;
    result.pageIndex = pageIndex;
    result.sourcePath = sourcePath;

    auto totalStart = std::chrono::high_resolution_clock::now();

    try {
        // Convert wxImage to cv::Mat
        cv::Mat cvImage = wxImageToCvMat(image);
        if (cvImage.empty()) {
            result.errorMessage = "Failed to convert image";
            return result;
        }

        // 1. Run YOLO detection
        auto detStart = std::chrono::high_resolution_clock::now();
        std::vector<Detection> detections = m_impl->yolo->detect(
            cvImage, m_confThreshold, m_nmsThreshold
        );
        auto detEnd = std::chrono::high_resolution_clock::now();
        result.detectionTimeMs =
            std::chrono::duration<double, std::milli>(detEnd - detStart).count();

        std::cout << "[OCR Debug] YOLO found " << detections.size() << " detections" << std::endl;

        // 2. Process each detected region
        auto recStart = std::chrono::high_resolution_clock::now();
        int detection_idx = 0;
        for (const auto& det : detections) {
            detection_idx++;
            std::cout << "\n[OCR Debug] Processing detection #" << detection_idx
                      << " bbox: (" << det.bbox.x << "," << det.bbox.y << ","
                      << det.bbox.width << "," << det.bbox.height << ") conf: " << det.confidence << std::endl;

            // Expand bbox with padding
            cv::Rect padded = expandBbox(det.bbox, m_bboxPadding, cvImage.size());

            // Skip tiny regions
            if (padded.width < m_minRegionSize || padded.height < m_minRegionSize) {
                std::cout << "[OCR Debug] Skipping tiny region: " << padded.width << "x" << padded.height << std::endl;
                continue;
            }

            // Crop region
            cv::Mat roi = cvImage(padded);

            // Run SVTR recognition
            float validRatio = 1.0f;
            std::vector<float> logits = m_impl->svtr->forward(roi, validRatio);

            // Run CTC decoder
            std::string recognizedText = m_impl->decoder->decode(logits, validRatio);

            // Skip empty results
            if (recognizedText.empty()) {
                std::cout << "[OCR Debug] Empty recognition result, skipping" << std::endl;
                continue;
            }

            std::cout << "[OCR Debug] Recognized text: \"" << recognizedText << "\"" << std::endl;

            // Create DetectedReference
            DetectedReference ref;
            ref.text = std::wstring(recognizedText.begin(), recognizedText.end());
            ref.x = det.bbox.x;
            ref.y = det.bbox.y;
            ref.width = det.bbox.width;
            ref.height = det.bbox.height;
            ref.confidence = det.confidence;

            result.references.push_back(ref);
        }
        auto recEnd = std::chrono::high_resolution_clock::now();
        result.recognitionTimeMs =
            std::chrono::duration<double, std::milli>(recEnd - recStart).count();

        result.success = true;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("OCR error: ") + e.what();
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    result.totalTimeMs =
        std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

    return result;
}

static cv::Mat wxImageToCvMat(const wxImage& wxImg) {
    if (!wxImg.IsOk()) {
        return cv::Mat();
    }

    int width = wxImg.GetWidth();
    int height = wxImg.GetHeight();

    // wxImage stores RGB, OpenCV uses BGR
    cv::Mat mat(height, width, CV_8UC3);

    unsigned char* wxData = wxImg.GetData();
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int wxIdx = (y * width + x) * 3;
            mat.at<cv::Vec3b>(y, x)[2] = wxData[wxIdx];     // R
            mat.at<cv::Vec3b>(y, x)[1] = wxData[wxIdx + 1]; // G
            mat.at<cv::Vec3b>(y, x)[0] = wxData[wxIdx + 2]; // B
        }
    }

    return mat;
}

static cv::Rect expandBbox(const cv::Rect& bbox, int padding,
                                   const cv::Size& imgSize) {
    int x1 = std::max(0, bbox.x - padding);
    int y1 = std::max(0, bbox.y - padding);
    int x2 = std::min(imgSize.width, bbox.x + bbox.width + padding);
    int y2 = std::min(imgSize.height, bbox.y + bbox.height + padding);
    return cv::Rect(x1, y1, x2 - x1, y2 - y1);
}

// Configuration setters
void NcnnOcrEngine::setConfidenceThreshold(float threshold) {
    m_confThreshold = std::clamp(threshold, 0.0f, 1.0f);
}

float NcnnOcrEngine::getConfidenceThreshold() const {
    return m_confThreshold;
}

void NcnnOcrEngine::setNmsThreshold(float threshold) {
    m_nmsThreshold = std::clamp(threshold, 0.0f, 1.0f);
}

void NcnnOcrEngine::setBboxPadding(int padding) {
    m_bboxPadding = std::max(0, padding);
}

void NcnnOcrEngine::setMinRegionSize(int minSize) {
    m_minRegionSize = std::max(1, minSize);
}

#endif // HAVE_OCR_SUPPORT
