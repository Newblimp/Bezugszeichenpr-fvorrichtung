// Quick test to check if OpenCV can load the ONNX models from disk
#include <opencv2/dnn.hpp>
#include <iostream>

int main() {
    std::cout << "Testing ONNX model loading..." << std::endl;

    try {
        std::cout << "Loading det.onnx from disk..." << std::endl;
        cv::dnn::Net detNet = cv::dnn::readNetFromONNX("../models/det.onnx");
        if (detNet.empty()) {
            std::cerr << "Failed to load det.onnx - network is empty" << std::endl;
            return 1;
        }
        std::cout << "det.onnx loaded successfully!" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error loading det.onnx: " << e.what() << std::endl;
        return 1;
    }

    try {
        std::cout << "Loading rec.onnx from disk..." << std::endl;
        cv::dnn::Net recNet = cv::dnn::readNetFromONNX("../models/rec.onnx");
        if (recNet.empty()) {
            std::cerr << "Failed to load rec.onnx - network is empty" << std::endl;
            return 1;
        }
        std::cout << "rec.onnx loaded successfully!" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error loading rec.onnx: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "All models loaded successfully!" << std::endl;
    return 0;
}
