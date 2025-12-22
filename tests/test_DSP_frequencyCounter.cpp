#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <FrequencyCounter.h>

#include <npy/tensor.h>
#include <npy/npy.h>

#if 1
TEST_CASE("FrequencyCounter DSP", "[FrequencyCounter]") {
    FrequencyCounterConfig c;
    const uint sr = 48000;
    c.sample_rate = sr;
    c.hilbert_shape_preprocessing = false;
    c.lo_freq = 3;
    c.duration_analysis_s = 10;
    c.decimation_factor = 25 * 128;

    FrequencyCounterDSP dsp(c);

    const uint bs = 128;
    float frequency = 3.1;

    std::vector<float> signal;
    signal.resize(20 * sr);
    for(uint i = 0; i < signal.size(); i++){
        signal[i] = sin(static_cast<double>(i) * frequency * 2 * M_PI / static_cast<float>(sr));
    }
    dsp.rt.init(bs);
    std::vector<std::complex<float>> res;
    res.reserve(signal.size());

    SECTION("Valeurs par défaut") {
        uint idx =0;
        while(idx + bs < signal.size()){
            std::vector<float> tmp(signal.data() + idx,
                                   signal.data() + idx + bs);
            dsp.rt.rt_process(tmp);
            idx += bs;

            std::complex<float> c;
            while(dsp.rt.getOut(c)){
                res.push_back(c);
            }
        }

        std::vector<size_t> shape({res.size(),2});
        npy::tensor<float> t(shape);
        for(uint i = 0; i < res.size(); i++){
            t(i,0)=res[i].real();
            t(i,1)=res[i].imag();
        }
        t.save("dump_out_dsp.npy");
    }
}
#endif