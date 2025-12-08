#pragma once

#ifdef HAVE_OPENCV

#include <vector>
#include <cstdint>
#include <string>
#include <opencv2/core/types.hpp>

/**
 * @brief Loads uncompressed ONNX models embedded in the executable
 *
 * Models are embedded as C arrays during build with no compression.
 * This class provides direct access to them for OpenCV DNN.
 */
class ModelLoader {
public:
    /**
     * @brief Get detection model data
     * @return ONNX bytes, empty vector on failure
     */
    static std::vector<unsigned char> getDetectionModel();

    /**
     * @brief Get recognition model data
     * @return ONNX bytes, empty vector on failure
     */
    static std::vector<unsigned char> getRecognitionModel();

    /**
     * @brief Check if models are available
     * @return true if embedded models exist
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
