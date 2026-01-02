#pragma once

#include <vector>
#include <string>

class CTCDecoder {
public:
    // Load from file-based weights (backward compatibility)
    CTCDecoder(const std::string& weights_dir);

    // Load from embedded memory buffers
    CTCDecoder(const unsigned char* weight_data, size_t weight_size,
               const unsigned char* bias_data, size_t bias_size);

    // Decode encoder output to digit string
    // encoder_output: [seq_len, channels] = [25, 192]
    // valid_ratio: fraction of sequence that contains valid text (rest is padding)
    // Returns: recognized digit string (e.g., "123")
    std::string decode(const std::vector<float>& encoder_output, float valid_ratio = 1.0f);

private:
    std::vector<float> decoder_weight;  // Shape: [11, 192]
    std::vector<float> decoder_bias;    // Shape: [11]

    // Linear layer: [seq_len, 192] -> [seq_len, 11]
    std::vector<float> linear(const std::vector<float>& input);
};
