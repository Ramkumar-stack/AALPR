#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <map>
#include <functional>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <stdexcept>

// Aliases for convenience
namespace ipc = boost::interprocess;

// Define queues for inter-process communication
ipc::message_queue *frameQueue = nullptr;
ipc::message_queue *preprocessedQueue = nullptr;
ipc::message_queue *detectedPlatesQueue = nullptr;
ipc::message_queue *ocrQueue = nullptr;

// Process function types
typedef std::function<void()> ProcessFunction;

// Flags to track process status
std::map<std::string, bool> processStatus;

// Process functions
void frameCapture() {
    try {
        for (int i = 1; i <= 10; ++i) {
            std::string frame = "Frame_" + std::to_string(i);
            frameQueue->send(frame.c_str(), frame.size() + 1, 0);
            std::cout << "[Frame Capture] Captured: " << frame << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } catch (const std::exception &e) {
        std::cerr << "[Frame Capture] Error: " << e.what() << std::endl;
        throw;
    }
}

void preprocessing() {
    try {
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;
        while (true) {
            frameQueue->receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string frame(buffer);
            std::string preprocessedFrame = frame + "_Preprocessed";
            preprocessedQueue->send(preprocessedFrame.c_str(), preprocessedFrame.size() + 1, 0);
            std::cout << "[Preprocessing] Processed: " << preprocessedFrame << std::endl;
            if (frame.find("10") != std::string::npos) break;
        }
    } catch (const std::exception &e) {
        std::cerr << "[Preprocessing] Error: " << e.what() << std::endl;
        throw;
    }
}

void plateDetection() {
    try {
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;
        while (true) {
            preprocessedQueue->receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string preprocessedFrame(buffer);
            std::string detectedPlate = preprocessedFrame + "_PlateDetected";
            detectedPlatesQueue->send(detectedPlate.c_str(), detectedPlate.size() + 1, 0);
            std::cout << "[Plate Detection] Detected: " << detectedPlate << std::endl;
            if (preprocessedFrame.find("10") != std::string::npos) break;
        }
    } catch (const std::exception &e) {
        std::cerr << "[Plate Detection] Error: " << e.what() << std::endl;
        throw;
    }
}

void ocrRecognition() {
    try {
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;
        while (true) {
            detectedPlatesQueue->receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string detectedPlate(buffer);
            std::string recognizedText = detectedPlate + "_OCRText";
            ocrQueue->send(recognizedText.c_str(), recognizedText.size() + 1, 0);
            std::cout << "[OCR Recognition] Recognized: " << recognizedText << std::endl;
            if (detectedPlate.find("10") != std::string::npos) break;
        }
    } catch (const std::exception &e) {
        std::cerr << "[OCR Recognition] Error: " << e.what() << std::endl;
        throw;
    }
}

void integration() {
    try {
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;
        while (true) {
            ocrQueue->receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string recognizedText(buffer);
            std::cout << "[Integration] Final Result: " << recognizedText << std::endl;
            if (recognizedText.find("10") != std::string::npos) break;
        }
    } catch (const std::exception &e) {
        std::cerr << "[Integration] Error: " << e.what() << std::endl;
        throw;
    }
}

// Process monitor function
void processMonitor(std::map<std::string, ProcessFunction> &processFunctions) {
    std::map<std::string, std::thread> processThreads;

    // Start all processes initially
    for (auto &[name, func] : processFunctions) {
        processStatus[name] = true;
        processThreads[name] = std::thread([name, func]() {
            try {
                func();
            } catch (...) {
                processStatus[name] = false;
            }
        });
    }

    // Monitor and restart processes if needed
    while (true) {
        for (auto &[name, status] : processStatus) {
            if (!status) {
                std::cout << "[Monitor] Restarting process: " << name << std::endl;
                processStatus[name] = true;
                processThreads[name] = std::thread([name, &processFunctions]() {
                    try {
                        processFunctions[name]();
                    } catch (...) {
                        processStatus[name] = false;
                    }
                });
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Join all threads on exit (not reached in this example)
    for (auto &[name, thread] : processThreads) {
        if (thread.joinable()) thread.join();
    }
}

int main() {
    // Remove any pre-existing queues
    ipc::message_queue::remove("frameQueue");
    ipc::message_queue::remove("preprocessedQueue");
    ipc::message_queue::remove("detectedPlatesQueue");
    ipc::message_queue::remove("ocrQueue");

    // Create message queues
    frameQueue = new ipc::message_queue(ipc::create_only, "frameQueue", 10, 1024);
    preprocessedQueue = new ipc::message_queue(ipc::create_only, "preprocessedQueue", 10, 1024);
    detectedPlatesQueue = new ipc::message_queue(ipc::create_only, "detectedPlatesQueue", 10, 1024);
    ocrQueue = new ipc::message_queue(ipc::create_only, "ocrQueue", 10, 1024);

    // Define process functions
    std::map<std::string, ProcessFunction> processFunctions = {
        {"FrameCapture", frameCapture},
        {"Preprocessing", preprocessing},
        {"PlateDetection", plateDetection},
        {"OCRRecognition", ocrRecognition},
        {"Integration", integration}};

    // Start the process monitor
    std::thread monitorThread(processMonitor, std::ref(processFunctions));

    // Join the monitor thread
    if (monitorThread.joinable()) monitorThread.join();

    // Cleanup
    ipc::message_queue::remove("frameQueue");
    ipc::message_queue::remove("preprocessedQueue");
    ipc::message_queue::remove("detectedPlatesQueue");
    ipc::message_queue::remove("ocrQueue");

    std::cout << "All processing completed!" << std::endl;
    return 0;
}
