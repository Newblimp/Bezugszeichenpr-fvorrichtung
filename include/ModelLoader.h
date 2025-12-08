#pragma once

#ifdef HAVE_OPENCV

#include <vector>
#include <cstdint>
#include <string>

/**
 * @brief Loads compressed ONNX models embedded in the executable
 *
 * Models are compressed with zlib during build and embedded as C arrays.
 * This class decompresses them at runtime for OpenCV DNN.
 */
class ModelLoader {
public:
    /**
     * @brief Get decompressed detection model data
     * @return Uncompressed ONNX bytes, empty vector on failure
     */
    static std::vector<uint8_t> getDetectionModel();

    /**
     * @brief Get decompressed recognition model data
     * @return Uncompressed ONNX bytes, empty vector on failure
     */
    static std::vector<uint8_t> getRecognitionModel();

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
    /**
     * @brief Decompress zlib data
     * @param compressed Compressed data pointer
     * @param compressedSize Size of compressed data
     * @param uncompressedSize Expected uncompressed size
     * @return Decompressed data, empty vector on failure
     */
    static std::vector<uint8_t> decompress(
        const uint8_t* compressed,
        size_t compressedSize,
        size_t uncompressedSize
    );

    static std::string s_lastError;
};

#endif // HAVE_OPENCV
