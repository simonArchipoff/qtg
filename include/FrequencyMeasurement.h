#include <vector>
#include <deque>
#include <complex>

#include <kiss_fft.h>

#include "CircularBuffer.h"
#include "LinearRegression.h"

kiss_fft_cpx to_kiss(const float &v)
{
    kiss_fft_cpx c{};
    c.r = static_cast<float>(v);
    c.i = 0.0f;
    return c;
}

kiss_fft_cpx to_kiss(const std::complex<float> &v)
{
    kiss_fft_cpx c{};
    c.r = static_cast<float>(std::real(v));
    c.i = static_cast<float>(std::imag(v));
    return c;
}
inline float kiss_phase(const kiss_fft_cpx &c)
{
    return std::atan2(c.i, c.r);
}

template<typename T>
inline void unwrapPhases(std::vector<T> &phase)
{
    const T TWO_PI = 2.0f * M_PI;
    int k = 0;

    for (size_t i = 1; i < phase.size(); i++)
    {
        T diff = (phase[i] + k * TWO_PI) - phase[i - 1];

        if (diff > M_PI)
            k -= 1;
        else if (diff < -M_PI)
            k += 1;

        phase[i] += k * TWO_PI;
    }
}

std::vector<double> getSNR(const std::vector<double>& magnitude) {
    size_t N = magnitude.size();
    if (N == 0) return {};

    std::vector<double> snr(N);

    double totalPower = 0.0;
    for (double mag : magnitude) {
        totalPower += mag * mag;
    }

    for (size_t i = 0; i < N; ++i) {
        double signalPower = magnitude[i] * magnitude[i];
        double noisePower = totalPower - signalPower;
        if (noisePower <= 0.0)
            noisePower = 1e-12;
        snr[i] = 10.0 * std::log10(signalPower / noisePower);
    }

    return snr;
}


struct PhaseDriftResult{
    double frequency;
    double stddev;
};

inline struct PhaseDriftResult getPhaseDriftResult(double sampleRate, const std::vector<size_t> &time, const std::vector<float> phase){
    assert(time.size() >= 2);
    assert(time.size() == phase.size());
    std::vector<double> time_d(time.size());
    std::vector<double> phase_d(phase.size());
    for(size_t i = 0; i < phase.size(); i++){
        time_d[i] = static_cast<double>(time[i]);
        phase_d[i] = static_cast<double>(phase[i]);
    }
    unwrapPhases(phase_d);
    auto lin = linear_regression(time_d, phase_d);

    PhaseDriftResult r{ .frequency = sampleRate * lin.alpha        / (2*M_PI)
                      , .stddev    = sampleRate * lin.alpha_stderr / (2*M_PI)
                    };
    return r;
}


class FrequencyMeasurement
{
  protected:
    std::deque<std::pair<size_t, std::vector<kiss_fft_cpx>>> history;

    CircularBuffer<kiss_fft_cpx> circ;
    kiss_fft_cfg fft_cfg = nullptr;
    size_t frame;
    size_t period;
    bool add_window = true;

  public:
    FrequencyMeasurement() {}

    void init(int block_size, int period)
    {
        if (fft_cfg)
        {
            kiss_fft_free(fft_cfg);
            fft_cfg = nullptr;
        }
        fft_cfg = kiss_fft_alloc(block_size, 0, nullptr, nullptr);
        circ.init(block_size);
        frame = 0;
        this->period = period;
    }

    void addFFT()
    {
        assert(frame % period == 0);
        assert(circ.size() == circ.capacity());
        auto f = circ.get_ordered();
        assert(f.size() == circ.size());
        if (add_window)
        {
            for (size_t i = 0; i < f.size(); ++i)
            {
                float w = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (f.size() - 1)));
                f[i].r = f[i].r * w;
                f[i].i = f[i].i * w;
            }
        }
        std::vector<kiss_fft_cpx> tmp(f.size());
        kiss_fft(fft_cfg, f.data(), tmp.data());
        history.push_back(std::pair(frame, tmp));
    }

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
    //todo : use complex phasor?
    void getPhases(int idx, std::vector<size_t> &time, std::vector<float> &phases)
    {
        assert(circ.size() > idx);
        time.resize(0);
        time.reserve(history.size());
        phases.resize(0);
        phases.reserve(history.size());

        for (const auto &i : history)
        {
            time.push_back(i.first);
            phases.push_back(kiss_phase(i.second.at(idx)));
        }
    }

    std::vector<double> getFrequencies(double sampleRate){
        assert(history.size() > 0);
        std::vector<double> frequencies(circ.size());
        const auto &v = history[0].second;
        const size_t N = v.size();
        const double Nd = static_cast<double>(N);
        for (size_t i = 0; i < v.size(); i++){
            double f;
            if (i < N / 2){
                f = static_cast<double>(i) * sampleRate / Nd;
            }else{
                f = (static_cast<double>(i)-static_cast<double>(N)) * sampleRate / Nd;
            }
            frequencies[i] = f;
        }
        return frequencies;
    }

    std::vector<double> getMagnitude(){
        assert(history.size() > 0);
        const auto &v = history[0].second;
        const size_t N = v.size();
        std::vector<double> magnitude(N);
        for(size_t i = 0; i < N; i++){
            auto m = std::sqrt(  v[i].i * v[i].i
                               + v[i].r * v[i].r);
            magnitude[i] = m;
        }
        return magnitude;
    }

    std::vector<double> getSNR(){
        auto m = getMagnitude();
        return ::getSNR(m);
    }

};