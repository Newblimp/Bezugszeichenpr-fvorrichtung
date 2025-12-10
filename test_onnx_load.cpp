// Quick test to check if OpenCV can load the ONNX models from disk
#include <opencv2/dnn.hpp>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <path_to_det.onnx> <path_to_rec.onnx>" << std::endl;
        return 1;
    }

    std::string det_path = argv[1];
    std::string rec_path = argv[2];

    std::cout << "Testing ONNX model loading..." << std::endl;

    try {
        std::cout << "Loading detection model from: " << det_path << std::endl;
        cv::dnn::Net detNet = cv::dnn::readNetFromONNX(det_path);
        if (detNet.empty()) {
            std::cerr << "Failed to load detection model - network is empty" << std::endl;
            return 1;
        }
        std::cout << "Detection model loaded successfully!" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error loading detection model: " << e.what() << std::endl;
        return 1;
    }

    try {
        std::cout << "Loading recognition model from: " << rec_path << std::endl;
        cv::dnn::Net recNet = cv::dnn::readNetFromONNX(rec_path);
        if (recNet.empty()) {
            std::cerr << "Failed to load recognition model - network is empty" << std::endl;
            return 1;
        }
        std::cout << "Recognition model loaded successfully!" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error loading recognition model: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "All models loaded successfully!" << std::endl;
    return 0;
}
