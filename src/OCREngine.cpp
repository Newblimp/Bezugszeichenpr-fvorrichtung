#ifdef HAVE_OPENCV

#include "OCREngine.h"
#include "ModelLoader.h"
#include <opencv2/imgproc.hpp>
#include <wx/image.h>
#include <algorithm>
#include <numeric>
#include <cmath>

OCREngine::OCREngine() : m_refNumberPattern(L"^\\d+[a-zA-Z']*$") {
    initCharDictionary();
}

OCREngine::~OCREngine() = default;

void OCREngine::initCharDictionary() {
    // PaddleOCR v3/v4/v5 character dictionary
    // Index 0 is blank for CTC decoding
    m_charDict.push_back(L"");  // blank

    // Digits 0-9 (indices 1-10)
    for (wchar_t c = L'0'; c <= L'9'; ++c) {
        m_charDict.push_back(std::wstring(1, c));
    }

    // Lowercase letters a-z (indices 11-36)
    for (wchar_t c = L'a'; c <= L'z'; ++c) {
        m_charDict.push_back(std::wstring(1, c));
    }

    // Uppercase letters A-Z (indices 37-62)
    for (wchar_t c = L'A'; c <= L'Z'; ++c) {
        m_charDict.push_back(std::wstring(1, c));
    }

    // Common punctuation
    const std::wstring punct = L"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    for (wchar_t c : punct) {
        m_charDict.push_back(std::wstring(1, c));
    }

    // Space
    m_charDict.push_back(L" ");
}

bool OCREngine::initialize() {
    if (m_initialized) {
        return true;
    }

    if (!ModelLoader::hasModels()) {
        m_lastError = "No embedded OCR models found";
        return false;
    }

    try {
        // Load detection model
        std::vector<uint8_t> detData = ModelLoader::getDetectionModel();
        if (detData.empty()) {
            m_lastError = wxString::FromUTF8(
                "Failed to decompress detection model: " + ModelLoader::getLastError());
            return false;
        }

        m_detNet = cv::dnn::readNetFromONNX(detData);
        if (m_detNet.empty()) {
            m_lastError = "Failed to load detection model";
            return false;
        }

        // Load recognition model
        std::vector<uint8_t> recData = ModelLoader::getRecognitionModel();
        if (recData.empty()) {
            m_lastError = wxString::FromUTF8(
                "Failed to decompress recognition model: " + ModelLoader::getLastError());
            return false;
        }

        m_recNet = cv::dnn::readNetFromONNX(recData);
        if (m_recNet.empty()) {
            m_lastError = "Failed to load recognition model";
            return false;
        }

        // Set backend and target (CPU)
        m_detNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_detNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        m_recNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_recNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

        m_initialized = true;
        m_lastError.Clear();
        return true;

    } catch (const cv::Exception& e) {
        m_lastError = wxString::Format("OpenCV error: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        m_lastError = wxString::Format("Error: %s", e.what());
        return false;
    }
}

cv::Mat OCREngine::wxBitmapToCvMat(const wxBitmap& bitmap) {
    wxImage image = bitmap.ConvertToImage();
    int width = image.GetWidth();
    int height = image.GetHeight();

    cv::Mat mat(height, width, CV_8UC3);
    unsigned char* imgData = image.GetData();

    // Convert RGB to BGR (OpenCV format)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int srcIdx = (y * width + x) * 3;
            int dstIdx = y * mat.step + x * 3;
            mat.data[dstIdx + 0] = imgData[srcIdx + 2]; // B
            mat.data[dstIdx + 1] = imgData[srcIdx + 1]; // G
            mat.data[dstIdx + 2] = imgData[srcIdx + 0]; // R
        }
    }

    return mat;
}

cv::Mat OCREngine::preprocessForDetection(const cv::Mat& image, float& ratio) {
    int h = image.rows;
    int w = image.cols;

    // Resize to fixed size while maintaining aspect ratio
    ratio = 1.0f;
    int resizeH = h;
    int resizeW = w;

    // Limit to max size
    if (std::max(h, w) > DET_INPUT_SIZE) {
        if (h > w) {
            ratio = static_cast<float>(DET_INPUT_SIZE) / h;
        } else {
            ratio = static_cast<float>(DET_INPUT_SIZE) / w;
        }
        resizeH = static_cast<int>(h * ratio);
        resizeW = static_cast<int>(w * ratio);
    }

    // Make dimensions divisible by 32
    resizeH = ((resizeH + 31) / 32) * 32;
    resizeW = ((resizeW + 31) / 32) * 32;

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(resizeW, resizeH));

    // Normalize and create blob
    cv::Mat normalized;
    resized.convertTo(normalized, CV_32F, 1.0 / 255.0);

    // Normalize with mean and std
    cv::Scalar mean(0.485, 0.456, 0.406);
    cv::Scalar std(0.229, 0.224, 0.225);
    cv::subtract(normalized, mean, normalized);
    cv::divide(normalized, std, normalized);

    // Create blob (NCHW format)
    cv::Mat blob = cv::dnn::blobFromImage(normalized);

    return blob;
}

std::vector<std::vector<cv::Point>> OCREngine::postprocessDetection(
    const cv::Mat& output, const cv::Size& origSize, float ratio) {

    std::vector<std::vector<cv::Point>> textBoxes;

    // Output shape is [1, 1, H, W] - probability map
    cv::Mat probMap;
    if (output.dims == 4) {
        // Get the 2D probability map
        int outH = output.size[2];
        int outW = output.size[3];
        probMap = cv::Mat(outH, outW, CV_32F, const_cast<float*>(output.ptr<float>(0, 0)));
    } else {
        m_lastError = "Unexpected detection output format";
        return textBoxes;
    }

    // Threshold to binary
    cv::Mat binary;
    cv::threshold(probMap, binary, DET_THRESHOLD, 1.0, cv::THRESH_BINARY);
    binary.convertTo(binary, CV_8UC1, 255);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        if (contour.size() < 4) continue;

        // Get bounding rect and check minimum size
        cv::Rect boundRect = cv::boundingRect(contour);
        if (boundRect.width < 5 || boundRect.height < 5) continue;

        // Calculate box score
        cv::Mat mask = cv::Mat::zeros(probMap.size(), CV_8UC1);
        cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{contour}, cv::Scalar(1));
        float score = static_cast<float>(cv::mean(probMap, mask)[0]);

        if (score < DET_BOX_THRESH) continue;

        // Get minimum area rectangle
        cv::RotatedRect minRect = cv::minAreaRect(contour);

        // Convert back to original image coordinates
        cv::Point2f vertices[4];
        minRect.points(vertices);

        std::vector<cv::Point> box;
        for (int i = 0; i < 4; ++i) {
            int x = static_cast<int>(vertices[i].x / ratio);
            int y = static_cast<int>(vertices[i].y / ratio);
            x = std::max(0, std::min(x, origSize.width - 1));
            y = std::max(0, std::min(y, origSize.height - 1));
            box.push_back(cv::Point(x, y));
        }

        textBoxes.push_back(box);
    }

    return textBoxes;
}

cv::Mat OCREngine::cropRotatedRect(const cv::Mat& image, const cv::RotatedRect& rect) {
    // Get the 4 corners
    cv::Point2f vertices[4];
    rect.points(vertices);

    // Sort points: top-left, top-right, bottom-right, bottom-left
    std::vector<cv::Point2f> pts(vertices, vertices + 4);
    std::sort(pts.begin(), pts.end(), [](const cv::Point2f& a, const cv::Point2f& b) {
        return a.y < b.y;
    });

    cv::Point2f tl, tr, br, bl;
    if (pts[0].x < pts[1].x) {
        tl = pts[0]; tr = pts[1];
    } else {
        tl = pts[1]; tr = pts[0];
    }
    if (pts[2].x < pts[3].x) {
        bl = pts[2]; br = pts[3];
    } else {
        bl = pts[3]; br = pts[2];
    }

    // Calculate dimensions
    float width = std::max(
        std::sqrt(std::pow(tr.x - tl.x, 2) + std::pow(tr.y - tl.y, 2)),
        std::sqrt(std::pow(br.x - bl.x, 2) + std::pow(br.y - bl.y, 2))
    );
    float height = std::max(
        std::sqrt(std::pow(bl.x - tl.x, 2) + std::pow(bl.y - tl.y, 2)),
        std::sqrt(std::pow(br.x - tr.x, 2) + std::pow(br.y - tr.y, 2))
    );

    // Perspective transform
    cv::Point2f srcPts[4] = {tl, tr, br, bl};
    cv::Point2f dstPts[4] = {
        cv::Point2f(0, 0),
        cv::Point2f(width, 0),
        cv::Point2f(width, height),
        cv::Point2f(0, height)
    };

    cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);
    cv::Mat cropped;
    cv::warpPerspective(image, cropped, M, cv::Size(static_cast<int>(width), static_cast<int>(height)));

    return cropped;
}

cv::Mat OCREngine::preprocessForRecognition(const cv::Mat& textCrop) {
    // Resize to fixed height, variable width
    int h = textCrop.rows;
    int w = textCrop.cols;

    float ratio = static_cast<float>(REC_HEIGHT) / h;
    int newW = static_cast<int>(w * ratio);

    // Limit width
    newW = std::min(newW, REC_WIDTH);

    cv::Mat resized;
    cv::resize(textCrop, resized, cv::Size(newW, REC_HEIGHT));

    // Pad to fixed width if needed
    cv::Mat padded = cv::Mat::zeros(REC_HEIGHT, REC_WIDTH, resized.type());
    resized.copyTo(padded(cv::Rect(0, 0, resized.cols, resized.rows)));

    // Convert to grayscale if needed, then normalize
    cv::Mat gray;
    if (padded.channels() == 3) {
        cv::cvtColor(padded, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = padded;
    }

    // Normalize to [-1, 1]
    cv::Mat normalized;
    gray.convertTo(normalized, CV_32F, 1.0 / 127.5, -1.0);

    // Create blob
    cv::Mat blob = cv::dnn::blobFromImage(normalized);

    return blob;
}

std::wstring OCREngine::decodeRecognition(const cv::Mat& output) {
    // Output shape is [1, seq_len, num_classes]
    std::wstring result;

    if (output.dims != 3) {
        return result;
    }

    int seqLen = output.size[1];
    int numClasses = output.size[2];

    int lastIdx = -1;
    for (int t = 0; t < seqLen; ++t) {
        // Find max probability class
        const float* scores = output.ptr<float>(0, t);
        int maxIdx = 0;
        float maxScore = scores[0];
        for (int c = 1; c < numClasses; ++c) {
            if (scores[c] > maxScore) {
                maxScore = scores[c];
                maxIdx = c;
            }
        }

        // CTC decoding: skip blanks and repeated characters
        if (maxIdx != 0 && maxIdx != lastIdx) {
            if (static_cast<size_t>(maxIdx) < m_charDict.size()) {
                result += m_charDict[maxIdx];
            }
        }
        lastIdx = maxIdx;
    }

    return result;
}

std::vector<OCRResult> OCREngine::processImage(const wxBitmap& bitmap) {
    std::vector<OCRResult> results;

    if (!m_initialized) {
        m_lastError = "OCR engine not initialized";
        return results;
    }

    try {
        // Convert wxBitmap to cv::Mat
        cv::Mat image = wxBitmapToCvMat(bitmap);
        if (image.empty()) {
            m_lastError = "Failed to convert image";
            return results;
        }

        // Detection
        float ratio;
        cv::Mat detBlob = preprocessForDetection(image, ratio);
        m_detNet.setInput(detBlob);
        cv::Mat detOutput = m_detNet.forward();

        // Get text boxes
        auto textBoxes = postprocessDetection(detOutput, image.size(), ratio);

        // Process each detected text region
        for (const auto& box : textBoxes) {
            if (box.size() != 4) continue;

            // Create rotated rect from box
            cv::RotatedRect rotRect = cv::minAreaRect(box);

            // Ensure minimum size
            if (rotRect.size.width < 10 || rotRect.size.height < 10) continue;

            // Crop text region
            cv::Mat textCrop = cropRotatedRect(image, rotRect);
            if (textCrop.empty() || textCrop.rows < 5 || textCrop.cols < 5) continue;

            // Recognition
            cv::Mat recBlob = preprocessForRecognition(textCrop);
            m_recNet.setInput(recBlob);
            cv::Mat recOutput = m_recNet.forward();

            // Decode text
            std::wstring text = decodeRecognition(recOutput);

            // Skip empty results
            if (text.empty()) continue;

            // Filter for reference numbers only
            if (!isReferenceNumber(text)) continue;

            // Add result
            OCRResult result;
            result.text = text;
            result.boundingBox = rotRect.boundingRect();
            result.confidence = 1.0f;  // TODO: Calculate actual confidence

            results.push_back(result);
        }

        m_lastError.Clear();

    } catch (const cv::Exception& e) {
        m_lastError = wxString::Format("OpenCV error: %s", e.what());
    } catch (const std::exception& e) {
        m_lastError = wxString::Format("Error: %s", e.what());
    }

    return results;
}

bool OCREngine::isReferenceNumber(const std::wstring& text) {
    // Pattern: starts with digit(s), optionally followed by letters or '
    // Examples: "10", "12a", "15'", "100", "10a'"
    return std::regex_match(text, m_refNumberPattern);
}

#endif // HAVE_OPENCV
