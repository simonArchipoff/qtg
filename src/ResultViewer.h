#pragma once

#include <optional>
#include <vector>
#include "concurrentqueue.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "ResultSignal.h"
#include <functional>
#include <complex>
#include <SoundCardDrift.h>
#include <CircularBuffer.h>

class ResultViewer {
public:
    ResultViewer();
    ~ResultViewer();

    void renderFrame();
    void pushResult(Result result);
    void pushDriftResult(DriftResult & drift){
        driftbuf.push_back(drift);
    }
    void pushRawData(std::vector<std::complex<float>> &);
    bool shouldClose() const;
    std::function<void()> onReset = [&](){};
private:
    GLFWwindow* window = nullptr;
    std::optional<Result> latestResult;
    std::optional<std::vector<float>> raw_data;
    CircularBuffer<DriftResult> driftbuf;
    void processQueue();
};