#ifdef HAVE_OPENCV

#include "OCREngine.h"
#include "ModelLoader.h"
#include <opencv2/imgproc.hpp>
#include <wx/image.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <sstream>
#include <codecvt>
#include <locale>

OCREngine::OCREngine() : m_refNumberPattern(L"^\\d+[a-zA-Z']*$") {
}

OCREngine::~OCREngine() = default;

void OCREngine::loadVocabulary(const std::vector<unsigned char>& vocabData) {
    // Parse vocabulary: one token per line
    m_vocabulary.clear();

    std::string vocabStr(vocabData.begin(), vocabData.end());
    std::istringstream stream(vocabStr);
    std::string line;

    // Convert UTF-8 to wstring
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    while (std::getline(stream, line)) {
        // Handle escaped characters
        std::string unescaped;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '\\' && i + 1 < line.size()) {
                char next = line[i + 1];
                if (next == 'n') { unescaped += '\n'; ++i; }
                else if (next == 'r') { unescaped += '\r'; ++i; }
                else if (next == '\\') { unescaped += '\\'; ++i; }
                else { unescaped += line[i]; }
            } else {
                unescaped += line[i];
            }
        }

        try {
            m_vocabulary.push_back(converter.from_bytes(unescaped));
        } catch (...) {
            m_vocabulary.push_back(L"");  // Invalid UTF-8, use empty
        }
    }

    std::cout << "Loaded vocabulary with " << m_vocabulary.size() << " tokens" << std::endl;
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
        // Load detection model (OpenCV DNN)
        std::vector<unsigned char> detData = ModelLoader::getDetectionModel();
        if (detData.empty()) {
            m_lastError = wxString::FromUTF8(
                "Failed to load detection model: " + ModelLoader::getLastError());
            return false;
        }

        m_detNet = cv::dnn::readNetFromONNX(detData);
        if (m_detNet.empty()) {
            m_lastError = "Failed to parse detection model";
            return false;
        }
        m_detNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_detNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        std::cout << "YOLOv11n detection model loaded (OpenCV DNN)" << std::endl;

        // Initialize ONNXRuntime
        m_ortEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "TrOCR");
        m_sessionOptions.SetIntraOpNumThreads(1);
        m_sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // Load TrOCR encoder
        std::vector<unsigned char> encoderData = ModelLoader::getTrOCREncoder();
        if (encoderData.empty()) {
            m_lastError = wxString::FromUTF8(
                "Failed to load TrOCR encoder: " + ModelLoader::getLastError());
            return false;
        }
        m_encoderSession = std::make_unique<Ort::Session>(
            *m_ortEnv,
            encoderData.data(),
            encoderData.size(),
            m_sessionOptions
        );
        std::cout << "TrOCR encoder loaded (ONNXRuntime)" << std::endl;

        // Load TrOCR decoder
        std::vector<unsigned char> decoderData = ModelLoader::getTrOCRDecoder();
        if (decoderData.empty()) {
            m_lastError = wxString::FromUTF8(
                "Failed to load TrOCR decoder: " + ModelLoader::getLastError());
            return false;
        }
        m_decoderSession = std::make_unique<Ort::Session>(
            *m_ortEnv,
            decoderData.data(),
            decoderData.size(),
            m_sessionOptions
        );
        std::cout << "TrOCR decoder loaded (ONNXRuntime)" << std::endl;

        // Load vocabulary
        std::vector<unsigned char> vocabData = ModelLoader::getTrOCRVocabulary();
        if (vocabData.empty()) {
            m_lastError = wxString::FromUTF8(
                "Failed to load vocabulary: " + ModelLoader::getLastError());
            return false;
        }
        loadVocabulary(vocabData);

        m_initialized = true;
        m_lastError.Clear();
        return true;

    } catch (const Ort::Exception& e) {
        m_lastError = wxString::Format("ONNXRuntime error: %s", e.what());
        return false;
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

wxBitmap OCREngine::cvMatToWxBitmap(const cv::Mat& mat) {
    // Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

    int width = rgb.cols;
    int height = rgb.rows;

    wxImage image(width, height);
    unsigned char* imgData = image.GetData();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int srcIdx = y * rgb.step + x * 3;
            int dstIdx = (y * width + x) * 3;
            imgData[dstIdx + 0] = rgb.data[srcIdx + 0]; // R
            imgData[dstIdx + 1] = rgb.data[srcIdx + 1]; // G
            imgData[dstIdx + 2] = rgb.data[srcIdx + 2]; // B
        }
    }

    return wxBitmap(image);
}

cv::Mat OCREngine::preprocessForDetection(const cv::Mat& image, float& scaleX, float& scaleY,
                                          int& padX, int& padY) {
    int origH = image.rows;
    int origW = image.cols;

    // Calculate scaling to fit image into DET_INPUT_SIZE x DET_INPUT_SIZE with letterboxing
    float scale = std::min(static_cast<float>(DET_INPUT_SIZE) / origW,
                          static_cast<float>(DET_INPUT_SIZE) / origH);

    int newW = static_cast<int>(origW * scale);
    int newH = static_cast<int>(origH * scale);

    // Calculate padding to center the image
    padX = (DET_INPUT_SIZE - newW) / 2;
    padY = (DET_INPUT_SIZE - newH) / 2;

    // Store scales for later coordinate conversion
    scaleX = scale;
    scaleY = scale;

    // Resize image
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(newW, newH));

    // Create padded image (letterbox) - fill with gray (114, 114, 114)
    cv::Mat padded = cv::Mat(DET_INPUT_SIZE, DET_INPUT_SIZE, CV_8UC3, cv::Scalar(114, 114, 114));
    resized.copyTo(padded(cv::Rect(padX, padY, newW, newH)));

    // Normalize to [0, 1] and convert to blob (NCHW format)
    cv::Mat blob = cv::dnn::blobFromImage(padded, 1.0 / 255.0, cv::Size(DET_INPUT_SIZE, DET_INPUT_SIZE),
                                         cv::Scalar(0, 0, 0), true, false);

    return blob;
}

std::vector<cv::Rect> OCREngine::postprocessDetection(
    const cv::Mat& output, const cv::Size& origSize,
    float scaleX, float scaleY, int padX, int padY) {

    std::vector<cv::Rect> textBoxes;

    // YOLOv11 output format: [1, num_predictions, 5] or [1, 5, num_predictions]
    // where 5 = [x_center, y_center, width, height, confidence]
    if (output.dims != 3) {
        m_lastError = "Unexpected YOLO output dimensions";
        std::cerr << "YOLO output dims: " << output.dims << std::endl;
        return textBoxes;
    }

    int dim0 = output.size[0];  // Should be 1 (batch)
    int dim1 = output.size[1];
    int dim2 = output.size[2];

    std::cout << "YOLO output shape: [" << dim0 << ", " << dim1 << ", " << dim2 << "]" << std::endl;

    // Determine format: [1, num_preds, 5] or [1, 5, num_preds]
    int numPredictions, numValues;
    bool transposed = false;

    if (dim1 == 5 || dim1 == 6) {  // [1, 5/6, num_preds] format (transposed)
        numValues = dim1;
        numPredictions = dim2;
        transposed = true;
    } else {  // [1, num_preds, 5/6] format
        numPredictions = dim1;
        numValues = dim2;
    }

    std::cout << "Detected " << numPredictions << " predictions (transposed=" << transposed << ")" << std::endl;

    // Collect all detections with confidence > threshold
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;

    const float* data = output.ptr<float>();

    for (int i = 0; i < numPredictions; ++i) {
        float confidence, xCenter, yCenter, width, height;

        if (transposed) {
            // Data layout: [x_center, y_center, width, height, conf, ...][pred0, pred1, ...]
            xCenter = data[0 * numPredictions + i];
            yCenter = data[1 * numPredictions + i];
            width = data[2 * numPredictions + i];
            height = data[3 * numPredictions + i];
            confidence = data[4 * numPredictions + i];
        } else {
            // Data layout: [pred0{x, y, w, h, conf}, pred1{...}, ...]
            int offset = i * numValues;
            xCenter = data[offset + 0];
            yCenter = data[offset + 1];
            width = data[offset + 2];
            height = data[offset + 3];
            confidence = data[offset + 4];
        }

        // Apply confidence threshold
        if (confidence < DET_CONF_THRESHOLD) {
            continue;
        }

        // Convert from YOLO format (center x, center y, width, height) to bounding box
        // Coordinates are in the letterboxed image space (0-640)
        float x1 = xCenter - width / 2.0f;
        float y1 = yCenter - height / 2.0f;
        float x2 = xCenter + width / 2.0f;
        float y2 = yCenter + height / 2.0f;

        // Remove padding and scale back to original image coordinates
        x1 = (x1 - padX) / scaleX;
        y1 = (y1 - padY) / scaleY;
        x2 = (x2 - padX) / scaleX;
        y2 = (y2 - padY) / scaleY;

        // Clamp to image bounds
        x1 = std::max(0.0f, std::min(x1, static_cast<float>(origSize.width - 1)));
        y1 = std::max(0.0f, std::min(y1, static_cast<float>(origSize.height - 1)));
        x2 = std::max(0.0f, std::min(x2, static_cast<float>(origSize.width - 1)));
        y2 = std::max(0.0f, std::min(y2, static_cast<float>(origSize.height - 1)));

        cv::Rect box(static_cast<int>(x1), static_cast<int>(y1),
                     static_cast<int>(x2 - x1), static_cast<int>(y2 - y1));

        boxes.push_back(box);
        confidences.push_back(confidence);
    }

    std::cout << "Found " << boxes.size() << " boxes before NMS" << std::endl;

    // Apply Non-Maximum Suppression (NMS)
    if (!boxes.empty()) {
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, DET_CONF_THRESHOLD, DET_NMS_THRESHOLD, indices);

        std::cout << "After NMS: " << indices.size() << " boxes" << std::endl;

        for (int idx : indices) {
            textBoxes.push_back(boxes[idx]);
        }
    }

    return textBoxes;
}

std::vector<float> OCREngine::preprocessForTrOCR(const cv::Mat& textCrop) {
    // Resize to 384x384
    cv::Mat resized;
    cv::resize(textCrop, resized, cv::Size(TROCR_IMAGE_SIZE, TROCR_IMAGE_SIZE));

    // Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    // Normalize: rescale to [0,1] then normalize with mean=0.5, std=0.5 -> [-1, 1]
    cv::Mat normalized;
    rgb.convertTo(normalized, CV_32F, 1.0 / 255.0);

    // Apply normalization: (x - 0.5) / 0.5 = 2*x - 1
    normalized = normalized * 2.0f - 1.0f;

    // Convert to NCHW format (batch=1, channels=3, height=384, width=384)
    std::vector<float> pixelValues(1 * 3 * TROCR_IMAGE_SIZE * TROCR_IMAGE_SIZE);

    for (int c = 0; c < 3; ++c) {
        for (int h = 0; h < TROCR_IMAGE_SIZE; ++h) {
            for (int w = 0; w < TROCR_IMAGE_SIZE; ++w) {
                pixelValues[c * TROCR_IMAGE_SIZE * TROCR_IMAGE_SIZE + h * TROCR_IMAGE_SIZE + w] =
                    normalized.at<cv::Vec3f>(h, w)[c];
            }
        }
    }

    return pixelValues;
}

std::wstring OCREngine::runTrOCRInference(const std::vector<float>& pixelValues) {
    try {
        Ort::AllocatorWithDefaultOptions allocator;
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        // === Run Encoder ===
        std::vector<int64_t> encoderInputShape = {1, 3, TROCR_IMAGE_SIZE, TROCR_IMAGE_SIZE};
        Ort::Value encoderInput = Ort::Value::CreateTensor<float>(
            memoryInfo,
            const_cast<float*>(pixelValues.data()),
            pixelValues.size(),
            encoderInputShape.data(),
            encoderInputShape.size()
        );

        const char* encoderInputNames[] = {"pixel_values"};
        const char* encoderOutputNames[] = {"last_hidden_state"};

        auto encoderOutputs = m_encoderSession->Run(
            Ort::RunOptions{nullptr},
            encoderInputNames, &encoderInput, 1,
            encoderOutputNames, 1
        );

        // Get encoder hidden states
        float* encoderHiddenData = encoderOutputs[0].GetTensorMutableData<float>();
        auto encoderOutputShape = encoderOutputs[0].GetTensorTypeAndShapeInfo().GetShape();
        size_t encoderSeqLen = encoderOutputShape[1];
        size_t hiddenSize = encoderOutputShape[2];

        std::cout << "  Encoder output: [1, " << encoderSeqLen << ", " << hiddenSize << "]" << std::endl;

        // === Run Decoder (autoregressive generation) ===
        std::vector<int64_t> generatedTokens;
        generatedTokens.push_back(TROCR_START_TOKEN);

        for (int step = 0; step < TROCR_MAX_LENGTH; ++step) {
            // Prepare decoder inputs
            std::vector<int64_t> inputIdsShape = {1, static_cast<int64_t>(generatedTokens.size())};
            Ort::Value inputIdsTensor = Ort::Value::CreateTensor<int64_t>(
                memoryInfo,
                generatedTokens.data(),
                generatedTokens.size(),
                inputIdsShape.data(),
                inputIdsShape.size()
            );

            std::vector<int64_t> encoderHiddenShape = {1, static_cast<int64_t>(encoderSeqLen), static_cast<int64_t>(hiddenSize)};
            size_t encoderHiddenSize = encoderSeqLen * hiddenSize;
            Ort::Value encoderHiddenTensor = Ort::Value::CreateTensor<float>(
                memoryInfo,
                encoderHiddenData,
                encoderHiddenSize,
                encoderHiddenShape.data(),
                encoderHiddenShape.size()
            );

            std::vector<Ort::Value> decoderInputs;
            decoderInputs.push_back(std::move(inputIdsTensor));
            decoderInputs.push_back(std::move(encoderHiddenTensor));

            const char* decoderInputNames[] = {"input_ids", "encoder_hidden_states"};
            const char* decoderOutputNames[] = {"logits"};

            auto decoderOutputs = m_decoderSession->Run(
                Ort::RunOptions{nullptr},
                decoderInputNames, decoderInputs.data(), 2,
                decoderOutputNames, 1
            );

            // Get logits for the last token
            float* logits = decoderOutputs[0].GetTensorMutableData<float>();
            auto logitsShape = decoderOutputs[0].GetTensorTypeAndShapeInfo().GetShape();
            size_t seqLen = logitsShape[1];
            size_t vocabSize = logitsShape[2];

            // Get logits for last position
            float* lastLogits = logits + (seqLen - 1) * vocabSize;

            // Apply logit biasing to favor digit tokens
            // Digit token IDs found from tokenizer.json (single digits 0-9)
            static const std::vector<int> digitTokenIds = {
                252,  // ▁2
                267,  // ▁1
                271,  // ▁3
                319,  // ▁4
                331,  // ▁5
                467,  // ▁6
                531,  // ▁7
                539,  // ▁8
                641,  // ▁9
                792,  // 2 (no space)
                896,  // 1 (no space)
                1023, // 3 (no space)
                1065, // 4 (no space)
                1264, // 5 (no space)
                1428, // 9 (no space)
                1439, // 8 (no space)
                1473, // 7 (no space)
                1487, // 6 (no space)
                1596, // ▁0
                1724  // 0 (no space)
            };

            // Bias: reduce logits for ALL tokens except digits and end token
            const float bias_penalty = -100.0f;  // Large negative number to suppress non-digits
            for (size_t i = 0; i < vocabSize; ++i) {
                // Skip end token (ID 2)
                if (i == TROCR_END_TOKEN) continue;

                // Check if this is a digit token
                bool isDigit = std::find(digitTokenIds.begin(), digitTokenIds.end(), i) != digitTokenIds.end();

                // Apply penalty to non-digit tokens
                if (!isDigit) {
                    lastLogits[i] += bias_penalty;
                }
            }

            // Find argmax after biasing
            int64_t nextToken = 0;
            float maxLogit = lastLogits[0];
            for (size_t i = 1; i < vocabSize; ++i) {
                if (lastLogits[i] > maxLogit) {
                    maxLogit = lastLogits[i];
                    nextToken = static_cast<int64_t>(i);
                }
            }

            std::cout << "  Step " << step << ": Generated token ID " << nextToken
                      << " (vocabSize=" << vocabSize << ", maxLogit=" << maxLogit << ")" << std::endl;

            // Check for end token
            if (nextToken == TROCR_END_TOKEN) {
                std::cout << "  Stopping: End token encountered" << std::endl;
                break;
            }

            generatedTokens.push_back(nextToken);
        }

        // Decode tokens to text (skip start token)
        std::vector<int64_t> outputTokens(generatedTokens.begin() + 1, generatedTokens.end());
        return decodeTokens(outputTokens);

    } catch (const Ort::Exception& e) {
        std::cerr << "ONNXRuntime error in TrOCR inference: " << e.what() << std::endl;
        return L"";
    }
}

std::wstring OCREngine::decodeTokens(const std::vector<int64_t>& tokens) {
    std::wstring result;

    for (int64_t token : tokens) {
        if (token >= 0 && static_cast<size_t>(token) < m_vocabulary.size()) {
            std::wstring tokenStr = m_vocabulary[token];

            // Handle sentencepiece: replace leading underscore with space
            if (!tokenStr.empty() && tokenStr[0] == L'\u2581') {
                // SentencePiece uses ▁ (U+2581) to mark spaces
                // Don't add space at the beginning
                if (!result.empty()) {
                    result += L' ';
                }
                result += tokenStr.substr(1);
            } else {
                result += tokenStr;
            }
        }
    }

    // Remove all spaces from the result (digits only)
    result.erase(std::remove(result.begin(), result.end(), L' '), result.end());

    return result;
}

std::wstring OCREngine::recognizeText(const cv::Mat& textRegion) {
    if (textRegion.empty() || textRegion.rows < 5 || textRegion.cols < 5) {
        return L"";
    }

    // Preprocess for TrOCR
    std::vector<float> pixelValues = preprocessForTrOCR(textRegion);

    // Run TrOCR inference
    return runTrOCRInference(pixelValues);
}

std::vector<OCRResult> OCREngine::processImage(const wxBitmap& bitmap) {
    std::cout << "Starting OCR process..." << std::endl;
    std::vector<OCRResult> results;

    if (!m_initialized) {
        m_lastError = "OCR engine not initialized";
        std::cout << "Error: OCR engine not initialized." << std::endl;
        return results;
    }

    try {
        // Convert wxBitmap to cv::Mat
        cv::Mat image = wxBitmapToCvMat(bitmap);
        if (image.empty()) {
            m_lastError = "Failed to convert image";
            std::cout << "Error: Failed to convert wxBitmap to cv::Mat." << std::endl;
            return results;
        }
        std::cout << "Image converted to cv::Mat successfully." << std::endl;

        // Create debug image (copy of original)
        cv::Mat debugImage = image.clone();

        // Detection (YOLOv11)
        float scaleX, scaleY;
        int padX, padY;
        cv::Mat detBlob = preprocessForDetection(image, scaleX, scaleY, padX, padY);
        m_detNet.setInput(detBlob);
        cv::Mat detOutput = m_detNet.forward();
        std::cout << "YOLOv11 detection network forwarded." << std::endl;

        // Get text boxes
        auto textBoxes = postprocessDetection(detOutput, image.size(), scaleX, scaleY, padX, padY);
        std::cout << "Detected " << textBoxes.size() << " text boxes after NMS." << std::endl;

        // Process each detected text region
        int boxIndex = 0;
        for (const auto& box : textBoxes) {
            // Skip very small boxes
            if (box.width < 10 || box.height < 10) continue;

            // Crop text region (simple rectangle crop)
            cv::Mat textCrop = image(box);
            if (textCrop.empty() || textCrop.rows < 5 || textCrop.cols < 5) continue;

            std::cout << "Processing text box #" << boxIndex << " at ("
                      << box.x << ", " << box.y << ", " << box.width << ", " << box.height << ")..." << std::endl;

            // Recognition with TrOCR
            std::wstring text = recognizeText(textCrop);
            std::wcout << L"Raw recognized text: '" << text << L"'" << std::endl;

            // Determine color based on result
            cv::Scalar color;
            std::string label;

            if (text.empty()) {
                // Blue: detected but no text recognized
                color = cv::Scalar(255, 0, 0);
                label = "[empty]";
            } else if (!isReferenceNumber(text)) {
                // Red: detected but filtered out (not a reference number)
                color = cv::Scalar(0, 0, 255);
                // Convert wstring to string for label
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                label = converter.to_bytes(text);
                std::wcout << L"  -> Text '" << text << L"' was filtered out (not a reference number)." << std::endl;
            } else {
                // Green: accepted reference number
                color = cv::Scalar(0, 255, 0);
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                label = converter.to_bytes(text);

                OCRResult result;
                result.text = text;
                result.boundingBox = box;
                result.confidence = 1.0f;
                results.push_back(result);
                std::wcout << L"  -> Added '" << text << L"' to results." << std::endl;
            }

            // Draw the bounding box
            cv::rectangle(debugImage, box, color, 2);

            // Draw label above the box
            cv::Point labelPos(box.x, box.y - 5);
            if (labelPos.y < 15) labelPos.y = 15;
            cv::putText(debugImage, label, labelPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);

            boxIndex++;
        }

        // Convert debug image to wxBitmap
        m_debugImage = cvMatToWxBitmap(debugImage);

        m_lastError.Clear();
        std::cout << "OCR process finished. Found " << results.size() << " reference numbers out of "
                  << boxIndex << " detected boxes." << std::endl;

    } catch (const Ort::Exception& e) {
        m_lastError = wxString::Format("ONNXRuntime error: %s", e.what());
        std::cerr << "ONNXRuntime error in processImage: " << e.what() << std::endl;
    } catch (const cv::Exception& e) {
        m_lastError = wxString::Format("OpenCV error: %s", e.what());
        std::cerr << "OpenCV error in processImage: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        m_lastError = wxString::Format("Error: %s", e.what());
        std::cerr << "Standard error in processImage: " << e.what() << std::endl;
    }

    return results;
}

bool OCREngine::isReferenceNumber(const std::wstring& text) {
    return std::regex_match(text, m_refNumberPattern);
}

#endif // HAVE_OPENCV
