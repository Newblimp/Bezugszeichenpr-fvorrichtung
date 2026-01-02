#include "preprocessing.h"

PreprocessResult ImagePreprocessor::preprocess(const cv::Mat& image) {
    // 1. Resize to 32 height, preserving aspect ratio
    int h = image.rows;
    int w = image.cols;
    int target_h = 32;
    int target_w = 100;

    // Calculate new width to maintain aspect ratio
    float scale = (float)target_h / h;
    int new_w = std::floor(w * scale);
    
    // Cap width at target_w if it exceeds (though rare for text images)
    if (new_w > target_w) {
        new_w = target_w;
    }

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(new_w, target_h));

    // 2. Pad to target_w width with 126
    cv::Mat padded(target_h, target_w, image.type(), cv::Scalar(126, 126, 126));
    // Copy resized image to the left side of padded image
    resized.copyTo(padded(cv::Rect(0, 0, new_w, target_h)));

    // 3. Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(padded, rgb, cv::COLOR_BGR2RGB);

    // 4. Convert to float and normalize: (pixel - 127.5) / 127.5
    cv::Mat normalized;
    rgb.convertTo(normalized, CV_32F);
    normalized = (normalized - 127.5f) / 127.5f;

    // 5. Convert HWC to CHW format
    PreprocessResult result;
    result.data = hwc_to_chw(normalized);
    result.valid_ratio = (float)new_w / target_w;
    
    return result;
}

std::vector<float> ImagePreprocessor::hwc_to_chw(const cv::Mat& image) {
    int H = image.rows;  // 32
    int W = image.cols;  // 100
    int C = image.channels();  // 3

    std::vector<float> output(C * H * W);

    for (int c = 0; c < C; ++c) {
        for (int h = 0; h < H; ++h) {
            for (int w = 0; w < W; ++w) {
                int idx = c * H * W + h * W + w;
                output[idx] = image.at<cv::Vec3f>(h, w)[c];
            }
        }
    }

    return output;
}
