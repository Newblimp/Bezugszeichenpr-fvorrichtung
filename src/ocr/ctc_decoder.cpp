#include "ctc_decoder.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstring>

static std::vector<float> load_weight(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open weight file: " + path);
    }

    // Read number of dimensions
    int32_t num_dims;
    file.read(reinterpret_cast<char*>(&num_dims), sizeof(int32_t));

    // Read shape
    std::vector<int32_t> shape(num_dims);
    file.read(reinterpret_cast<char*>(shape.data()), num_dims * sizeof(int32_t));

    // Calculate total elements
    size_t total = 1;
    for (int dim : shape) {
        total *= dim;
    }

    // Read data
    std::vector<float> data(total);
    file.read(reinterpret_cast<char*>(data.data()), total * sizeof(float));

    return data;
}

CTCDecoder::CTCDecoder(const std::string& weights_dir) {
    // Load decoder linear layer weights: [11, 192]
    decoder_weight = load_weight(weights_dir + "/decoder_decoder_weight.bin");
    decoder_bias = load_weight(weights_dir + "/decoder_decoder_bias.bin");

    std::cout << "CTC Decoder loaded: weight=" << decoder_weight.size()
              << ", bias=" << decoder_bias.size() << " (from file)" << std::endl;
}

CTCDecoder::CTCDecoder(const unsigned char* weight_data, size_t weight_size,
                       const unsigned char* bias_data, size_t bias_size) {
    // Load decoder weights from memory buffers
    // Weight data format: [num_dims (int32), shape...  (int32s), data (floats)]
    // Bias data format: same as weight data

    // Helper lambda to load from memory buffer
    auto load_from_memory = [](const unsigned char* data, size_t size) -> std::vector<float> {
        // Read number of dimensions
        int32_t num_dims;
        if (size < sizeof(int32_t)) {
            throw std::runtime_error("Insufficient data for num_dims");
        }
        std::memcpy(&num_dims, data, sizeof(int32_t));

        // Read shape
        std::vector<int32_t> shape(num_dims);
        if (size < sizeof(int32_t) + num_dims * sizeof(int32_t)) {
            throw std::runtime_error("Insufficient data for shape");
        }
        std::memcpy(shape.data(), data + sizeof(int32_t), num_dims * sizeof(int32_t));

        // Calculate total elements
        size_t total = 1;
        for (int dim : shape) {
            total *= dim;
        }

        // Read data
        size_t data_offset = sizeof(int32_t) + num_dims * sizeof(int32_t);
        if (size < data_offset + total * sizeof(float)) {
            throw std::runtime_error("Insufficient data for weights");
        }

        std::vector<float> result(total);
        std::memcpy(result.data(), data + data_offset, total * sizeof(float));
        return result;
    };

    decoder_weight = load_from_memory(weight_data, weight_size);
    decoder_bias = load_from_memory(bias_data, bias_size);

    std::cout << "CTC Decoder loaded: weight=" << decoder_weight.size()
              << ", bias=" << decoder_bias.size() << " (embedded)" << std::endl;
}

std::vector<float> CTCDecoder::linear(const std::vector<float>& input) {
    // input: [seq_len, in_features]
    // weight: [out_features, in_features] = [11, 192]
    // output: [seq_len, out_features]

    const int in_features = 192;
    const int out_features = 11;
    const int seq_len = input.size() / in_features;

    std::vector<float> output(seq_len * out_features);

    for (int t = 0; t < seq_len; ++t) {
        for (int o = 0; o < out_features; ++o) {
            float sum = decoder_bias[o];

            for (int i = 0; i < in_features; ++i) {
                sum += input[t * in_features + i] *
                       decoder_weight[o * in_features + i];
            }

            output[t * out_features + o] = sum;
        }
    }

    return output;
}

std::string CTCDecoder::decode(const std::vector<float>& encoder_output, float valid_ratio) {
    // Apply linear layer
    std::vector<float> logits = linear(encoder_output);

    // Greedy CTC decoding
    std::string result;
    int prev_idx = 10;  // Blank token (class 10)

    const int num_classes = 11;
    const int seq_len = logits.size() / num_classes;

    // Calculate valid sequence length based on valid_ratio
    // Only process the portion of the sequence that contains actual text, not padding
    int valid_seq_len = static_cast<int>(seq_len * valid_ratio);
    valid_seq_len = std::min(valid_seq_len, seq_len);  // Clamp to max seq_len

    // Debug: Print first few logits to see what the model is predicting
    if (seq_len > 0) {
        std::cout << "[CTC Debug] First timestep logits: ";
        for (int c = 0; c < num_classes; ++c) {
            std::cout << logits[c] << " ";
        }
        std::cout << std::endl;
    }

    for (int t = 0; t < valid_seq_len; ++t) {  // Only process valid portion
        // Find argmax at timestep t
        int max_idx = 0;
        float max_val = logits[t * num_classes];

        for (int c = 1; c < num_classes; ++c) {
            if (logits[t * num_classes + c] > max_val) {
                max_val = logits[t * num_classes + c];
                max_idx = c;
            }
        }

        // Debug: Print decoded character
        if (max_idx != 10 && max_idx != prev_idx) {
            std::cout << "[CTC Debug] t=" << t << " max_idx=" << max_idx << " char='" << ('0' + max_idx) << "'" << std::endl;
        }

        // Add to result if not blank and not repeat
        if (max_idx != 10 && max_idx != prev_idx) {
            result += ('0' + max_idx);  // Convert to digit character
        }

        prev_idx = max_idx;
    }

    std::cout << "[CTC Debug] Final result: \"" << result << "\"" << std::endl;

    return result;
}
