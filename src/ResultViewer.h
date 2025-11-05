#pragma once

#include <optional>
#include <vector>
#include <imgui.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "ResultSignal.h"
#include <functional>
#include <complex>
#include "SoundCardDrift.h"



class VuMeter {
public:
    VuMeter(){}

    void push_level(float level) {
        _level = std::clamp(level, 0.0f, 1.0f);
    }

    void draw(const ImVec2& size = ImVec2(100, 10)) {
        float epsilon = 1e-5f;
        float level_db = 20.0f * std::log10(_level + epsilon);

        float log_level = (level_db + 100.0f) / 100.0f;
        log_level = std::clamp(log_level, 0.0f, 1.0f);

        ImGui::ProgressBar(log_level, size, "");
    }

private:
    float _level = 0.0f;
};


class ResultViewer {
public:
    ResultViewer(Unit u);
    ~ResultViewer();

    void renderFrame();
    void pushDriftResult(DriftResult drift){
        this->drift = drift;
    }
    void pushRawData(std::vector<std::complex<float>> &);
    void setCurrentIntegration(double d){
        currentIntegrationPart = d;
    }
    virtual void displayResult() = 0;
    bool shouldClose() const;
    std::function<void()> onReset = [&](){};
    std::function<void(double)> onIntegrationTime = [&](double){abort();};
    std::function<void(double)> onApplyCorrection = [&](double){abort();};
protected:
    enum Unit unit;
    GLFWwindow* window = nullptr;

    std::optional<DriftResult> drift;
    double currentIntegrationPart = 0.0;
    void processQueue();
public:
    VuMeter vumeter;
};


class ResultViewerQuartz : public ResultViewer {
    public:
    ResultViewerQuartz(Unit u):ResultViewer(u){};
    void displayResult() override;
    void pushResult(Result result);
    protected:
    std::optional<Result> latestResult;
    std::vector<std::complex<float>> raw_data;

};


#include "MecaDSP.h"

class ResultViewerMeca : public ResultViewer{
    public:
    ResultViewerMeca(Unit u):ResultViewer(u),circ(2048){}


    void pushResult(MecaDSPResult & r){
        mecaresult = r;
        circ.push_back(r.newSamples);
    }
    void displayResult() override;

    MecaDSPResult mecaresult;
    CircularBuffer<float> circ;

};