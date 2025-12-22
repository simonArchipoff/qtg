#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <FrequencyCounter.h>
#if 1
TEST_CASE("FrequencyCounter", "[FrequencyCounter]") {
    FrequencyCounterConfig c;
    const uint sr = 48000;
    c.sample_rate = sr;
    c.hilbert_shape_preprocessing = false;
    c.lo_freq = 3;
    c.duration_analysis_s = 1;
    c.decimation_factor = 25 * 128;

    FrequencyCounterDSP dsp(c);

    const uint bs = 128;
    float frequency = 3.1;

    std::vector<float> signal;
    signal.resize(100 * sr);
    for(uint i = 0; i < signal.size(); i++){
        signal[i] = sin(static_cast<double>(i) * frequency * 2 * M_PI / static_cast<double>(sr));
    }
    dsp.rt.init(bs);

    SECTION("Valeurs par défaut") {
        uint idx =0;
        while(idx + bs < signal.size()){
            std::vector<float> tmp(signal.data() + idx,
                                   signal.data() + idx + bs);
            dsp.rt.rt_process(tmp);
            idx += bs;
            static int i = 0;
            if((++i)%100 == 0){
                Result r;
                if(dsp.getResult(r)){
                    (void)1;
                }
            }

        }
    }
}

#endif 