#ifdef HAVE_OPENCV

#include "ModelLoader.h"
#include <zlib.h>
#include <cstring>

// External declarations from generated models_compressed.cpp
extern const uint8_t det_model_compressed[];
extern const size_t det_model_compressed_size;
extern const size_t det_model_uncompressed_size;

extern const uint8_t rec_model_compressed[];
extern const size_t rec_model_compressed_size;
extern const size_t rec_model_uncompressed_size;

// Static member initialization
std::string ModelLoader::s_lastError;

std::vector<uint8_t> ModelLoader::decompress(
    const uint8_t* compressed,
    size_t compressedSize,
    size_t uncompressedSize)
{
    if (!compressed || compressedSize == 0 || uncompressedSize == 0) {
        s_lastError = "Invalid input data for decompression";
        return {};
    }

    std::vector<uint8_t> result(uncompressedSize);

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));

    // Initialize zlib for decompression
    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        s_lastError = "Failed to initialize zlib decompression";
        return {};
    }

    strm.avail_in = static_cast<uInt>(compressedSize);
    strm.next_in = const_cast<Bytef*>(compressed);
    strm.avail_out = static_cast<uInt>(uncompressedSize);
    strm.next_out = result.data();

    ret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        s_lastError = "Decompression failed with error code: " + std::to_string(ret);
        return {};
    }

    // Verify we got the expected size
    if (strm.total_out != uncompressedSize) {
        s_lastError = "Decompressed size mismatch: expected " +
                      std::to_string(uncompressedSize) + ", got " +
                      std::to_string(strm.total_out);
        return {};
    }

    s_lastError.clear();
    return result;
}

std::vector<uint8_t> ModelLoader::getDetectionModel() {
    return decompress(
        det_model_compressed,
        det_model_compressed_size,
        det_model_uncompressed_size
    );
}

std::vector<uint8_t> ModelLoader::getRecognitionModel() {
    return decompress(
        rec_model_compressed,
        rec_model_compressed_size,
        rec_model_uncompressed_size
    );
}

bool ModelLoader::hasModels() {
    return det_model_compressed_size > 0 && rec_model_compressed_size > 0;
}

std::string ModelLoader::getLastError() {
    return s_lastError;
}

#endif // HAVE_OPENCV
