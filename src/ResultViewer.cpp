#include "ResultViewer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <iostream>
#include <thread>
#include <GL/gl.h>

ResultViewer::ResultViewer() {
    if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
    window = glfwCreateWindow(800, 600, "Result Viewer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
}

ResultViewer::~ResultViewer() {
       ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void ResultViewer::pushResult(Result result) {
    latestResult.emplace(result);
}
bool ResultViewer::shouldClose() const {
    return window && glfwWindowShouldClose(window);
}


void ResultViewer::renderFrame() {
    glfwPollEvents();
    int w,h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
    ImGui::Begin("Résultat",nullptr,    ImGuiWindowFlags_NoTitleBar |
    //ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse);

    if (latestResult) {
        ImGui::Text("Time: %.2f s", latestResult->time);
        if (ImGui::Button("Reset")) {
            onReset();
        }
        ImVec2 size = ImGui::GetContentRegionAvail();
        // Le plot ImPlot
        if (ImPlot::BeginPlot("energie vs dérive", size, ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxes("seconde par mois", "energie", 0& ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, -60, 60);
            //ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
            auto spm =  latestResult->sec_per_month();
                        //latestResult->frequencies;
            ImPlot::PlotLine("seconde par mois",
                spm.data(),
                latestResult->magnitudes.data(),
                (int)latestResult->frequencies.size());

            ImPlot::EndPlot();
        }

    } else {
        ImGui::Text("En attente de données...");
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


    glfwSwapBuffers(window);
}