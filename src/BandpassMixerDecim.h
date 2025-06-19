
#pragma once

#include <cmath>

#include <Butterworth.h>


using namespace std;
template<uint sample_rate
        , uint input_size
        , uint outputsize
        , uint decimation_factor>
class RealToBasebandDecimator {
public:
    RealToBasebandDecimator()
    {
        frame=0;
        bandpass.setup(8,sample_rate,32768,1);
        lowpass_i.setup(4,sample_rate, 1000);
        lowpass_q.setup(4,sample_rate, 1000);

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
        float* i_ptrs[1] = { tmp_buff_i };
        float* q_ptrs[1] = { tmp_buff_q };
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
    float tmp_buff_i[input_size];
    float tmp_buff_q[input_size];
    Dsp::SimpleFilter<Dsp::Butterworth::BandPass<8>, 1> bandpass;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 1> lowpass_i,lowpass_q;   
};
