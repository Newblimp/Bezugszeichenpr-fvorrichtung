# EmbedModels.cmake - Compress and embed ONNX models as C++ byte arrays
# This script generates resources/models_compressed.cpp containing zlib-compressed model data

set(MODEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/models)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(OUTPUT_FILE ${RESOURCES_DIR}/models_compressed.cpp)
set(EMBED_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compress_models.py)

# Create resources directory if it doesn't exist
file(MAKE_DIRECTORY ${RESOURCES_DIR})

# Check if models exist
set(DET_MODEL ${MODEL_DIR}/det.onnx)
set(REC_MODEL ${MODEL_DIR}/rec.onnx)

if(NOT EXISTS ${DET_MODEL})
    message(WARNING "Detection model not found at ${DET_MODEL}")
    message(WARNING "OCR functionality will not work without models")
endif()

if(NOT EXISTS ${REC_MODEL})
    message(WARNING "Recognition model not found at ${REC_MODEL}")
    message(WARNING "OCR functionality will not work without models")
endif()

# Find Python for model compression
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Custom command to generate compressed models
add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${Python3_EXECUTABLE} ${EMBED_SCRIPT}
            --det ${DET_MODEL}
            --rec ${REC_MODEL}
            --output ${OUTPUT_FILE}
    DEPENDS ${DET_MODEL} ${REC_MODEL} ${EMBED_SCRIPT}
    COMMENT "Compressing and embedding OCR models..."
    VERBATIM
)

# Custom target for model compression
add_custom_target(compress_ocr_models DEPENDS ${OUTPUT_FILE})

# Make sure models are compressed before building the main executable
if(TARGET Bezugszeichenvorrichtung)
    add_dependencies(Bezugszeichenvorrichtung compress_ocr_models)
endif()

message(STATUS "OCR model embedding configured")
message(STATUS "  Detection model: ${DET_MODEL}")
message(STATUS "  Recognition model: ${REC_MODEL}")
message(STATUS "  Output: ${OUTPUT_FILE}")
