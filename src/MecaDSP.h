#pragma once

#include <Butterworth.h>
#include "Constants.h"
#include <readerwriterqueue.h>
#include <cstddef>
#include "ResultSignal.h"
#include "CircularBuffer.h"
#include "Hilbert.h"

#include <DSPModule_rt.h>


struct MecaDSPConfig
{
    float low_pass_cut = 192/2;
    float high_pass_cut = 0.1;
    unsigned int decimation_factor = 500;
    unsigned int sample_rate = 96000;
};


struct MecaDSPResult{
    std::vector<float> newSamples;
    int sampleRate;
};

class MecaDSP_rt : public DSPModule_rt
{
  private:
    const struct MecaDSPConfig &config;
    unsigned long frame;
    unsigned long phase_decim;

    std::vector<float> tmp_buff_r;
    std::vector<float> tmp_buff_i;
    Hilbert hilbert;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 1> lowpass;
    Dsp::SimpleFilter<Dsp::Butterworth::HighPass<4>, 1> highpass;
    moodycamel::ReaderWriterQueue<float> outputQueue;

  public:
    MecaDSP_rt(MecaDSPConfig &c) : config(c), hilbert(c.sample_rate)
    {
        frame = 0;
        lowpass.setup(4, c.sample_rate, c.low_pass_cut);
        highpass.setup(4, c.sample_rate, c.high_pass_cut);
    }

    size_t sampleRate() const override{
        return config.sample_rate;
    }

    void init(std::size_t input_size) override
    {
        tmp_buff_r.resize(input_size);
        tmp_buff_i.resize(input_size);
    }

    inline bool getOut(float &o) { return outputQueue.try_dequeue(o); }

    void rt_process(std::vector<float> &input_block) override
    {
        assert(input_block.size() == tmp_buff_i.size());
        hilbert.process(
            input_block.size(), input_block.data(), tmp_buff_r.data(), tmp_buff_i.data());
        for (size_t i = 0; i < input_block.size(); i++)
        {
            tmp_buff_r[i] =
                std::sqrt(tmp_buff_r[i] * tmp_buff_r[i] + tmp_buff_i[i] * tmp_buff_i[i]);
        }
        auto tmp = tmp_buff_r.data();
        lowpass.process<float>(input_block.size(), &tmp);
        highpass.process<float>(input_block.size(), &tmp);

        for (unsigned int i = 0; i < input_block.size(); i++)
        {
            if (phase_decim % config.decimation_factor == 0)
            {
                auto out = tmp[i];
                outputQueue.enqueue(out);
                phase_decim = 0;
            }
            phase_decim++; // todo optimize that
        }
    }
};

class MecaDSPAsync
{
  public:
    const struct MecaDSPConfig &config;
    MecaDSPAsync(MecaDSPConfig &c) : config(c), real_sr(c.sample_rate), circbuf(10) {}

    inline void push(float &o) { this->circbuf.push_back(o); }
    bool getResult(MecaDSPResult &r){
        r.sampleRate = config.sample_rate / config.decimation_factor;
        if(circbuf.size() > 0){ 
            circbuf.get_ordered(r.newSamples);
            circbuf.reset();
            return true;
        } else{
            assert(false);
            //this shouldn't happen
            return false;
        }
    }
    void reset()
    {
        circbuf.reset();
        //for(uint i = 0; i < circbuf.capacity(); i++){
        //circbuf.push_back(std::complex<float>(0,0));
        //}
    }
    double real_sr = 0.0;
    CircularBuffer<float> circbuf;
};

class MecaDSP
{
  public:
    struct MecaDSPConfig config;
    MecaDSP(MecaDSPConfig &c) : config(c), rt(c), async(c) {}
    MecaDSP_rt rt;
    MecaDSPAsync async;
    bool new_data = false;

    bool getResult(MecaDSPResult &r)
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
    void runAsync()
    {
        float o;
        while (rt.getOut(o))
        {
            async.push(o);
            new_data = true;
        }
    }
};