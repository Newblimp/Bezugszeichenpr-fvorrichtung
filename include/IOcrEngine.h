#pragma once

#ifdef HAVE_OCR_SUPPORT

#include <string>
#include <vector>
#include <memory>

// Forward declaration for wxImage
class wxImage;

// Represents a single detected reference number in a drawing
struct DetectedReference {
    std::wstring text;           // Recognized text (e.g., L"123")
    int x, y, width, height;     // Bounding box in image coordinates
    float confidence;            // Detection confidence (0.0-1.0)

    // Validation status (set by DrawingAnalyzer)
    enum class ValidationStatus {
        NOT_VALIDATED,          // Not yet cross-validated
        VALID,                  // Matches text analysis
        MISSING_IN_TEXT,        // Found in drawing but not in text
        MISSING_IN_DRAWING,     // Found in text but not in drawing
        CONFLICT                // Different term assignments
    };
    ValidationStatus status{ValidationStatus::NOT_VALIDATED};
    std::wstring validationMessage;  // Human-readable explanation
};

// Container for OCR results from a single image
struct OcrResult {
    std::vector<DetectedReference> references;
    size_t pageIndex{0};
    std::string sourcePath;
    bool success{false};
    std::string errorMessage;

    // Performance metrics
    double detectionTimeMs{0.0};
    double recognitionTimeMs{0.0};
    double totalTimeMs{0.0};
};

// Abstract interface for OCR engines
class IOcrEngine {
public:
    virtual ~IOcrEngine() = default;

    // Process a single image and detect reference numbers
    virtual OcrResult processImage(const wxImage& image,
                                   size_t pageIndex = 0,
                                   const std::string& sourcePath = "") = 0;

    // Configuration
    virtual void setConfidenceThreshold(float threshold) = 0;
    virtual float getConfidenceThreshold() const = 0;

    // Engine metadata
    virtual std::string getEngineName() const = 0;
    virtual std::string getEngineVersion() const = 0;
};

#endif // HAVE_OCR_SUPPORT
