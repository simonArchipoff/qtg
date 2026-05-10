#pragma once

#include <vector>
#include <deque>
#include <complex>


#include "CircularBuffer.h"
#include <kiss_fft.h>


inline kiss_fft_cpx to_kiss(const float &v){
    kiss_fft_cpx c{};
    c.r = static_cast<float>(v);
    c.i = 0.0f;
    return c;
}

inline kiss_fft_cpx to_kiss(const std::complex<float> &v){
    kiss_fft_cpx c{};
    c.r = static_cast<float>(std::real(v));
    c.i = static_cast<float>(std::imag(v));
    return c;
}

struct PhaseDriftResult{
    double frequency;
    double stddev;
};

struct PhaseDriftResult getPhaseDriftResult(double sampleRate, const std::vector<size_t> &time, const std::vector<float> phase);

double getFrequencyNormBin(int bin, int N);
double getPeriodBin(int bin, int N);

class WeirdSTFT
{
  protected:
    std::deque<std::pair<size_t, std::vector<kiss_fft_cpx>>> history;

    CircularBuffer<kiss_fft_cpx> circ;
    kiss_fft_cfg fft_cfg = nullptr;
    size_t frame;
    size_t period;
    size_t size_history;
    bool add_window = true;

  public:
    WeirdSTFT() {}

    void init(int block_size, int period, int size_history=0 /*0 === unlimited*/);
    uint getBlockSize() const;

    void reset();
    int history_size() const;
    void addFFT();

    template <typename InputIt>
    void addSamples(InputIt first, InputIt last)
    {
        for (auto it = first; it != last; ++it)
        {
            circ.push_back(to_kiss(*it));
            frame++;
            if (frame % period == 0 && frame >= circ.capacity())
            {
                addFFT();
            }
        }
    }
    void addSamples(const std::vector<std::complex<float>> & samples);
    void getPhases(uint idx, std::vector<size_t> &time, std::vector<float> &phases);
    std::vector<double> getFrequencies(double sampleRate);
    std::vector<double> getMagnitude();
    std::vector<double> getSNR();
};
