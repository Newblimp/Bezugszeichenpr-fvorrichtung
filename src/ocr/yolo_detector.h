#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "net.h"

struct Detection {
    cv::Rect bbox;
    float confidence;
    int class_id;
};

class YOLODetector {
public:
    // Load from files (backward compatibility)
    YOLODetector(const std::string& param_path, const std::string& bin_path);

    // Load from embedded memory buffers
    YOLODetector(const unsigned char* param_data, size_t param_size,
                 const unsigned char* bin_data, size_t bin_size);

    ~YOLODetector();

    std::vector<Detection> detect(const cv::Mat& image,
                                   float conf_threshold = 0.25f,
                                   float nms_threshold = 0.45f);

private:
    ncnn::Net net_;
    int input_size_ = 640;
    int num_classes_ = 1;  // "reference_sign"

    // Preprocessing: letterbox resize
    void preprocess(const cv::Mat& image, ncnn::Mat& input,
                    float& scale, int& pad_w, int& pad_h);

    // Postprocessing: decode predictions and apply NMS
    std::vector<Detection> postprocess(const ncnn::Mat& output,
                                        float scale, int pad_w, int pad_h,
                                        int orig_w, int orig_h,
                                        float conf_threshold, float nms_threshold);

    // Non-Maximum Suppression
    void nms(std::vector<Detection>& detections, float threshold);

    // IoU calculation
    float iou(const cv::Rect& a, const cv::Rect& b);
};

#endif // YOLO_DETECTOR_H
