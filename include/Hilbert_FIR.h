#pragma once
#include <FIR.h>
#include <cstddef>
#include <cassert>
#include "FIR_hilbert_coefs.h"

struct Hilbert {
    unsigned int sr;
    FIR<coefs_re_48000_63> fir_re;
    FIR<coefs_im_48000_63> fir_im;

    Hilbert() : sr(0) {}

    Hilbert(unsigned int sampleRate) : sr(sampleRate) {
        assert(sampleRate == 48000);
    }

    void init(unsigned int sampleRate) {
        sr = sampleRate;
        assert(sr==48000);
    }

    void process(std::size_t size, float* input, float* output_r, float* output_i) {
        for(auto i = 0; i < size; i++){
            fir_re.process(input + i,output_r + i);
            fir_im.process(input + i ,output_i + i);
        }
    }
};
