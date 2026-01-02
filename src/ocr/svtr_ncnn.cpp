#include "svtr_ncnn.h"
#include <iostream>
#include <algorithm>

SVTRncnn::SVTRncnn(const std::string& param_path, const std::string& bin_path) {
    net_.opt.use_vulkan_compute = false;
    net_.opt.num_threads = 4;
    net_.opt.use_fp16_packed = false;   // Disable FP16 packed (computation still in FP32)
    net_.opt.use_fp16_storage = true;   // Enable FP16 storage (50% smaller model)

    int ret = net_.load_param(param_path.c_str());
    if (ret != 0) {
        throw std::runtime_error("Failed to load SVTR param: " + param_path);
    }

    ret = net_.load_model(bin_path.c_str());
    if (ret != 0) {
        throw std::runtime_error("Failed to load SVTR model: " + bin_path);
    }

    std::cout << "SVTR ncnn model loaded successfully (fp16, from file)" << std::endl;
}

SVTRncnn::SVTRncnn(const unsigned char* param_data, size_t param_size,
                   const unsigned char* bin_data, size_t bin_size) {
    net_.opt.use_vulkan_compute = false;
    net_.opt.num_threads = 4;
    net_.opt.use_fp16_packed = false;   // Disable FP16 packed (computation still in FP32)
    net_.opt.use_fp16_storage = true;   // Enable FP16 storage (50% smaller model)

    std::cout << "[SVTR Load] Param size: " << param_size << " bytes" << std::endl;
    std::cout << "[SVTR Load] Bin size: " << bin_size << " bytes" << std::endl;

    // Load param from memory
    ncnn::DataReaderFromMemory param_reader(param_data);
    int ret = net_.load_param(param_reader);
    if (ret != 0) {
        throw std::runtime_error("Failed to load SVTR param from memory");
    }
    std::cout << "[SVTR Load] Param loaded successfully" << std::endl;

    // Load model from memory
    ncnn::DataReaderFromMemory model_reader(bin_data);
    ret = net_.load_model(model_reader);
    if (ret != 0) {
        throw std::runtime_error("Failed to load SVTR model from memory");
    }
    std::cout << "[SVTR Load] Model weights loaded successfully" << std::endl;
    std::cout << "[SVTR Load] Number of layers: " << net_.layers().size() << std::endl;

    std::cout << "SVTR ncnn model loaded successfully (fp16, embedded)" << std::endl;
}

SVTRncnn::~SVTRncnn() {
    net_.clear();
}

ncnn::Mat SVTRncnn::preprocess(const cv::Mat& image, float& valid_ratio) {
    int img_h = image.rows;
    int img_w = image.cols;

    std::cout << "[SVTR Preprocess] Input image type: " << image.type()
              << " channels: " << image.channels() << std::endl;

    // Calculate resize scale (maintain aspect ratio) - match original preprocessing
    float scale = (float)input_height_ / img_h;
    int new_w = std::floor(img_w * scale);

    // Cap width at input_width_ if it exceeds
    if (new_w > input_width_) {
        new_w = input_width_;
    }

    std::cout << "[SVTR Preprocess] Resizing to " << new_w << "x" << input_height_ << std::endl;

    // Resize
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(new_w, input_height_));

    // Create padded image with LEFT-aligned padding (not center!) - match original
    cv::Mat padded(input_height_, input_width_, CV_8UC3, cv::Scalar(126, 126, 126));
    resized.copyTo(padded(cv::Rect(0, 0, new_w, input_height_)));  // LEFT align at (0,0)

    // Calculate valid ratio (for CTC cropping)
    valid_ratio = (float)new_w / input_width_;

    std::cout << "[SVTR Preprocess] Padded image: " << input_width_ << "x" << input_height_
              << " type: " << padded.type() << std::endl;

    // Convert to ncnn Mat (note: ncnn uses width x height)
    ncnn::Mat input = ncnn::Mat::from_pixels(
        padded.data, ncnn::Mat::PIXEL_BGR2RGB, input_width_, input_height_);

    std::cout << "[SVTR Preprocess] ncnn::Mat created - c:" << input.c
              << " h:" << input.h << " w:" << input.w << std::endl;

    // Check a few pixel values before normalization
    std::cout << "[SVTR Preprocess] Sample pixel values before norm: "
              << input.channel(0)[0] << " " << input.channel(1)[0] << " "
              << input.channel(2)[0] << std::endl;

    // Normalize: (pixel - 127.5) / 127.5 → [-1, 1]
    const float mean_vals[3] = {127.5f, 127.5f, 127.5f};
    const float norm_vals[3] = {1.0f/127.5f, 1.0f/127.5f, 1.0f/127.5f};
    input.substract_mean_normalize(mean_vals, norm_vals);

    // Check after normalization
    std::cout << "[SVTR Preprocess] Sample pixel values after norm: "
              << input.channel(0)[0] << " " << input.channel(1)[0] << " "
              << input.channel(2)[0] << std::endl;

    return input;
}

std::vector<float> SVTRncnn::forward(const cv::Mat& image, float& valid_ratio) {
    // Preprocess
    ncnn::Mat input = preprocess(image, valid_ratio);

    std::cout << "[SVTR Debug] Input image: " << image.cols << "x" << image.rows
              << ", valid_ratio: " << valid_ratio << std::endl;

    // Run inference
    ncnn::Extractor ex = net_.create_extractor();
    ex.input("in0", input);  // PNNX uses "in0"

    ncnn::Mat output;
    int ret = ex.extract("out0", output);  // PNNX uses "out0"
    if (ret != 0) {
        throw std::runtime_error("SVTR inference failed");
    }

    // Output shape from PNNX: [192, 1, 25]
    // But we need to convert this to [25, 192] for CTC decoder
    // ncnn output dims: output.c=192, output.h=1, output.w=25

    int seq_len = output.w;      // 25
    int channels = output.c;      // 192

    std::cout << "[SVTR Debug] Output shape: c=" << channels << ", h=" << output.h
              << ", w=" << seq_len << std::endl;

    // Reshape from [192, 1, 25] to [25, 192] for CTC decoder
    std::vector<float> result(seq_len * channels);

    for (int t = 0; t < seq_len; t++) {
        for (int c = 0; c < channels; c++) {
            // ncnn storage: channel-major
            const float* channel_ptr = output.channel(c);
            result[t * channels + c] = channel_ptr[t];
        }
    }

    // Debug: Print statistics of first timestep features
    float min_val = result[0], max_val = result[0];
    float sum = 0.0f;
    for (int c = 0; c < channels; c++) {
        float val = result[c];
        min_val = std::min(min_val, val);
        max_val = std::max(max_val, val);
        sum += val;
    }
    std::cout << "[SVTR Debug] First timestep features - min: " << min_val
              << ", max: " << max_val << ", mean: " << (sum / channels) << std::endl;

    return result;
}
