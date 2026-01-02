#ifndef SVTR_NCNN_H
#define SVTR_NCNN_H

#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "net.h"
#include "datareader.h"

class SVTRncnn {
public:
    // Load from files (for backward compatibility)
    SVTRncnn(const std::string& param_path, const std::string& bin_path);

    // Load from embedded memory buffers
    SVTRncnn(const unsigned char* param_data, size_t param_size,
             const unsigned char* bin_data, size_t bin_size);

    ~SVTRncnn();

    // Forward pass: returns [seq_len, num_classes] logits
    // Output shape: [192, 1, 25] from ncnn
    std::vector<float> forward(const cv::Mat& image, float& valid_ratio);

private:
    ncnn::Net net_;
    int input_height_ = 32;
    int input_width_ = 100;

    // Preprocessing
    ncnn::Mat preprocess(const cv::Mat& image, float& valid_ratio);
};

#endif // SVTR_NCNN_H
