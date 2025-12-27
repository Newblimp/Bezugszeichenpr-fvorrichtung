#pragma once

#ifdef HAVE_OCR_SUPPORT

#include "IOcrEngine.h"
#include <memory>

// Forward declaration for cv::Mat to avoid OpenCV header in public interface
namespace cv {
    class Mat;
}

/**
 * @brief ncnn-based OCR engine using YOLO + SVTR + CTC decoder
 *
 * Pipeline:
 * 1. YOLO detector finds text regions (bounding boxes)
 * 2. SVTR recognizer processes each region (feature extraction)
 * 3. CTC decoder converts features to digit strings
 *
 * Models are embedded as constexpr arrays in the executable.
 */
class NcnnOcrEngine : public IOcrEngine {
public:
    // Constructor loads embedded models
    NcnnOcrEngine();

    // Destructor (PIMPL pattern to hide OpenCV/ncnn includes)
    ~NcnnOcrEngine() override;

    // IOcrEngine implementation
    OcrResult processImage(const wxImage& image,
                          size_t pageIndex = 0,
                          const std::string& sourcePath = "") override;

    void setConfidenceThreshold(float threshold) override;
    float getConfidenceThreshold() const override;

    std::string getEngineName() const override { return "YOLO-SVTR-ncnn"; }
    std::string getEngineVersion() const override { return "1.0.0"; }

    // Additional configuration
    void setNmsThreshold(float threshold);
    void setBboxPadding(int padding);
    void setMinRegionSize(int minSize);

private:
    // PIMPL pattern to hide implementation details
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // Configuration
    float m_confThreshold{0.25f};
    float m_nmsThreshold{0.45f};
    int m_bboxPadding{5};
    int m_minRegionSize{10};
};

#endif // HAVE_OCR_SUPPORT
