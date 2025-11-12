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

class VuMeter
{
  public:
    VuMeter() {}

    void push_level(float level) { _level = std::clamp(level, 0.0f, 1.0f); }

    void draw(const ImVec2 &size = ImVec2(100, 10))
    {
        float epsilon = 1e-5f;
        float level_db = 20.0f * std::log10(_level + epsilon);

        float log_level = (level_db + 100.0f) / 100.0f;
        log_level = std::clamp(log_level, 0.0f, 1.0f);

        ImGui::ProgressBar(log_level, size, "");
    }

  private:
    float _level = 0.0f;
};

class ResultViewer
{
  public:
    ResultViewer(Unit u);
    ~ResultViewer();

    void renderFrame();
    void pushDriftResult(DriftResult drift) { this->drift = drift; }
    void pushRawData(std::vector<std::complex<float>> &);
    void setCurrentIntegration(double d) { currentIntegrationPart = d; }
    virtual void displayResult() = 0;
    bool shouldClose() const;
    std::function<void()> onReset = [&]() {};
    std::function<void(double)> onIntegrationTime = [&](double) { abort(); };
    std::function<void(double)> onApplyCorrection = [&](double) { abort(); };

  protected:
    enum Unit unit;
    GLFWwindow *window = nullptr;

    std::optional<DriftResult> drift;
    double currentIntegrationPart = 0.0;
    void processQueue();

  public:
    VuMeter vumeter;
};

class ResultViewerQuartz : public ResultViewer
{
  public:
    ResultViewerQuartz(Unit u) : ResultViewer(u) {};
    void displayResult() override;
    void pushResult(Result result);

  protected:
    std::optional<Result> latestResult;
    std::vector<std::complex<float>> raw_data;
};

struct Chronogram
{

    Chronogram(uint sampleRate, int freq, int duration)
        : tex_height(sampleRate * duration / freq), tex_width(sampleRate / freq),
          data(tex_width * tex_height, 0.0f), size(0), tmp(0)
    {
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_R32F, tex_width, tex_height, 0, GL_RED, GL_FLOAT, nullptr);
        for (auto &i : data)
        {
            i = 0.0;
        }
        UpdateTexture();
    }
    ~Chronogram()
    {
        if (texture_id)
        {
            glDeleteTextures(1, &texture_id);
        }
    }
    Chronogram(const Chronogram &) = delete;
    Chronogram &operator=(const Chronogram &) = delete;

    GLuint texture_id = 0;
    int tex_width;
    int tex_height;

    std::vector<float> data;
    int size;

    std::vector<float> tmp;

    void addSamples(const std::vector<float> &r)
    {
        int i = 0;
        while (i < r.size())
        {
            while (tmp.size() < tex_width && i < r.size())
            {
                tmp.push_back((r[i++]));
            }
            if(tmp.size() == tex_width)
                addLine();
        }
        UpdateTexture();
    }

  private:
    void addLine()
    {
        assert(tmp.size() == tex_width);

        if (size + tmp.size() > data.size())
        {
            //make room
            auto s = (size + tmp.size()) - data.size();
            size -= s;
            std::memmove(data.data(), data.data() + s, size * sizeof(data[0]));
        }
        std::memcpy(data.data() + size, tmp.data(), tmp.size() * sizeof(tmp[0]));
        size += tmp.size();
        tmp.resize(0);

        auto m = *std::max_element(data.data(), data.data()+size);
        if(m == 0.0)
          return;
        for(int i = 0; i < size; i++){
          data[i] /= m;
        }
    }

  public:
    void UpdateTexture()
    {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RED, GL_FLOAT, data.data());
    }

    void DrawMatrixTexture(ImVec2 size)
    {
        //ImGui::Text("Matrice de données (flux)");
        assert(texture_id);
        ImGui::Image(
            (ImTextureID)(intptr_t)texture_id, size/*ImVec2(10*tex_width,10*tex_height)*/, ImVec2(0, 0), ImVec2(1, 1));
    }
};

#include "MecaDSP.h"

class ResultViewerMeca : public ResultViewer
{
  public:
    ResultViewerMeca(Unit u) : ResultViewer(u), circ(2048), chrono(192, 7, 1) {}

    void pushResult(MecaDSPResult &r)
    {
        mecaresult = r;
        circ.push_back(r.newSamples);
        chrono.addSamples(r.newSamples);
    }
    void displayResult() override;

    MecaDSPResult mecaresult;
    CircularBuffer<float> circ;
    Chronogram chrono;
};