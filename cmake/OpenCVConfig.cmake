# OpenCVConfig.cmake - OpenCV Configuration Module for OCR
# This file contains the OpenCV build configuration for PaddleOCR support
# Only builds the minimal modules needed: core, imgproc, dnn

function(setup_opencv)
    set(OPENCV_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opencv)

    # Verify OpenCV source exists
    if(NOT EXISTS ${OPENCV_SOURCE_DIR}/CMakeLists.txt)
        message(FATAL_ERROR "OpenCV source not found at ${OPENCV_SOURCE_DIR}. "
                            "Please run: git submodule update --init --recursive")
    endif()

    message(STATUS "Configuring OpenCV from ${OPENCV_SOURCE_DIR}")

    # ==============================================================================
    # Build Configuration - Static library, minimal features
    # ==============================================================================
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(OPENCV_ENABLE_NONFREE OFF CACHE BOOL "" FORCE)
    set(ENABLE_PIC ON CACHE BOOL "" FORCE)

    # Disable all extra tools and bindings
    set(BUILD_opencv_apps OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python2 OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python3 OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python_bindings_generator OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_java OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_java_bindings_generator OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_js OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_js_bindings_generator OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_objc_bindings_generator OFF CACHE BOOL "" FORCE)
    set(BUILD_ANDROID_PROJECTS OFF CACHE BOOL "" FORCE)
    set(BUILD_ANDROID_EXAMPLES OFF CACHE BOOL "" FORCE)

    # Disable tests, examples, docs
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_ts OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_world OFF CACHE BOOL "" FORCE)

    # ==============================================================================
    # Enable only required modules: core, imgproc, dnn
    # ==============================================================================
    set(BUILD_opencv_core ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_imgproc ON CACHE BOOL "" FORCE)
    set(BUILD_opencv_dnn ON CACHE BOOL "" FORCE)

    # Disable all other OpenCV modules
    set(BUILD_opencv_calib3d OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_features2d OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_flann OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_gapi OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_highgui OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_imgcodecs OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_ml OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_objdetect OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_photo OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_stitching OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_video OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_videoio OFF CACHE BOOL "" FORCE)

    # ==============================================================================
    # DNN Backend Configuration
    # ==============================================================================
    # Enable protobuf for ONNX support
    set(WITH_PROTOBUF ON CACHE BOOL "" FORCE)
    set(BUILD_PROTOBUF ON CACHE BOOL "" FORCE)
    set(PROTOBUF_UPDATE_FILES OFF CACHE BOOL "" FORCE)

    # ONNX support for PaddleOCR models
    set(OPENCV_DNN_OPENCL OFF CACHE BOOL "" FORCE)

    # ==============================================================================
    # Disable GPU backends (CPU only for maximum portability)
    # ==============================================================================
    set(WITH_CUDA OFF CACHE BOOL "" FORCE)
    set(WITH_CUDNN OFF CACHE BOOL "" FORCE)
    set(WITH_OPENCL OFF CACHE BOOL "" FORCE)
    set(WITH_OPENCLAMDFFT OFF CACHE BOOL "" FORCE)
    set(WITH_OPENCLAMDBLAS OFF CACHE BOOL "" FORCE)
    set(WITH_VULKAN OFF CACHE BOOL "" FORCE)

    # ==============================================================================
    # Disable I/O backends we don't need
    # ==============================================================================
    set(WITH_FFMPEG OFF CACHE BOOL "" FORCE)
    set(WITH_GSTREAMER OFF CACHE BOOL "" FORCE)
    set(WITH_V4L OFF CACHE BOOL "" FORCE)
    set(WITH_DSHOW OFF CACHE BOOL "" FORCE)
    set(WITH_MSMF OFF CACHE BOOL "" FORCE)
    set(WITH_AVFOUNDATION OFF CACHE BOOL "" FORCE)

    # Disable GUI backends (we use wxWidgets)
    set(WITH_GTK OFF CACHE BOOL "" FORCE)
    set(WITH_GTK_2_X OFF CACHE BOOL "" FORCE)
    set(WITH_QT OFF CACHE BOOL "" FORCE)
    set(WITH_WIN32UI OFF CACHE BOOL "" FORCE)

    # Disable image codecs (we use wxWidgets for image I/O)
    set(WITH_PNG OFF CACHE BOOL "" FORCE)
    set(WITH_JPEG OFF CACHE BOOL "" FORCE)
    set(WITH_TIFF OFF CACHE BOOL "" FORCE)
    set(WITH_WEBP OFF CACHE BOOL "" FORCE)
    set(WITH_OPENJPEG OFF CACHE BOOL "" FORCE)
    set(WITH_JASPER OFF CACHE BOOL "" FORCE)
    set(WITH_OPENEXR OFF CACHE BOOL "" FORCE)

    # Disable other optional features
    set(WITH_EIGEN OFF CACHE BOOL "" FORCE)
    set(WITH_LAPACK OFF CACHE BOOL "" FORCE)
    set(WITH_IPP OFF CACHE BOOL "" FORCE)
    set(WITH_TBB OFF CACHE BOOL "" FORCE)
    set(WITH_OPENMP OFF CACHE BOOL "" FORCE)
    set(WITH_PTHREADS_PF OFF CACHE BOOL "" FORCE)
    set(WITH_1394 OFF CACHE BOOL "" FORCE)

    # ==============================================================================
    # Platform-specific settings
    # ==============================================================================
    if(MSVC)
        # MSVC-specific settings
        set(BUILD_WITH_STATIC_CRT OFF CACHE BOOL "" FORCE)
    endif()

    # ==============================================================================
    # Add OpenCV subdirectory
    # ==============================================================================
    add_subdirectory(${OPENCV_SOURCE_DIR} ${CMAKE_BINARY_DIR}/opencv EXCLUDE_FROM_ALL)

    # Make sure the DNN module is built
    if(TARGET opencv_dnn)
        message(STATUS "OpenCV DNN module configured successfully")
    else()
        message(WARNING "OpenCV DNN module not available - OCR will not work")
    endif()

endfunction()
