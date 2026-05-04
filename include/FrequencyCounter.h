#pragma once

#include <Butterworth.h>
#include "Constants.h"
#include <readerwriterqueue.h>
#include <cstddef>
#include "ResultSignal.h"
#include "CircularBuffer.h"

#include <DSPModule_rt.h>
#include <FrequencyMeasurement.h>

struct FrequencyCounterConfig
{
    int lo_freq = 0;
    unsigned int sample_rate_nominal = 0;
    unsigned int decimation_factor = 0;
    double duration_analysis_s = 1;

    double duration_between_analysis = 0.1; /* should probably be smaller than duration_analysis*/
    uint number_measures = 0; // 0 === unlimited

};

class FrequencyCounter_rt : public DSPModule_rt
{
  private:
    const struct FrequencyCounterConfig &config;

    unsigned long frame;
    unsigned long phase_decim;

    std::vector<float> tmp_buff_i;
    std::vector<float> tmp_buff_q;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 2> lowpass;

    moodycamel::ReaderWriterQueue<std::complex<float>> outputQueue;

  public:
    FrequencyCounter_rt(FrequencyCounterConfig &c);
    void init(std::size_t input_size) override
    {
        tmp_buff_i.resize(input_size);
        tmp_buff_q.resize(input_size);
    }
    size_t sampleRateNominal() const override { return config.sample_rate_nominal; }

    inline bool getOut(std::complex<float> &o) { return outputQueue.try_dequeue(o); }

    void rt_process(std::vector<float> &input_block) override;
};

class FrequencyCounterDSPAsync
{
  public:
    const struct FrequencyCounterConfig &config;
    FrequencyMeasurement freq_measurement;
    double real_sr = 0.0;
    FrequencyCounterDSPAsync(FrequencyCounterConfig &c);

    inline void push(std::complex<float> &o) { freq_measurement.addSamples({o}); }
    bool getResult(Result &r);
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
