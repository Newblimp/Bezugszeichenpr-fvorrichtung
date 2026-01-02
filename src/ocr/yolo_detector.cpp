#include "yolo_detector.h"
#include <algorithm>
#include <iostream>
#include "datareader.h"

YOLODetector::YOLODetector(const std::string& param_path, const std::string& bin_path) {
    net_.opt.use_vulkan_compute = false;
    net_.opt.num_threads = 4;

    int ret = net_.load_param(param_path.c_str());
    if (ret != 0) {
        throw std::runtime_error("Failed to load YOLO param file: " + param_path);
    }

    ret = net_.load_model(bin_path.c_str());
    if (ret != 0) {
        throw std::runtime_error("Failed to load YOLO model file: " + bin_path);
    }

    std::cout << "YOLODetector loaded successfully (from file)" << std::endl;
}

YOLODetector::YOLODetector(const unsigned char* param_data, size_t param_size,
                           const unsigned char* bin_data, size_t bin_size) {
    net_.opt.use_vulkan_compute = false;
    net_.opt.num_threads = 4;

    // Load param from memory
    ncnn::DataReaderFromMemory param_reader(param_data);
    int ret = net_.load_param(param_reader);
    if (ret != 0) {
        throw std::runtime_error("Failed to load YOLO param from memory");
    }

    // Load model from memory
    ncnn::DataReaderFromMemory model_reader(bin_data);
    ret = net_.load_model(model_reader);
    if (ret != 0) {
        throw std::runtime_error("Failed to load YOLO model from memory");
    }

    std::cout << "YOLODetector loaded successfully (embedded)" << std::endl;
}

YOLODetector::~YOLODetector() {
    net_.clear();
}

void YOLODetector::preprocess(const cv::Mat& image, ncnn::Mat& input,
                               float& scale, int& pad_w, int& pad_h) {
    int img_w = image.cols;
    int img_h = image.rows;

    // Calculate scale to fit in input_size while maintaining aspect ratio
    scale = std::min((float)input_size_ / img_w, (float)input_size_ / img_h);
    int new_w = static_cast<int>(img_w * scale);
    int new_h = static_cast<int>(img_h * scale);

    // Calculate padding
    pad_w = (input_size_ - new_w) / 2;
    pad_h = (input_size_ - new_h) / 2;

    // Resize image
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(new_w, new_h));

    // Create padded image with gray background (114, 114, 114)
    cv::Mat padded(input_size_, input_size_, CV_8UC3, cv::Scalar(114, 114, 114));
    resized.copyTo(padded(cv::Rect(pad_w, pad_h, new_w, new_h)));

    // Convert to ncnn::Mat and normalize
    input = ncnn::Mat::from_pixels(padded.data, ncnn::Mat::PIXEL_BGR2RGB,
                                    input_size_, input_size_);

    // Normalize to [0, 1]
    const float norm_vals[3] = {1 / 255.f, 1 / 255.f, 1 / 255.f};
    input.substract_mean_normalize(0, norm_vals);
}

std::vector<Detection> YOLODetector::postprocess(const ncnn::Mat& output,
                                                   float scale, int pad_w, int pad_h,
                                                   int orig_w, int orig_h,
                                                   float conf_threshold, float nms_threshold) {
    std::vector<Detection> detections;

    // YOLOv11 output shape: [1, 5, 8400] where 5 = x, y, w, h, conf (for 1 class)
    // After ncnn processing it's [5, 8400] or transposed
    int num_proposals = output.w;
    int num_outputs = output.h;  // 5 = 4 (bbox) + 1 (class)

    for (int i = 0; i < num_proposals; i++) {
        // Get confidence score (for single class model)
        float confidence = output.row(4)[i];

        if (confidence < conf_threshold) {
            continue;
        }

        // Get bounding box (x_center, y_center, width, height)
        float cx = output.row(0)[i];
        float cy = output.row(1)[i];
        float w = output.row(2)[i];
        float h = output.row(3)[i];

        // Convert to corner format
        float x1 = cx - w / 2.0f;
        float y1 = cy - h / 2.0f;
        float x2 = cx + w / 2.0f;
        float y2 = cy + h / 2.0f;

        // Remove padding and scale back to original image
        x1 = (x1 - pad_w) / scale;
        y1 = (y1 - pad_h) / scale;
        x2 = (x2 - pad_w) / scale;
        y2 = (y2 - pad_h) / scale;

        // Clamp to image bounds
        x1 = std::max(0.0f, std::min(x1, (float)orig_w));
        y1 = std::max(0.0f, std::min(y1, (float)orig_h));
        x2 = std::max(0.0f, std::min(x2, (float)orig_w));
        y2 = std::max(0.0f, std::min(y2, (float)orig_h));

        Detection det;
        det.bbox = cv::Rect(
            static_cast<int>(x1),
            static_cast<int>(y1),
            static_cast<int>(x2 - x1),
            static_cast<int>(y2 - y1)
        );
        det.confidence = confidence;
        det.class_id = 0;  // Single class

        detections.push_back(det);
    }

    // Apply NMS
    nms(detections, nms_threshold);

    return detections;
}

float YOLODetector::iou(const cv::Rect& a, const cv::Rect& b) {
    int x1 = std::max(a.x, b.x);
    int y1 = std::max(a.y, b.y);
    int x2 = std::min(a.x + a.width, b.x + b.width);
    int y2 = std::min(a.y + a.height, b.y + b.height);

    int inter_w = std::max(0, x2 - x1);
    int inter_h = std::max(0, y2 - y1);
    float inter_area = inter_w * inter_h;

    float area_a = a.width * a.height;
    float area_b = b.width * b.height;
    float union_area = area_a + area_b - inter_area;

    if (union_area <= 0) return 0.0f;
    return inter_area / union_area;
}

void YOLODetector::nms(std::vector<Detection>& detections, float threshold) {
    if (detections.empty()) return;

    // Sort by confidence (descending)
    std::sort(detections.begin(), detections.end(),
              [](const Detection& a, const Detection& b) {
                  return a.confidence > b.confidence;
              });

    std::vector<bool> suppressed(detections.size(), false);
    std::vector<Detection> result;

    for (size_t i = 0; i < detections.size(); i++) {
        if (suppressed[i]) continue;

        result.push_back(detections[i]);

        for (size_t j = i + 1; j < detections.size(); j++) {
            if (suppressed[j]) continue;

            if (iou(detections[i].bbox, detections[j].bbox) > threshold) {
                suppressed[j] = true;
            }
        }
    }

    detections = result;
}

std::vector<Detection> YOLODetector::detect(const cv::Mat& image,
                                             float conf_threshold,
                                             float nms_threshold) {
    if (image.empty()) {
        return {};
    }

    // Preprocess
    ncnn::Mat input;
    float scale;
    int pad_w, pad_h;
    preprocess(image, input, scale, pad_w, pad_h);

    // Run inference
    ncnn::Extractor ex = net_.create_extractor();
    ex.input("in0", input);

    ncnn::Mat output;
    ex.extract("out0", output);

    // Postprocess
    return postprocess(output, scale, pad_w, pad_h,
                       image.cols, image.rows,
                       conf_threshold, nms_threshold);
}
