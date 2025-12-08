#pragma once
#include <cmath>
#include <algorithm>

class PeakDetector {
public:
    PeakDetector(float sr, float fc) {
        set_cutoff(sr, fc);
        level = 0.0f;
    }
protected:
    void show_buffer(const float* buffer, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            float abs_sample = std::abs(buffer[i]);
            level = std::max(abs_sample, level * alpha + (1.0f - alpha) * abs_sample);
        }
    }
    void set_cutoff(float sr, float fc) {
        alpha = std::exp(-2.0f * static_cast<float>(M_PI) * fc / sr);
    }
public:
    float get_level() const {
        return level;
    }
private:
    float level = 0.0f;
    float alpha = 0.999f;
};