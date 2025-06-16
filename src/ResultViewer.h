#pragma once

#include <optional>
#include <vector>
#include "concurrentqueue.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "ResultSignal.h"
#include <functional>

class ResultViewer {
public:
    ResultViewer();
    ~ResultViewer();

    void renderFrame();
    void pushResult(Result result);
    bool shouldClose() const;
    std::function<void()> onReset = [&](){};
private:
    GLFWwindow* window = nullptr;
    std::optional<Result> latestResult;

    void processQueue();
};