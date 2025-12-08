#pragma once
#include <Butterworth.h>
#include <cstddef>
#include <cassert>
struct Hilbert{
    unsigned int sr;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<4>, 2> lowpass;
    Hilbert():sr(0){}
    Hilbert(unsigned int sampleRate):sr(sampleRate)
    {
        init(sampleRate);
    }

    void init(unsigned sampleRate){
        this->sr = sampleRate;
        lowpass.setup(4/*order*/, sr, sr / 4.0);
    }

    void process(std::size_t size, float * input,  float * output_r, float * output_i){
        assert(size % 4 == 0);
        assert(sr);
        
        //TODO : try write this as a single loop to perform one pass instead of 3 ?

        // modulate by complex sin at sr/4, values are 1, i, -1, -i
        for(std::size_t i = 0; i < size; i+=4){
            output_r[i+0] = input[i+0];  // * 1
            output_i[i+0] = 0.0;

            output_r[i+1] = 0.0;         // * i
            output_i[i+1] = input[i+1];

            output_r[i+2] = -input[i+2]; // * -1
            output_i[i+2] = 0.0;

            output_r[i+3] = 0.0;            
            output_i[i+3] = -input[i+3]; // * -i
        }
        float * c[] = {output_r,output_i};
        lowpass.process(size,c); // remove the negative frequencies

        // demodulate, values are 1, -i, -1, i, ...
        for(std::size_t i = 0; i < size; i+= 4){
            //output_r[i+0] = output_r[i+0]; // * 1
            //output_i[i+0] = output_i[i+0];

            output_r[i+1] =  output_i[i+1];  // * -i
            output_i[i+1] = -output_r[i+1];

            output_r[i+2] = -output_r[i+2];  // * -1
            output_i[i+2] = -output_i[i+2];
            
            output_r[i+3] = -output_i[i+3];  // * i
            output_i[i+3] =  output_r[i+3];
        }
    }
};