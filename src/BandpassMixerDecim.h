#pragma once

#include <cmath>

#include <Butterworth.h>


using namespace std;

class RealToBasebandDecimator {
public:
    RealToBasebandDecimator(  uint sample_rate
                            , uint input_size
                            , uint outputsize
                            , uint decimation_factor)
                            :frame(0),input_size(input_size),sample_rate(sample_rate), outputsize(outputsize),decimation_factor(decimation_factor)
    {
        bandpass.setup(8,sample_rate,32768,10);
        lowpass_i.setup(4,sample_rate, 1000);
        lowpass_q.setup(4,sample_rate, 1000);
        tmp_buff_i.resize(input_size);
        tmp_buff_q.resize(input_size);
    }

    void process(vector<float> & input_block, std::complex<float>*out) {

        auto tmp = input_block.data();
        bandpass.process(input_size, &tmp);

        for (size_t i = 0; i < input_size; ++i) {
            auto f = frame++ % sample_rate;;
            auto t = static_cast<double>(f) / static_cast<double>(sample_rate);
            auto phase = -2 * M_PI * 32760 * t;
            double c,s;
            sincos(phase,&s,&c);
            std::complex<float> carrier{static_cast<float>(c),static_cast<float>(-s)};
            
            tmp_buff_i[i] = c * input_block[i];
            tmp_buff_q[i] = s * input_block[i];
        }
        float* i_ptrs[1] = { tmp_buff_i.data() };
        float* q_ptrs[1] = { tmp_buff_q.data() };
        lowpass_i.process(input_size, i_ptrs);
        lowpass_q.process(input_size, q_ptrs);


        for(int i = 0; i < outputsize; i++){
            assert(i*decimation_factor < input_size);
            out[i] = std::complex<float>(tmp_buff_i[i*decimation_factor]
                                        ,tmp_buff_q[i*decimation_factor]);
        }
    }


private:
    unsigned long frame;
    unsigned long input_size,sample_rate,outputsize,decimation_factor;
    std::vector<float> tmp_buff_i;
    std::vector<float> tmp_buff_q;
    Dsp::SimpleFilter<Dsp::Butterworth::BandPass<8>, 1> bandpass;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 1> lowpass_i,lowpass_q;   
};
