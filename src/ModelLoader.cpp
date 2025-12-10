#ifdef HAVE_OPENCV

#include "ModelLoader.h"

// External declarations from generated models_uncompressed.cpp
extern const uint8_t det_model_uncompressed[];
extern const size_t det_model_uncompressed_size;

extern const uint8_t trocr_encoder_uncompressed[];
extern const size_t trocr_encoder_uncompressed_size;

extern const uint8_t trocr_decoder_uncompressed[];
extern const size_t trocr_decoder_uncompressed_size;

extern const uint8_t trocr_vocab_uncompressed[];
extern const size_t trocr_vocab_uncompressed_size;

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

std::vector<unsigned char> ModelLoader::getTrOCREncoder() {
    if (!trocr_encoder_uncompressed || trocr_encoder_uncompressed_size == 0) {
        s_lastError = "TrOCR encoder model not available";
        return {};
    }

    s_lastError.clear();
    return std::vector<unsigned char>(
        trocr_encoder_uncompressed,
        trocr_encoder_uncompressed + trocr_encoder_uncompressed_size
    );
}

std::vector<unsigned char> ModelLoader::getTrOCRDecoder() {
    if (!trocr_decoder_uncompressed || trocr_decoder_uncompressed_size == 0) {
        s_lastError = "TrOCR decoder model not available";
        return {};
    }

    s_lastError.clear();
    return std::vector<unsigned char>(
        trocr_decoder_uncompressed,
        trocr_decoder_uncompressed + trocr_decoder_uncompressed_size
    );
}

std::vector<unsigned char> ModelLoader::getTrOCRVocabulary() {
    if (!trocr_vocab_uncompressed || trocr_vocab_uncompressed_size == 0) {
        s_lastError = "TrOCR vocabulary not available";
        return {};
    }

    s_lastError.clear();
    return std::vector<unsigned char>(
        trocr_vocab_uncompressed,
        trocr_vocab_uncompressed + trocr_vocab_uncompressed_size
    );
}

bool ModelLoader::hasModels() {
    return det_model_uncompressed_size > 0 &&
           trocr_encoder_uncompressed_size > 0 &&
           trocr_decoder_uncompressed_size > 0 &&
           trocr_vocab_uncompressed_size > 0;
}

std::string ModelLoader::getLastError() {
    return s_lastError;
}

#endif // HAVE_OPENCV
