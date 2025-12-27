#ifdef HAVE_OCR_SUPPORT

#include <gtest/gtest.h>
#include "NcnnOcrEngine.h"
#include <wx/image.h>
#include <opencv2/opencv.hpp>

// Forward declare internal SVTR class for direct testing
#include "../src/ocr/svtr_ncnn.h"
#include "../src/ocr/models/svtr_model_param.h"
#include "../src/ocr/models/svtr_model_weights.h"

// Basic test to verify NcnnOcrEngine can be instantiated
TEST(OcrEngineTest, CanInstantiate) {
    EXPECT_NO_THROW({
        NcnnOcrEngine engine;
        EXPECT_EQ(engine.getEngineName(), "YOLO-SVTR-ncnn");
        EXPECT_EQ(engine.getEngineVersion(), "1.0.0");
    });
}

// Test confidence threshold configuration
TEST(OcrEngineTest, CanSetConfidenceThreshold) {
    NcnnOcrEngine engine;

    engine.setConfidenceThreshold(0.5f);
    EXPECT_FLOAT_EQ(engine.getConfidenceThreshold(), 0.5f);

    // Test clamping to valid range
    engine.setConfidenceThreshold(1.5f);
    EXPECT_FLOAT_EQ(engine.getConfidenceThreshold(), 1.0f);

    engine.setConfidenceThreshold(-0.5f);
    EXPECT_FLOAT_EQ(engine.getConfidenceThreshold(), 0.0f);
}

// Test processing with invalid image
TEST(OcrEngineTest, HandlesInvalidImage) {
    NcnnOcrEngine engine;
    wxImage invalidImage;

    OcrResult result = engine.processImage(invalidImage);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

// Test with actual simple image to check for NaN issues
TEST(OcrEngineTest, ProcessesSimpleWhiteImage) {
    std::cout << "\n=== Testing OCR with simple white image ===" << std::endl;

    NcnnOcrEngine engine;

    // Create a simple 100x100 white image
    wxImage testImage(100, 100);
    unsigned char* data = testImage.GetData();

    // Fill with white (RGB = 255, 255, 255)
    for (int i = 0; i < 100 * 100 * 3; i++) {
        data[i] = 255;
    }

    std::cout << "Processing 100x100 white image..." << std::endl;

    OcrResult result = engine.processImage(testImage);

    std::cout << "Success: " << result.success << std::endl;
    std::cout << "Error: " << result.errorMessage << std::endl;
    std::cout << "Detections: " << result.references.size() << std::endl;
    std::cout << "Detection time: " << result.detectionTimeMs << " ms" << std::endl;
    std::cout << "Recognition time: " << result.recognitionTimeMs << " ms" << std::endl;

    // Should succeed even if no detections
    EXPECT_TRUE(result.success);

    std::cout << "=== Test complete ===" << std::endl;
}

// Test with image containing black rectangle (simulated text)
TEST(OcrEngineTest, ProcessesImageWithBlackRectangle) {
    std::cout << "\n=== Testing OCR with black rectangle on white ===" << std::endl;

    NcnnOcrEngine engine;

    // Create 200x200 white image
    wxImage testImage(200, 200);
    unsigned char* data = testImage.GetData();

    // Fill with white
    for (int i = 0; i < 200 * 200 * 3; i++) {
        data[i] = 255;
    }

    // Draw a black rectangle (50,50) to (150,100) to simulate text
    for (int y = 50; y < 100; y++) {
        for (int x = 50; x < 150; x++) {
            int idx = (y * 200 + x) * 3;
            data[idx] = 0;     // R
            data[idx + 1] = 0; // G
            data[idx + 2] = 0; // B
        }
    }

    std::cout << "Processing 200x200 image with black rectangle..." << std::endl;

    OcrResult result = engine.processImage(testImage);

    std::cout << "Success: " << result.success << std::endl;
    std::cout << "Error: " << result.errorMessage << std::endl;
    std::cout << "Detections: " << result.references.size() << std::endl;
    std::cout << "Detection time: " << result.detectionTimeMs << " ms" << std::endl;
    std::cout << "Recognition time: " << result.recognitionTimeMs << " ms" << std::endl;

    // Print detected references
    for (size_t i = 0; i < result.references.size(); i++) {
        const auto& ref = result.references[i];
        std::wcout << L"  [" << i << L"] Text: \"" << ref.text
                   << L"\" @ (" << ref.x << "," << ref.y
                   << ") conf: " << ref.confidence << std::endl;
    }

    EXPECT_TRUE(result.success);

    std::cout << "=== Test complete ===" << std::endl;
}

// Direct SVTR model test to isolate NaN issue
TEST(OcrEngineTest, SVTRModelDirectTest) {
    std::cout << "\n=== Direct SVTR Model Test ===" << std::endl;

    // Create SVTR model instance using embedded model data
    SVTRncnn svtr(svtr_model_param, svtr_model_param_len,
                  svtr_model_weights, svtr_model_weights_len);

    // Create simple white test image
    cv::Mat testImg(50, 80, CV_8UC3, cv::Scalar(255, 255, 255));

    std::cout << "Running SVTR forward pass on 80x50 white image..." << std::endl;

    float validRatio = 1.0f;
    std::vector<float> logits = svtr.forward(testImg, validRatio);

    std::cout << "Got " << logits.size() << " output values" << std::endl;

    // Check for NaN
    bool hasNaN = false;
    for (size_t i = 0; i < std::min(logits.size(), size_t(20)); i++) {
        std::cout << "  logits[" << i << "] = " << logits[i];
        if (std::isnan(logits[i])) {
            std::cout << " <-- NaN!";
            hasNaN = true;
        }
        std::cout << std::endl;
    }

    EXPECT_FALSE(hasNaN) << "SVTR model produced NaN values!";

    std::cout << "=== Test complete ===" << std::endl;
}

#endif // HAVE_OCR_SUPPORT
