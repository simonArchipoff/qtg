#include <kiss_fft.h>
#include <WeirdSTFT.h>
#include <LinearRegression.h>

#ifndef NDEBUG
//#include <npy/tensor.h>
#include <npy/npy.h>
#endif

template <typename T>
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

inline float kiss_phase(const kiss_fft_cpx &c)
{
    return std::atan2(c.i, c.r);
}

inline std::vector<double> getSNR(const std::vector<double> &magnitude, uint harmonics)
{
    size_t N = magnitude.size();
    if (N == 0)
        return {};

    std::vector<double> snr(N);

    double totalPower = 0.0;
    for (double mag : magnitude)
    {
        totalPower += mag * mag;
    }

    snr[0] = 10 / std::log10((magnitude[0] * magnitude[0]) / (totalPower - (magnitude[0] * magnitude[0])));

    for (size_t i = 1; i < N; ++i)
    {
        double signalPower = 0.0;
        for(uint j = 0; j <= harmonics; j++)
        {
            auto idx = i * (j+1);
            if(idx < magnitude.size()/2)
            {
                signalPower += magnitude[idx] * magnitude[idx];
            }
        }
        double noisePower = totalPower - signalPower;
        if (noisePower <= 0.0)
            noisePower = 1e-12;
        snr[i] = 10.0 * std::log10(signalPower / noisePower);
    }

    return snr;
}

struct PhaseDriftResult getPhaseDriftResult(
    double sampleRate, const std::vector<size_t> &time, const std::vector<float> phase)
{
    assert(time.size() >= 2);
    assert(time.size() == phase.size());
    std::vector<double> time_d(time.size());
    std::vector<double> phase_d(phase.size());
    for (size_t i = 0; i < phase.size(); i++)
    {
        time_d[i] = static_cast<double>(time[i]);
        phase_d[i] = static_cast<double>(phase[i]);
    }
    unwrapPhases(phase_d);
    auto lin = linear_regression(time_d, phase_d);

    std::vector<double> diff(phase_d.size() - 1);
    for (uint i = 0; i < diff.size(); i++)
    {
        diff[i] = phase_d[i] - phase_d[i + 1];
    }

    PhaseDriftResult r{.frequency = sampleRate * lin.alpha / (2 * M_PI),
        .stddev = sampleRate * lin.alpha_stderr / (2 * M_PI)};
    return r;
}

inline double getFrequencyNormBin(int bin, int N)
{
    const double Nd = static_cast<double>(N);
    if (bin < N / 2)
    {
        return static_cast<double>(bin) / Nd;
    }
    else
    {
        return (static_cast<double>(bin) - static_cast<double>(N)) / Nd;
    }
}
inline double getPeriodBin(int bin, int N)
{
    const double Nd = static_cast<double>(N);
    if (bin < N / 2)
    {
        return Nd / static_cast<double>(bin);
    }
    else
    {
        return Nd / (static_cast<double>(bin) - static_cast<double>(N));
    }
}

void WeirdSTFT::init(int block_size, int period, int size_history)
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
    this->size_history = size_history;
}
uint WeirdSTFT::getBlockSize() const
{
    return circ.capacity();
}

void WeirdSTFT::reset()
{
    circ.reset();
    history.resize(0);
    frame = 0;
}
int WeirdSTFT::history_size() const
{
    return history.size();
}
void WeirdSTFT::addFFT()
{
    assert(frame % period == 0);
    assert(frame >= period);
    assert(circ.size() == circ.capacity());
    auto f = circ.get_ordered();
    assert(f.size() == circ.size());
#if 0
    static int idx=0;
    // dump data
    std::vector<size_t> shape({f.size(),2});
    npy::tensor<float> t(shape);
    for(uint i = 0; i < f.size(); i++){
        t(i,0)=f[i].r;
        t(i,1)=f[i].i;
    }
    t.save("dump_signal" + std::to_string(idx++) +".npy");
#endif
    if (false && add_window)
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

    const auto n = getBlockSize();
    //phase coherence
    const auto fr = frame - 2 * period;
    double p = 2 * M_PI * (static_cast<double>((fr % n)) / n);

    for (uint i = 1; i < tmp.size(); i++)
    {
        double s, c;
        sincos(p * static_cast<double>(i), &s, &c);
        kiss_fft_cpx corr;
        corr.i = -s;
        corr.r = c;
        double cr = tmp[i].r * corr.r - tmp[i].i * corr.i;
        double ci = tmp[i].r * corr.i + tmp[i].i * corr.r;
        tmp[i].i = ci;
        tmp[i].r = cr;
    }

    history.push_back(std::pair(fr, tmp));
    if (size_history)
    {
        while (history.size() > size_history)
        {
            history.pop_front();
        }
    }
}

void WeirdSTFT::addSamples(const std::vector<std::complex<float>> &samples)
{
    addSamples(samples.begin(), samples.end());
}
//todo : use complex phasor?
void WeirdSTFT::getPhases(uint idx, std::vector<size_t> &time, std::vector<float> &phases)
{
    assert(circ.size() > idx);
    assert(history.size() > 0);
    time.clear();
    time.reserve(history.size());
    phases.clear();
    phases.reserve(history.size());
    double period = ::getPeriodBin(idx, circ.size());
    for (const auto &i : history)
    {
        time.push_back(i.first - history[0].first);
        auto p = kiss_phase(i.second.at(idx));
        phases.push_back(p); //std::fmod(diff,2*M_PI));
        assert(!isnan(p));
    }
}

std::vector<double> WeirdSTFT::getFrequencies(double sampleRate)
{
    assert(history.size() > 0);
    std::vector<double> frequencies(circ.size());
    const auto &v = history[0].second;
    const size_t N = v.size();
    const double Nd = static_cast<double>(N);
    for (size_t i = 0; i < v.size(); i++)
    {
        frequencies[i] = ::getFrequencyNormBin(i, N) * sampleRate;
    }
    return frequencies;
}

std::vector<double> WeirdSTFT::WeirdSTFT::getMagnitude(uint harmonics)
{
    assert(history.size() > 0);
    const auto &v = history[0].second;
    const size_t N = v.size();
    std::vector<double> magnitude(N);
    magnitude[0] = std::sqrt(v[0].i * v[0].i + v[0].r * v[0].r);
    for (size_t i = 1; i < N; i++)
    {
        magnitude[i] = 0;
        for (uint j = 0; j <= harmonics; j++)
        {
            auto idx = i * (j+1);
            if(idx < v.size()/2){
                auto m = std::sqrt(v[idx].i * v[idx].i + v[idx].r * v[idx].r);
                magnitude[i] = m;
            }
        }
    }
    return magnitude;
}

std::vector<double> WeirdSTFT::WeirdSTFT::getSNR(uint harmonics)
{
    auto m = getMagnitude(harmonics);
    return ::getSNR(m,harmonics);
}