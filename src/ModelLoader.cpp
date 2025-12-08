#ifdef HAVE_OPENCV

#include "ModelLoader.h"

// External declarations from generated models_uncompressed.cpp
extern const uint8_t det_model_uncompressed[];
extern const size_t det_model_uncompressed_size;

extern const uint8_t rec_model_uncompressed[];
extern const size_t rec_model_uncompressed_size;

// Static member initialization
std::string ModelLoader::s_lastError;

std::vector<unsigned char> ModelLoader::getDetectionModel() {
    if (!det_model_uncompressed || det_model_uncompressed_size == 0) {
        s_lastError = "Detection model not available";
        return {};
    }

    s_lastError.clear();
    return std::vector<unsigned char>(
        det_model_uncompressed,
        det_model_uncompressed + det_model_uncompressed_size
    );
}

std::vector<unsigned char> ModelLoader::getRecognitionModel() {
    if (!rec_model_uncompressed || rec_model_uncompressed_size == 0) {
        s_lastError = "Recognition model not available";
        return {};
    }

    s_lastError.clear();
    return std::vector<unsigned char>(
        rec_model_uncompressed,
        rec_model_uncompressed + rec_model_uncompressed_size
    );
}

bool ModelLoader::hasModels() {
    return det_model_uncompressed_size > 0 && rec_model_uncompressed_size > 0;
}

std::string ModelLoader::getLastError() {
    return s_lastError;
}

#endif // HAVE_OPENCV
