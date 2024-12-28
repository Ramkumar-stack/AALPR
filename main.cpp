#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <string>
#include <map>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <thread>

namespace ipc = boost::interprocess;

// Define message queue names
const char* FRAME_QUEUE = "frameQueue";
const char* PREPROCESSED_QUEUE = "preprocessedQueue";
const char* DETECTED_PLATES_QUEUE = "detectedPlatesQueue";
const char* OCR_QUEUE = "ocrQueue";

// Process IDs for child processes
std::map<std::string, pid_t> processIDs;

void cleanQueues() {
    ipc::message_queue::remove(FRAME_QUEUE);
    ipc::message_queue::remove(PREPROCESSED_QUEUE);
    ipc::message_queue::remove(DETECTED_PLATES_QUEUE);
    ipc::message_queue::remove(OCR_QUEUE);
}

void createQueues() {
    new ipc::message_queue(ipc::create_only, FRAME_QUEUE, 10, 1024);
    new ipc::message_queue(ipc::create_only, PREPROCESSED_QUEUE, 10, 1024);
    new ipc::message_queue(ipc::create_only, DETECTED_PLATES_QUEUE, 10, 1024);
    new ipc::message_queue(ipc::create_only, OCR_QUEUE, 10, 1024);
}

void frameCapture() {
    try {
        ipc::message_queue frameQueue(ipc::open_only, FRAME_QUEUE);
        for (int i = 1; i <= 10; ++i) {
            std::string frame = "Frame_" + std::to_string(i);
            frameQueue.send(frame.c_str(), frame.size() + 1, 0);
            std::cout << "[Frame Capture] Captured: " << frame << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } catch (const std::exception& e) {
        std::cerr << "[Frame Capture] Error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void preprocessing() {
    try {
        ipc::message_queue frameQueue(ipc::open_only, FRAME_QUEUE);
        ipc::message_queue preprocessedQueue(ipc::open_only, PREPROCESSED_QUEUE);
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;

        while (true) {
            frameQueue.receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string frame(buffer);
            std::string preprocessedFrame = frame + "_Preprocessed";
            preprocessedQueue.send(preprocessedFrame.c_str(), preprocessedFrame.size() + 1, 0);
            std::cout << "[Preprocessing] Processed: " << preprocessedFrame << std::endl;
            if (frame.find("10") != std::string::npos) break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Preprocessing] Error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void plateDetection() {
    try {
        ipc::message_queue preprocessedQueue(ipc::open_only, PREPROCESSED_QUEUE);
        ipc::message_queue detectedPlatesQueue(ipc::open_only, DETECTED_PLATES_QUEUE);
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;

        while (true) {
            preprocessedQueue.receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string preprocessedFrame(buffer);
            std::string detectedPlate = preprocessedFrame + "_PlateDetected";
            detectedPlatesQueue.send(detectedPlate.c_str(), detectedPlate.size() + 1, 0);
            std::cout << "[Plate Detection] Detected: " << detectedPlate << std::endl;
            if (preprocessedFrame.find("10") != std::string::npos) break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Plate Detection] Error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void ocrRecognition() {
    try {
        ipc::message_queue detectedPlatesQueue(ipc::open_only, DETECTED_PLATES_QUEUE);
        ipc::message_queue ocrQueue(ipc::open_only, OCR_QUEUE);
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;

        while (true) {
            detectedPlatesQueue.receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string detectedPlate(buffer);
            std::string recognizedText = detectedPlate + "_OCRText";
            ocrQueue.send(recognizedText.c_str(), recognizedText.size() + 1, 0);
            std::cout << "[OCR Recognition] Recognized: " << recognizedText << std::endl;
            if (detectedPlate.find("10") != std::string::npos) break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[OCR Recognition] Error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void integration() {
    try {
        ipc::message_queue ocrQueue(ipc::open_only, OCR_QUEUE);
        char buffer[1024];
        size_t receivedSize;
        unsigned priority;

        while (true) {
            ocrQueue.receive(buffer, sizeof(buffer), receivedSize, priority);
            std::string recognizedText(buffer);
            std::cout << "[Integration] Final Result: " << recognizedText << std::endl;
            if (recognizedText.find("10") != std::string::npos) break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Integration] Error: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void monitorProcess(const std::string& name, void(*function)()) {
    while (true) {
        pid_t pid = fork();
        if (pid == 0) {
            function();
        } else if (pid > 0) {
            processIDs[name] = pid;
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                std::cout << "[Monitor] Process " << name << " exited with status: " << WEXITSTATUS(status) << std::endl;
                if (WEXITSTATUS(status) == 0) break; // Successful completion
            }

            std::cout << "[Monitor] Restarting process: " << name << std::endl;
        } else {
            std::cerr << "[Monitor] Failed to fork process: " << name << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

int main() {
    cleanQueues();
    createQueues();

    std::map<std::string, void(*)()> processes = {
        {"FrameCapture", frameCapture},
        {"Preprocessing", preprocessing},
        {"PlateDetection", plateDetection},
        {"OCRRecognition", ocrRecognition},
        {"Integration", integration}
    };

    for (const auto& [name, function] : processes) {
        if (fork() == 0) {
            monitorProcess(name, function);
            exit(0);
        }
    }

    for (const auto& [name, pid] : processIDs) {
        waitpid(pid, nullptr, 0);
    }

    cleanQueues();
    return 0;
}
