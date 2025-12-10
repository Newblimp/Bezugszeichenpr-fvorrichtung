#pragma once

#ifdef HAVE_OPENCV

#include <vector>
#include <cstdint>
#include <string>

/**
 * @brief Loads embedded OCR models from executable
 *
 * Models embedded:
 * - Detection: YOLOv11n (for OpenCV DNN)
 * - TrOCR Encoder: Vision Transformer (for ONNXRuntime)
 * - TrOCR Decoder: GPT-2 autoregressive (for ONNXRuntime)
 * - Vocabulary: Token ID to string mapping
 */
class ModelLoader {
public:
    /**
     * @brief Get detection model data (YOLOv11n)
     * @return ONNX bytes, empty vector on failure
     */
    static std::vector<unsigned char> getDetectionModel();

    /**
     * @brief Get TrOCR encoder model data
     * @return ONNX bytes, empty vector on failure
     */
    static std::vector<unsigned char> getTrOCREncoder();

    /**
     * @brief Get TrOCR decoder model data
     * @return ONNX bytes, empty vector on failure
     */
    static std::vector<unsigned char> getTrOCRDecoder();

    /**
     * @brief Get TrOCR vocabulary data (one token per line)
     * @return UTF-8 text bytes, empty vector on failure
     */
    static std::vector<unsigned char> getTrOCRVocabulary();

    /**
     * @brief Check if all models are available
     * @return true if all embedded models exist
     */
    static bool hasModels();

    /**
     * @brief Get last error message
     */
    static std::string getLastError();

private:
    static std::string s_lastError;
};

#endif // HAVE_OPENCV
