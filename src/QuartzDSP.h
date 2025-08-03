#pragma once

#include <Butterworth.h>
#include "Constants.h"
#include <readerwriterqueue.h>
#include <cstddef>
#include "ResultSignal.h"
#include "CircularBuffer.h"

using namespace std;

struct QuartzDSPConfig {
    double target_freq = Constants::QUARTZ_FREQUENCY; 
    int lo_freq = Constants::QUARTZ_FREQUENCY;  
    double bw_bandpass = 6;
    unsigned int sample_rate = 96000;
    unsigned int decimation_factor = 32000;
    double duration_analysis_s = 180;
};

class QuartzDSP_rt {
private:
    const struct QuartzDSPConfig & config;
    unsigned long frame;
    unsigned long phase_decim;

    std::vector<float> tmp_buff_i;
    std::vector<float> tmp_buff_q;
    Dsp::SimpleFilter<Dsp::Butterworth::BandPass<8>, 1> bandpass;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 1> lowpass_i,lowpass_q;
    moodycamel::ReaderWriterQueue<std::complex<float>> outputQueue;

    public:
    QuartzDSP_rt(QuartzDSPConfig & c)
            :config(c)
    {
        frame = 0;
        phase_decim = 0;
        auto sample_rate = config.sample_rate;
        auto target_freq = config.target_freq;
        auto decimation_factor = config.decimation_factor;
        auto bw_bandpass = config.bw_bandpass;
        bandpass.setup(8,sample_rate,target_freq,bw_bandpass);
        auto fc = sample_rate / (2.0 * decimation_factor);
        lowpass_i.setup(4,sample_rate, fc);
        lowpass_q.setup(4,sample_rate, fc);
    }
    
    void init(std::size_t input_size){
        tmp_buff_i.resize(input_size);
        tmp_buff_q.resize(input_size);
    }

    inline bool getOut(std::complex<float>&o){
        return outputQueue.try_dequeue(o);
    }

    void rt_process(vector<float> &input_block);
};

class QuartzDSPAsync{
    public:
    const struct QuartzDSPConfig & config;
    QuartzDSPAsync(QuartzDSPConfig & c)
    :config(c)
    ,real_sr(c.sample_rate)
    ,circbuf(c.duration_analysis_s * c.sample_rate / c.decimation_factor)
    {}

    inline void push(std::complex<float> & o ){
        this->circbuf.push_back(o);
    }
    bool getResult(Result &r);
    void reset(){
        circbuf.reset();
        //for(uint i = 0; i < circbuf.capacity(); i++){
            //circbuf.push_back(std::complex<float>(0,0));
        //}
    }
    double real_sr = 0.0;
    CircularBuffer<std::complex<float>> circbuf;
};


class QuartzDSP {
    public:
    struct QuartzDSPConfig config;
    QuartzDSP(QuartzDSPConfig &c)
        :config(c)
        ,rt(c)
        ,async(c)
    {}
    QuartzDSP_rt rt;
    QuartzDSPAsync async;
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