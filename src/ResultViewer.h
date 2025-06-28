#pragma once

#include <optional>
#include <vector>
#include "concurrentqueue.h"
#include <imgui.h>
#include <GL/gl.h>
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
    void pushDriftResult(DriftResult drift){
        this->drift = drift;
    }
    void pushRawData(std::vector<std::complex<float>> &);
    void setCurrentIntegration(double d){
        currentIntegrationPart = d;
    }
    
    bool shouldClose() const;
    std::function<void()> onReset = [&](){};
    std::function<void(double)> onIntegrationTime = [&](double){abort();};
    std::function<void(double)> onApplyCorrection = [&](double){abort();};
private:
    GLFWwindow* window = nullptr;
    std::optional<Result> latestResult;
    std::optional<std::vector<float>> raw_data;
    std::optional<DriftResult> drift;
    double currentIntegrationPart = 0.0;
    void processQueue();
};