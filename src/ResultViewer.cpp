#include "ResultViewer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>


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
void ResultViewer::pushRawData(std::vector<std::complex<float>> & c) {
    //std::optional<std::vector<float>>;
    raw_data = std::vector<float>(c.size());
    for(uint i = 0; i < c.size(); i++)
        (*raw_data)[i] =std::abs(c[i]);

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
    
    ImPlot::MapInputDefault();


    //ImGui::SameLine();

    if(ImGui::CollapsingHeader("Soundcard data")){
        ImGui::BeginGroup();
        if(drift){
            ImGui::Text("samplerate: %.4fHz ± %.4f (ic95)", drift->get_fps_estimated(), drift->get_fps_ci95() );
            ImGui::Text("%+.4f (±%.4f) sec per month", drift->get_spm(), drift->get_spm_ci95());
            ImGui::Text("%f ppm",drift->get_ppm());
        } else{
            ImGui::Text("Drift data incomming...");
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        if(raw_data){
        if (ImPlot::BeginPlot("sample", ImVec2(300, 100),
                        ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_NoMenus)) {
            ImPlot::SetupAxis(ImAxis_Y1, "amplitude", ImPlotAxisFlags_AutoFit);

            // Axe Y sans labels ni ticks
            ImPlot::SetupAxis(ImAxis_X1, nullptr,
                                ImPlotAxisFlags_NoTickLabels |
                                ImPlotAxisFlags_NoTickMarks |
                                ImPlotAxisFlags_NoGridLines |
                                ImPlotAxisFlags_AutoFit );
                        //ImPlot::SetupAxes("i", "amplitude", ImPlotAxisFlags_AutoFit| ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_AutoFit);
                        
            ImPlot::PlotLine("samples",
                raw_data->data(),
                (int)raw_data->size());
            ImPlot::EndPlot();
        }
    }
    }


    if (latestResult){
    if (ImGui::Button("Restart integration")) {
            onReset();
        } 
        ImGui::SameLine();
        ImGui::Text("Time: %.2fs Progress: %.2f%%", latestResult->time, latestResult->getProgressPercent());
        if(ImGui::Button("Apply current drift Compensation")){
            if(drift){
                onApplyCorrection(drift->get_spm());
            }
        }
        ImGui::SameLine();
        ImGui::Text("Compensation %.2f spm", latestResult->correction_spm);
        ImVec2 size = ImGui::GetContentRegionAvail();
        // Le plot ImPlot
        if (ImPlot::BeginPlot("energy vs drift", size, ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxes("second per month", "energy",ImPlotAxisFlags_Opposite, ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, -60, 60);
            //ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
            auto spm =  latestResult->sec_per_month_corrected();
                        //latestResult->frequencies;
            ImPlot::PlotLine("second per month",
                spm.data(),
                latestResult->magnitudes.data(),
                (int)latestResult->frequencies.size());

            ImPlot::EndPlot();
        }

    } else {
        ImGui::Text("Waiting for data");
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


    glfwSwapBuffers(window);
}