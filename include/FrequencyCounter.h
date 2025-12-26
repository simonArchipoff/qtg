#pragma once

#include <Butterworth.h>
#include "Hilbert.h"
#include "Constants.h"
#include <readerwriterqueue.h>
#include <cstddef>
#include "ResultSignal.h"
#include "CircularBuffer.h"

#include <DSPModule_rt.h>
#include <FrequencyMeasurement.h>

struct FrequencyCounterConfig
{
    bool hilbert_shape_preprocessing = false;

    int lo_freq = 0;
    unsigned int sample_rate = 0;
    unsigned int decimation_factor = 0;
    double duration_analysis_s = 1;
};

class FrequencyCounter_rt : public DSPModule_rt
{
  private:
    const struct FrequencyCounterConfig &config;

    Hilbert hilbert;

    unsigned long frame;
    unsigned long phase_decim;

    std::vector<float> tmp_buff_i;
    std::vector<float> tmp_buff_q;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 2> lowpass;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 2> lowpass_decim;

    moodycamel::ReaderWriterQueue<std::complex<float>> outputQueue;

  public:
    FrequencyCounter_rt(FrequencyCounterConfig &c) : config(c), hilbert(c.sample_rate)
    {
        frame = 0;
        phase_decim = 0;
        assert(config.lo_freq * 2 <= config.sample_rate);
        auto sample_rate = config.sample_rate;
        auto decimation_factor = config.decimation_factor;

        auto fc = sample_rate / (2.0 * decimation_factor);
        lowpass.setup(1, sample_rate, fc);
        lowpass_decim.setup(4,sample_rate / config.decimation_factor, config.lo_freq / 2);
    }

    void init(std::size_t input_size) override
    {
        tmp_buff_i.resize(input_size);
        tmp_buff_q.resize(input_size);
    }
    size_t sampleRate() const override { return config.sample_rate; }

    inline bool getOut(std::complex<float> &o) { return outputQueue.try_dequeue(o); }

    void rt_process(std::vector<float> &input_block) override;
};

class FrequencyCounterDSPAsync
{
  public:
    const struct FrequencyCounterConfig &config;
    FrequencyMeasurement freq_measurement;
    double real_sr = 0.0;
    FrequencyCounterDSPAsync(FrequencyCounterConfig &c)
        : config(c), real_sr(c.sample_rate / c.decimation_factor)
    {
        auto s = kiss_fft_next_fast_size((c.sample_rate / c.decimation_factor) * c.duration_analysis_s);
        freq_measurement.init(s, std::max(s/10,1));
    }

    inline void push(std::complex<float> &o) { freq_measurement.addSamples({o}); }
    bool getResult(Result &r)
    {
        if (!freq_measurement.history_size())
            return false;
        r.magnitudes = freq_measurement.getMagnitude();
        r.frequencies = freq_measurement.getFrequencies(this->real_sr);
        for (auto &f : r.frequencies)
        {
            f += config.lo_freq;
        }
        auto snr = freq_measurement.getSNR();
        std::vector<double> freq_bis(r.frequencies.begin(), r.frequencies.end());
        auto imax = std::distance(snr.begin(),std::max_element(snr.begin(), snr.end()));

        if (freq_measurement.history_size() > 1)
        {
            for (uint i = 0; i < r.frequencies.size(); i++)
            {
                std::vector<size_t> t;
                std::vector<float> p;
                freq_measurement.getPhases(i, t, p);
                auto ri = ::getPhaseDriftResult(this->real_sr, t, p);
                //it doesn't depend on the bin? I am missing something
                //ri.frequency += ::getFrequencyNormBin(i, freq_measurement.getBlockSize()) * real_sr;
                freq_bis[i] += ri.frequency;
            }
            r.frequencies_corrected_phasedrift = freq_bis;
        }
        return true;
    }
    void reset() { freq_measurement.reset(); }
};

class FrequencyCounterDSP
{
  public:
    struct FrequencyCounterConfig config;
    FrequencyCounterDSP(FrequencyCounterConfig &c) : config(c), rt(c), async(c) {}
    FrequencyCounter_rt rt;
    FrequencyCounterDSPAsync async;
    bool new_data = false;

    bool getResult(Result &r)
    {
        runAsync();
        if (new_data)
        {
            auto res = async.getResult(r);
            new_data = false;
            return res;
        }
        return false;
    }

    void reset() { async.reset(); }
    void setRealSR(double sr) { this->async.real_sr = sr; }
    void runAsync()
    {
        std::complex<float> o;
        while (rt.getOut(o))
        {
            async.push(o);
            new_data = true;
        }
    }
};