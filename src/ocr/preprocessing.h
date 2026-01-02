#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

struct PreprocessResult {
    std::vector<float> data;
    float valid_ratio; // valid_width / target_width (0.0 to 1.0)
};

class ImagePreprocessor {
public:
    // Preprocess image: resize to 32x100, normalize to [-1,1], convert to CHW
    static PreprocessResult preprocess(const cv::Mat& image);

private:
    static std::vector<float> hwc_to_chw(const cv::Mat& image);
};
