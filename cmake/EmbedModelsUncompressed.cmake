# Embed uncompressed ONNX models in the executable
# Models are stored as C++ hex arrays (no compression/decompression)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Add custom command to generate models_uncompressed.cpp
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources/models_uncompressed.cpp
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_models.py
        --det ${CMAKE_CURRENT_SOURCE_DIR}/models/det.onnx
        --rec ${CMAKE_CURRENT_SOURCE_DIR}/models/rec.onnx
        --output ${CMAKE_CURRENT_BINARY_DIR}/resources/models_uncompressed.cpp
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/models/det.onnx
        ${CMAKE_CURRENT_SOURCE_DIR}/models/rec.onnx
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_models.py
    COMMENT "Embedding uncompressed OCR models..."
    VERBATIM
)

# Create custom target for embedding
add_custom_target(embed_ocr_models ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/resources/models_uncompressed.cpp
)

message(STATUS "OCR model embedding configured (uncompressed)")
