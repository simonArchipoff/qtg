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

struct FrequencyCounterConfig {
    bool hilbert_shape_preprocessing = false;

    double target_freq = Constants::QUARTZ_FREQUENCY; 
    int lo_freq = Constants::QUARTZ_FREQUENCY;  
    double bw_bandpass = 6;
    unsigned int sample_rate = 96000;
    unsigned int decimation_factor = 32000;
    double duration_analysis_s = 180;
};

class FrequencyCounter_rt : public DSPModule_rt{
private:
    const struct FrequencyCounterConfig & config;

    Hilbert hilbert;

    unsigned long frame;
    unsigned long phase_decim;

    std::vector<float> tmp_buff_i;
    std::vector<float> tmp_buff_q;
    Dsp::SimpleFilter<Dsp::Butterworth::BandPass<8>, 1> bandpass;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 2> lowpass;
    moodycamel::ReaderWriterQueue<std::complex<float>> outputQueue;

    public:
    FrequencyCounter_rt(FrequencyCounterConfig & c)
            :config(c),hilbert(c.sample_rate)
    {
        frame = 0;
        phase_decim = 0;
        auto sample_rate = config.sample_rate;
        auto target_freq = config.target_freq;
        auto decimation_factor = config.decimation_factor;
        auto bw_bandpass = config.bw_bandpass;
        bandpass.setup(8,sample_rate,target_freq,bw_bandpass);
        auto fc = sample_rate / (2.0 * decimation_factor);
        lowpass.setup(4,sample_rate, fc);
    }
    
    void init(std::size_t input_size) override{
        tmp_buff_i.resize(input_size);
        tmp_buff_q.resize(input_size);
    }
    size_t sampleRate() const override{
        return config.sample_rate;
    }

    inline bool getOut(std::complex<float>&o){
        return outputQueue.try_dequeue(o);
    }

    void rt_process(std::vector<float> &input_block) override;
};

class FrequencyCounterDSPAsync{
    public:
    const struct FrequencyCounterConfig & config;
    FrequencyMeasurement freq_measurement;
    FrequencyCounterDSPAsync(FrequencyCounterConfig & c)
    :config(c)
    ,real_sr(c.sample_rate)
    {
        freq_measurement.init(512,512); // todo find smart values
    }

    inline void push(std::complex<float> & o ){
        freq_measurement.addSamples({o});
    }
    bool getResult(Result &r){
        
    }
    void reset(){
        freq_measurement.reset();
    }
    double real_sr = 0.0;
};


class FrequencyCounterDSP {
    public:
    struct FrequencyCounterConfig config;
    FrequencyCounterDSP(FrequencyCounterConfig &c)
        :config(c)
        ,rt(c)
        ,async(c)
    {}
    FrequencyCounter_rt rt;
    FrequencyCounterDSPAsync async;
    bool new_data=false;

    bool getResult(Result&r){
        runAsync();
        if(new_data)
        { 
            auto res = async.getResult(r);
            new_data = false;
            return res;
        }
        return false;
    }

    void reset(){
        async.reset();
    }
    void setRealSR(double sr){
        this->async.real_sr = sr;
    }
    void runAsync(){
        std::complex<float> o;
        while(rt.getOut(o)){
            async.push(o);
            new_data = true;
        }
    }
};